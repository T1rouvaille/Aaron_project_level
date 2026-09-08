#include "hal_data.h"
#include "config.h"
#include "debug_uart/bsp_debug_uart.h"
#include "iic/iic.h"
#include "iic/imu.h"
#include "flash/flash.h"
#include "gpt/gpt.h"
#include "key/key.h"
#include "lcd/lcd.h"
#include "display/display.h"
#include "power/power.h"
#include "adc/adc.h"
#include "battery/battery.h"
#include "param/param.h"
#include "command/command.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

/* ----------------------------------------------------------------------
 *  应用初始化: 外设初始化 + 启动打印 + UI 状态机初始化
 * ---------------------------------------------------------------------- */
static void app_init(ui_state_t *ui)
{
    /* LATCH 上电保持：P301 拉高开机 */
    power_latch_init();

    /* UART 初始化 */
    Debug_UART9_Init();

    /* I2C 初始化 */
    i2c_master_init();

    /* Flash 初始化 */
    flash_init();

    /* 参数存储: 读回配置/统计, 首次上电写入默认 */
    param_init();

    /* IMU 识别与初始化 (IIM-42351 / ICM-40608 / LSM6DSOTR) */
    if (IMU_SENSOR_NONE == imu_init())
    {
        /* 三款 IMU 均未识别: 记录 I2C 失败关机原因并关机 */
        param_set_shutdown_reason(SHUTDOWN_I2C_FAIL);
        param_save();
        power_off();

        /* LATCH 已拉低, 等待系统断电 */
        while (1)
        {
        }
    }

    /* LCD 初始化 (AiP31033E, 硬件 SPI + GPIO A0/CS/RST) */
    lcd_init();

    /* 1ms 心跳定时器启动 (GPT9) */
    gpt_init();

    /* 按键初始化 (SW1~SW4, 高电平触发) */
    key_init();

    /* 电源时序: 先拉高 EN 7V(P914), 延时 10ms, 再拉高 EN 5V(P915) */
    power_seq_init();

    /* ADC 初始化 (buck 7V 电压检测 P014 + 电池电量检测 P015) */
    adc_init();

    /* 自动关机定时器: AGT1 1s, 无操作 60s 后关机 */
    power_autooff_init();

    /* 启动信息打印 */
    uart9_send_blocking("\r\n========================================\r\n");
    uart9_send_blocking("  Hanger LDM V0.1  System Boot\r\n");
    uart9_send_blocking("========================================\r\n");
    uart9_send_blocking("[PWR ] LATCH ON (P301 HIGH)\r\n");
    uart9_send_blocking("[UART] UART9 Init OK\r\n");
    uart9_send_blocking("[I2C ] I2C Init OK\r\n");
    uart9_send_blocking("[FLSH] Flash Init OK\r\n");
    uart9_send_blocking("[LCD ] LCD Init OK\r\n");
    uart9_send_blocking("[TMR ] GPT9 1ms Tick OK\r\n");
    uart9_send_blocking("[ADC ] ADC Init OK\r\n");
    uart9_send_blocking("System init OK\r\n");

    /* 初始化状态机到开机初始态并刷屏 */
    ui_state_init(ui);

    /* 开机键检测: 按 SW4 开机 -> 显示 T7 + 拉高 P000; 否则首次短按立即响应 */
    if (key_is_pressed(KEY_ID_SW4))
    {
        ui->t7 = T7_ON;
        display_refresh(ui);
    }
    else
    {
        ui->sw4_first = false;
    }
}

/* ----------------------------------------------------------------------
 *  按键事件处理 (10ms 周期): 扫描 + 串口指令 + 按键分派
 * ---------------------------------------------------------------------- */
static void app_process_keys(ui_state_t *ui)
{
    uint8_t i;

    key_scan();

    /* 串口接收: 统一指令分发 (指令或米值) */
    if (uart9_line_ready())
    {
        char line[32];

        uart9_get_line(line, sizeof(line));
        command_process_line(line, ui);
    }

    /* 按键事件: 映射为 UI 事件并分派给状态机 */
    for (i = 0U; i < KEY_NUM; i++)
    {
        key_event_t ev = key_get_event((key_id_t)i);

        /* 任意按键事件均重置 60s 自动关机倒计时 */
        if (KEY_EVENT_NONE != ev)
        {
            power_activity();
        }

        if (KEY_EVENT_SHORT_PRESS == ev)
        {
            /* 记录短按次数 */
            param_key_event(i, false);

            /* 按键 ID -> 动作 (SW1~SW4 分派 UI 事件; SW3 额外触发 P002 短按脉冲) */
            switch (i)
            {
                case KEY_ID_SW1: ui_state_dispatch(ui, UI_EVT_SW1_SHORT); break;
                case KEY_ID_SW2: ui_state_dispatch(ui, UI_EVT_SW2_SHORT); break;
                case KEY_ID_SW3:
                    ui_state_dispatch(ui, UI_EVT_SW3_SHORT);
                    power_key_out_start(KEY_OUT_SHORT_MS);
                    break;
                case KEY_ID_SW4: ui_state_dispatch(ui, UI_EVT_SW4_SHORT); break;
                default: break;
            }
        }
        else if (KEY_EVENT_LONG_PRESS == ev)
        {
            /* 记录长按次数 */
            param_key_event(i, true);

            /* SW3/SW4 长按关机 */
            if ((KEY_ID_SW3 == i) || (KEY_ID_SW4 == i))
            {
                param_set_shutdown_reason(SHUTDOWN_MANUAL);
                param_save();
                power_off();
            }
        }
    }
}

/* ----------------------------------------------------------------------
 *  100ms 周期任务: IMU 方向检测 + 电源监控 + 保护关机
 * ---------------------------------------------------------------------- */
static void app_process_100ms(ui_state_t *ui)
{
    /* IMU 方向检测 (迟滞死区 + 去抖计数: 连续多次确认才切换) */
    int16_t acc[3];
    if (0 == imu_get_acc(acc))
    {
        static direction_t s_candidate     = DIR_FORWARD;
        static uint32_t    s_candidate_cnt = 0U;

        direction_t target = ui->direction;

        /* 迟滞死区: 越过阈值才产生候选方向 */
        if (DIR_FORWARD == ui->direction)
        {
            /* 当前正向 (X<0): 越过正阈值才候选反向 */
            if (acc[0] > (int16_t)IMU_DIR_HYST_TH)
            {
                target = DIR_REVERSE;
            }
        }
        else
        {
            /* 当前反向 (X>=0): 越过负阈值才候选正向 */
            if (acc[0] < -(int16_t)IMU_DIR_HYST_TH)
            {
                target = DIR_FORWARD;
            }
        }

        /* 去抖: 候选方向连续确认 IMU_DIR_CONFIRM_CNT 次才真正切换 */
        if (target != ui->direction)
        {
            if (target == s_candidate)
            {
                s_candidate_cnt++;
            }
            else
            {
                s_candidate     = target;
                s_candidate_cnt = 1U;
            }

            if (s_candidate_cnt >= IMU_DIR_CONFIRM_CNT)
            {
                ui_state_set_direction(ui, target);
                s_candidate_cnt = 0U;
            }
        }
        else
        {
            /* 方向无变化, 清空候选状态 */
            s_candidate     = target;
            s_candidate_cnt = 0U;
        }
    }

    /* ADC 采样: 读 buck 7V + 电池原始值到缓存 (100ms 周期) */
    adc_sample_update();

    /* 电源监控: 更新电量格数 + 低电/BUCK 失效关机判定 */
    battery_update();
    ui_state_set_battery(ui, battery_get_level());

    /* 保护关机: BUCK 失效优先级高于低电 */
    if (battery_buck_need_shutdown())
    {
        static bool buck_handled = false;

        if (!buck_handled)
        {
            buck_handled = true;
            param_set_shutdown_reason(SHUTDOWN_BUCK_FAIL);
            param_save();
        }

        power_off();
    }
    else if (battery_need_shutdown())
    {
        static bool handled = false;

        if (!handled)
        {
            handled = true;
            /* 记录低电关机原因并写回 Flash (仅触发一次) */
            param_set_shutdown_reason(SHUTDOWN_LOW_BATT);
            param_save();
        }

        power_off();
    }
}

/* ----------------------------------------------------------------------
 *  主循环: 按节拍分派各周期任务
 * ---------------------------------------------------------------------- */
static void app_run(ui_state_t *ui)
{
    while (1)
    {
        /* 检测到 LATCH 拉低 (任一关机路径触发) 则清屏 */
        if (power_latch_is_low())
        {
            lcd_clear();
        }

        /* AGT1 1s 节拍: 时间统计 */
        if (power_1s_tick_pending())
        {
            param_tick_1s(battery_is_low());
        }

        /* 1ms 任务 */
        if (g_task_flags & TASK_FLAG_1MS)
        {
            gpt_flag_clear(TASK_FLAG_1MS);
            /* TODO: 添加 1ms 任务 */
        }

        /* 10ms 任务: 按键 + 串口 + 外部按键输出状态机 */
        if (g_task_flags & TASK_FLAG_10MS)
        {
            gpt_flag_clear(TASK_FLAG_10MS);
            power_key_out_poll();
            app_process_keys(ui);
        }

        /* 20ms 任务 */
        if (g_task_flags & TASK_FLAG_20MS)
        {
            gpt_flag_clear(TASK_FLAG_20MS);
            /* TODO: 添加 20ms 任务 */
        }

        /* 50ms 任务: 基准点闪烁节拍 */
        if (g_task_flags & TASK_FLAG_50MS)
        {
            gpt_flag_clear(TASK_FLAG_50MS);
            ui_state_blink_tick(ui);
        }

        /* 100ms 任务: IMU + ADC */
        if (g_task_flags & TASK_FLAG_100MS)
        {
            gpt_flag_clear(TASK_FLAG_100MS);
            app_process_100ms(ui);
        }

        /* 1000ms 任务 */
        if (g_task_flags & TASK_FLAG_1000MS)
        {
            gpt_flag_clear(TASK_FLAG_1000MS);
        }
    }
}

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    ui_state_t ui;

    R_BSP_PinAccessEnable();
    app_init(&ui);
    app_run(&ui);
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0
        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;
        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */
        /* Configure pins. */
        R_IOPORT_Open (&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);

#if BSP_CFG_SDRAM_ENABLED
        /* Setup SDRAM and initialize it. Must configure pins first. */
        R_BSP_SdramInit(true);
#endif
    }
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
FSP_CPP_FOOTER

#endif
