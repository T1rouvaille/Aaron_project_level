/*
 * display.h
 *
 *  LCD 应用显示层: UI 状态机 + 图标/数字渲染。
 *  依赖底层 lcd.c (SPI 驱动) 与 lcd_seg_map (段映射)。
 *  本层负责「状态如何变化」(ui_state_*) 与「画什么」(display_refresh),
 *  不涉及 SPI/时序等硬件细节, 便于移植到其他 LCD 驱动。
 */

#ifndef DISPLAY_DISPLAY_H_
#define DISPLAY_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>

/* ======================================================================
 *  状态定义 (明确枚举)
 * ====================================================================== */

/* 图标形式: SW1 短按在 A -> B -> C -> A 间循环。
 *   FORM_IDLE 仅作开机初始态 (不按键时的基础态), 不参与循环。 */
typedef enum
{
    FORM_IDLE = 0,   /* 开机初始态: T1/T2/T9~T12 常亮, 无数字 */
    FORM_A,          /* T1+T4+T5+T8+T13 + 数字 */
    FORM_B,          /* T3+T5+T8+T13   + 数字 */
    FORM_C,          /* T6+T8+T13      + 数字 */
    FORM_NUM,        /* 形式总数 (用于循环计数) */
} form_t;

/* 数字换算态: SW2 短按循环, 7 态单位换算 (无关闭态)。 */
typedef enum
{
    DIGIT_FT_DEC = 0,   /* 十进制英尺 */
    DIGIT_FT_FRAC,      /* 英尺 + 1/8 分数 */
    DIGIT_FT_IN,        /* 英尺 + 英寸 (双单位) */
    DIGIT_IN_DEC,       /* 十进制英寸 */
    DIGIT_IN_FRAC,      /* 英寸 + 1/2 分数 */
    DIGIT_M,            /* 米 */
    DIGIT_CM,           /* 厘米 (4 位整数 + 1 位小数) */
    DIGIT_NUM,          /* 数字态总数 (7) */
} digital_t;

/* T7 十字加号显示状态: SW4 短按往复切换。 */
typedef enum
{
    T7_OFF = 0,   /* 不显示 T7 */
    T7_ON,        /* 显示 T7 */
} t7_t;

/* 显示方向: 由 IMU 加速度计 X 轴检测。 */
typedef enum
{
    DIR_FORWARD = 0,   /* 正向 */
    DIR_REVERSE,       /* 反向 (180° 倒置) */
} direction_t;

/* ======================================================================
 *  UI 状态机上下文 (集中管理所有显示状态)
 * ====================================================================== */
typedef struct
{
    form_t      form;        /* 图标形式 */
    digital_t   digital;     /* 数字换算态 */
    t7_t        t7;          /* T7 十字加号 */
    direction_t direction;   /* 显示方向 */
    uint32_t    value_ft;    /* A 档基准值 (0.001 ft), 由串口米值换算 */
    uint8_t     battery_level; /* 电池电量格数 (0~3) */
    bool        sw4_first;   /* SW4 首次按下忽略标志 */
    bool        ldm_on;      /* LDM 测量开关 (SW3 短按往复) */
    bool        ldm_ever_on; /* LDM 是否曾经开启过 (控制 SW1 有效性) */
} ui_state_t;

/* ======================================================================
 *  状态机输入事件 (由按键产生, hal_entry 负责按键 ID -> 事件映射)
 * ====================================================================== */
typedef enum
{
    UI_EVT_SW1_SHORT = 0,   /* SW1 短按: 图标形式循环 */
    UI_EVT_SW2_SHORT,       /* SW2 短按: 数字态循环 */
    UI_EVT_SW3_SHORT,       /* SW3 短按: LDM 测量开/关往复 */
    UI_EVT_SW4_SHORT,       /* SW4 短按: T7 往复 (首次忽略) */
} ui_event_t;

/* ======================================================================
 *  状态机接口
 * ====================================================================== */

/* 初始化状态机到开机初始态并刷屏 */
void ui_state_init(ui_state_t *s);

/* 分派一个 UI 事件: 执行状态转移并刷屏 */
void ui_state_dispatch(ui_state_t *s, ui_event_t ev);

/* 设置显示方向: 方向变化时才刷屏 */
void ui_state_set_direction(ui_state_t *s, direction_t dir);

/* 设置测量值 (单位毫米): 由串口米值换算后调用, 开机 IDLE 态自动进入 A 形态并默认显示米 */
void ui_state_set_value(ui_state_t *s, uint32_t value_mm);

/* 设置电池电量格数 (0~3): 变化时才刷屏 */
void ui_state_set_battery(ui_state_t *s, uint8_t level);

/* 按当前状态刷新整屏 (渲染) */
void display_refresh(const ui_state_t *s);

/* 基准点闪烁节拍: 由 100ms 任务周期调用, LDM 开启时翻转相位并重绘 */
void ui_state_blink_tick(ui_state_t *s);

#endif /* DISPLAY_DISPLAY_H_ */
