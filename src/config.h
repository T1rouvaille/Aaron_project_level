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
#define KEY_LONG_PRESS_CNT   (200U)                                                   /* 长按阈值: 200 次 (2000ms) */

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
 *  显示层: 档位差值 (0.001 ft), SW1 切换形态时相对 A 档的递减量。
 *    A -> B 减 0.125 ft, B -> C 再减 0.250 ft (A -> C 累计 0.375 ft)。
 * ---------------------------------------------------------------------- */
#define FORM_OFFSET_B         (125UL)                                                 /* B = A - 0.125 ft */
#define FORM_OFFSET_C         (375UL)                                                 /* C = A - 0.375 ft */

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
