/*
 * power.c
 *
 *  电源控制实现: LATCH 开机保持 + EN 7V/5V 上电时序 + 无操作计时。
 */

#include "power.h"
#include "config.h"
#include <assert.h>

/* LDM/LASER 分级自动关闭阈值集中定义于 config.h */

static volatile uint32_t s_idle_sec = 0U;    /* 无操作累计秒数 (AGT1 中断累加, 按键清零) */
static volatile bool     s_tick_1s  = false; /* AGT1 1s 节拍标志 (中断置位, 主循环消费) */

static bool     s_key_out_active    = false;                   /* 外部按键输出进行中 */
static uint32_t s_key_out_remain_ms = 0U;                      /* 剩余低电平时间 (ms) */

void power_latch_init(void)
{
    R_IOPORT_PinCfg(&g_ioport_ctrl, LATCH_PIN, LATCH_PIN_INIT_CFG);
    R_IOPORT_PinWrite(&g_ioport_ctrl, LATCH_PIN, LATCH_LEVEL_ON);
}

void power_seq_init(void)
{
    /* 先拉高 EN 7V(P914), 延时 10ms, 再拉高 EN 5V(P915) */
    R_IOPORT_PinWrite(&g_ioport_ctrl, EN_7V_PIN, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(PWR_SEQ_DELAY_MS, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, EN_5V_PIN, BSP_IO_LEVEL_HIGH);

    /* EN_5V 拉高后直接拉低 P002 到空闲态 (不再发送上电握手脉冲) */
    power_key_out_init();
}

void power_off(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, LATCH_PIN, LATCH_LEVEL_OFF);
}

bool power_latch_is_low(void)
{
    bsp_io_level_t level = BSP_IO_LEVEL_LOW;

    R_IOPORT_PinRead(&g_ioport_ctrl, LATCH_PIN, &level);
    return (BSP_IO_LEVEL_LOW == level);
}

void power_key_out_init(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, KEY_OUT_PIN, KEY_OUT_LEVEL_IDLE);
    s_key_out_active    = false;
    s_key_out_remain_ms = 0U;
}

void power_key_out_start(uint32_t duration_ms)
{
    /* 已有输出进行中, 忽略新触发 */
    if (s_key_out_active)
    {
        return;
    }

    /* 拉高产生上升沿, 记录时长, 由 poll 计时结束后拉低 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, KEY_OUT_PIN, KEY_OUT_LEVEL_ACTIVE);
    s_key_out_remain_ms = duration_ms;
    s_key_out_active    = true;
}

void power_key_out_poll(void)
{
    if (!s_key_out_active)
    {
        return;
    }

    /* 按 KEY_OUT_POLL_MS 步长递减, 计时结束拉高释放 */
    if (s_key_out_remain_ms > KEY_OUT_POLL_MS)
    {
        s_key_out_remain_ms -= KEY_OUT_POLL_MS;
    }
    else
    {
        R_IOPORT_PinWrite(&g_ioport_ctrl, KEY_OUT_PIN, KEY_OUT_LEVEL_IDLE);
        s_key_out_active    = false;
        s_key_out_remain_ms = 0U;
    }
}

/* AGT1 1s 周期中断回调: 仅置节拍标志 + 无操作秒数累加 (保持中断短小) */
void agt1_stats_callback(timer_callback_args_t *p_args)
{
    if (TIMER_EVENT_CYCLE_END != p_args->event)
    {
        return;
    }

    s_tick_1s = true;
    s_idle_sec++;
}

/* 读取并清除 1s 节拍标志 (主循环调用, 用于时间统计) */
bool power_1s_tick_pending(void)
{
    bool pending;

    __disable_irq();
    pending   = s_tick_1s;
    s_tick_1s = false;
    __enable_irq();

    return pending;
}

/* 启动 AGT1 1s 定时器 (提供 1s 节拍 + 无操作秒数累加) */
void power_autooff_init(void)
{
    fsp_err_t err = R_AGT_Open(&g_agt1_ctrl, &g_agt1_cfg);
    assert(FSP_SUCCESS == err);

    err = R_AGT_Start(&g_agt1_ctrl);
    assert(FSP_SUCCESS == err);
}

/* 按键活动: 重置无操作计时 */
void power_activity(void)
{
    __disable_irq();
    s_idle_sec = 0U;
    __enable_irq();
}

/* 读取当前无操作累计秒数 (32 位对齐读在 Cortex-M23 上原子, 直接返回) */
uint32_t power_get_idle_sec(void)
{
    return s_idle_sec;
}
