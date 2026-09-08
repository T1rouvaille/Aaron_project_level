#ifndef CONFIG_H_
#define CONFIG_H_

#include "hal_data.h"

/* ======================================================================
 *  系统集中配置 (可调参数)
 *  各模块「按需求可能调整」的参数集中于此, 便于统一维护;
 *  硬件固定参数 (命令字/段映射/寄存器等) 仍留在各模块内部。
 * ====================================================================== */

/* ----------------------------------------------------------------------
 *  LATCH 电源控制引脚
 *  P301 作为 LATCH 信号：
 *    拉高 (HIGH) = 开机
 *    拉低 (LOW)  = 关机
 * ---------------------------------------------------------------------- */
#define LATCH_PIN            (BSP_IO_PORT_03_PIN_01)                                /* P301 */
#define LATCH_PIN_INIT_CFG   (IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW) /* 初始输出低电平 */
#define LATCH_LEVEL_ON       (BSP_IO_LEVEL_HIGH)                                     /* 开机电平 */
#define LATCH_LEVEL_OFF      (BSP_IO_LEVEL_LOW)                                      /* 关机电平 */

/* ----------------------------------------------------------------------
 *  电源时序引脚 (EN 7V -> EN 5V)
 *  上电时序: 先拉高 EN 7V, 延时 10ms, 再拉高 EN 5V
 *  (两引脚在 pin_data.c 中已初始化为输出低)
 * ---------------------------------------------------------------------- */
#define EN_7V_PIN            (BSP_IO_PORT_09_PIN_14)                                  /* P914 EN 7V */
#define EN_5V_PIN            (BSP_IO_PORT_09_PIN_15)                                  /* P915 EN 5V */
#define PWR_SEQ_DELAY_MS     (10U)                                                    /* EN 7V -> EN 5V 延时 */

/* 无操作自动关机阈值 (秒) */
#define AUTO_OFF_SEC         (60U)

/* ----------------------------------------------------------------------
 *  按键引脚 (SW1~SW4, 高有效: 按下接 VCC)
 * ---------------------------------------------------------------------- */
#define KEY_SW1_PIN          (BSP_IO_PORT_09_PIN_13)                                  /* SW1: P913 */
#define KEY_SW2_PIN          (BSP_IO_PORT_04_PIN_07)                                  /* SW2: P407 */
#define KEY_SW3_PIN          (BSP_IO_PORT_04_PIN_08)                                  /* SW3: P408 */
#define KEY_SW4_PIN          (BSP_IO_PORT_04_PIN_09)                                  /* SW4: P409 */

/* 按键输入配置 (RA2E1 无内部下拉, 高有效需外部下拉) */
#define KEY_PIN_INPUT_CFG    (IOPORT_CFG_PORT_DIRECTION_INPUT)

/* 按键检测参数 (基于 10ms 扫描周期) */
#define KEY_DEBOUNCE_CNT     (2U)                                                     /* 消抖: 连续 2 次 (20ms) */
#define KEY_LONG_PRESS_CNT   (300U)                                                   /* 长按阈值: 300 次 (3000ms) */

/* ----------------------------------------------------------------------
 *  LCD 引脚 (A0 = P104 数据/命令, RST = P110 复位, CS = P111 片选)
 * ---------------------------------------------------------------------- */
#define LCD_A0_PIN           (BSP_IO_PORT_01_PIN_04)
#define LCD_RST_PIN          (BSP_IO_PORT_01_PIN_10)
#define LCD_CS_PIN           (BSP_IO_PORT_01_PIN_11)

/* LCD 默认对比度 (EV 0~31, 越大越亮; 过高出现鬼影) */
#define LCD_DEFAULT_EV        (0x0FU)

/* ----------------------------------------------------------------------
 *  T7 指示输出引脚
 *  P000 作为 T7 十字加号状态输出:
 *    T7 显示时拉高, 不显示时拉低。
 *  (引脚在 ra_gen/pin_data.c 中已初始化为输出低电平)
 * ---------------------------------------------------------------------- */
#define T7_CTRL_PIN           (BSP_IO_PORT_00_PIN_00)                                /* P000 */
#define T7_CTRL_LEVEL_ON      (BSP_IO_LEVEL_HIGH)                                     /* T7 显示 */
#define T7_CTRL_LEVEL_OFF     (BSP_IO_LEVEL_LOW)                                      /* T7 隐藏 */

/* ----------------------------------------------------------------------
 *  外部按键输出引脚 P002: 触发另一 MCU 的上升沿按键输入。
 *    空闲态保持低电平; 模拟按键时拉高 (产生上升沿) 持续一段时间再拉低。
 *  (P002 在 pin_data.c 中已初始化为输出高, 上电时序中由 power_seq_init 拉低)
 * ---------------------------------------------------------------------- */
#define KEY_OUT_PIN           (BSP_IO_PORT_00_PIN_02)                                /* P002 */
#define KEY_OUT_LEVEL_IDLE    (BSP_IO_LEVEL_LOW)                                     /* 空闲低电平 */
#define KEY_OUT_LEVEL_ACTIVE  (BSP_IO_LEVEL_HIGH)                                    /* 按下高电平 */

/* 三种按键模式时长 (ms), 串口指令模拟使用 */
#define KEY_OUT_SHORT_MS      (120U)                                                 /* 短按 */
#define KEY_OUT_LONG3_MS      (3000U)                                                /* 长按 3s */
#define KEY_OUT_LONG5_MS      (5000U)                                                /* 长按 5s */

/* 状态机轮询步长 (ms), 对应主循环 10ms 任务周期 */
#define KEY_OUT_POLL_MS       (10U)

/* ----------------------------------------------------------------------
 *  显示层: 档位差值 (0.001 ft), SW1 切换形态时相对 A 档的递减量。
 *    A -> B 减 0.125 ft, B -> C 再减 0.250 ft (A -> C 累计 0.375 ft)。
 * ---------------------------------------------------------------------- */
#define FORM_OFFSET_B         (125UL)                                                 /* B = A - 0.125 ft */
#define FORM_OFFSET_C         (375UL)                                                 /* C = A - 0.375 ft */

/* LDM 测量开启时基准点闪烁周期: 5 x 50ms = 250ms 亮/灭翻转一次 */
#define LCD_BLINK_TICK_CNT    (5U)

/* ----------------------------------------------------------------------
 *  IMU 方向检测迟滞死区 + 去抖计数 (acc[0] 原始 LSB)
 *  方向映射: acc[0] < 0 -> 正向, acc[0] >= 0 -> 反向。
 *  当前正向时, 需 acc[0] > +TH 才候选反向;
 *  当前反向时, 需 acc[0] < -TH 才候选正向; [-TH, +TH] 为死区保持原方向。
 *  候选方向需连续确认 CONFIRM_CNT 次 (100ms 周期) 才真正切换。
 *  注意: TH 需小于静止 1g 对应的 LSB, 否则方向永不切换
 *        (TDK ±4g: 1g≈8192; LSM6DSO ±2g: 1g≈16384)。
 * ---------------------------------------------------------------------- */
#define IMU_DIR_HYST_TH       (10000)                                                 /* 迟滞死区阈值 (LSB) */
#define IMU_DIR_CONFIRM_CNT   (5U)                                                    /* 去抖连续确认次数 (100ms/次) */

/* ----------------------------------------------------------------------
 *  ADC 通道映射 (RA2E1A73)
 *    P014 -> AN009 (ADC_CHANNEL_9)  : buck 7V 电压
 *    P015 -> AN010 (ADC_CHANNEL_10) : 电池电压
 * ---------------------------------------------------------------------- */
#define ADC_CH_BUCK_7V        (ADC_CHANNEL_9)
#define ADC_CH_BATTERY        (ADC_CHANNEL_10)

/* 电池电量跳变阈值 (ADC 原始值, 实测标定) */
#define ADC_BATT_3TO2_RAW     (1390U)                                                 /* 3 格 -> 2 格 (4.05V) */
#define ADC_BATT_2TO1_RAW     (1280U)                                                 /* 2 格 -> 1 格 (3.45V) */
#define ADC_BATT_OFF_RAW      (1185U)                                                 /* 关机阈值    (<3.0V)   */

/* 回滞带 (ADC 原始值): 上升阈值 = 跳变阈值 + 回滞带 */
#define ADC_BATT_HYST         (20U)

/* 低电关机防抖: 连续低于关机阈值的采样次数 (100ms 周期) */
#define ADC_BATT_LOW_CNT_MAX  (3U)

/* ----------------------------------------------------------------------
 *  buck 7V 输出正常范围 (ADC 原始值): 超出判定 BUCK 芯片失效并保护关机。
 *  实测电池 4.5V~3V 放电全程 buck raw 约 897~1078, 此处放宽到 [750, 1300]。
 * ---------------------------------------------------------------------- */
#define ADC_BUCK_RAW_MIN      (750U)   /* 正常下限: 低于则 BUCK 失效 */
#define ADC_BUCK_RAW_MAX      (1300U)  /* 正常上限: 高于则 BUCK 失效 */

/* BUCK 失效防抖: 连续超范围采样次数 (100ms 周期) */
#define ADC_BUCK_FAIL_CNT_MAX (3U)

/* ADC 参考电压 (V) 与 12bit 分辨率 (用于原始值 -> 电压换算, 当前仅预留) */
#define V_ref                 (3.3f)
#define ADC_12_BIT            (4096u)

/* ----------------------------------------------------------------------
 *  参数存储 (Data Flash)
 *  基础配置 + 运行时统计持久化, 详见 param 模块。
 * ---------------------------------------------------------------------- */
#define PARAM_FLASH_OFFSET    (0U)          /* 存储起始偏移 (相对 Data Flash 基地址) */
#define PARAM_FLASH_BLOCKS    (3U)          /* 占用块数 (3 x 64B = 192B >= sizeof(param_t)) */

/* 基础信息默认值 (首次上电写入 Flash) */
#define PARAM_PROJECT_NAME    "Hanger_Ldm_Level"
#define PARAM_PART_NUMBER     "NA123456"
#define PARAM_SW_VERSION      "V0.0.1"

#endif /* CONFIG_H_ */
