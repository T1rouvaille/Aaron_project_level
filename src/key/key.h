/*
 * key.h
 *
 *  按键检测接口 (4 键: SW1~SW4)
 *  四键均高有效 (按下接 VCC)
 *  消抖 + 短按/长按状态机, 需 10ms 周期调用 key_scan()
 */

#ifndef KEY_KEY_H_
#define KEY_KEY_H_

#include <stdbool.h>

/* 按键 ID */
typedef enum
{
    KEY_ID_SW1 = 0,   /* SW1: P913 */
    KEY_ID_SW2,       /* SW2: P407 */
    KEY_ID_SW3,       /* SW3: P408 */
    KEY_ID_SW4,       /* SW4: P409 */
    KEY_NUM           /* 按键总数 */
} key_id_t;

/* 按键事件 */
typedef enum
{
    KEY_EVENT_NONE        = 0,  /* 无事件 */
    KEY_EVENT_SHORT_PRESS,      /* 短按 */
    KEY_EVENT_LONG_PRESS        /* 长按 */
} key_event_t;

/* 初始化按键引脚 (输入, 四键均需外部下拉保证默认低) */
void key_init(void);

/* 按键扫描 (每 10ms 调用一次: 消抖 + 长短按检测) */
void key_scan(void);

/* 读取并清除指定按键的事件 (无事件返回 KEY_EVENT_NONE) */
key_event_t key_get_event(key_id_t id);

/* 查询按键当前是否处于按下状态 (原始电平, 供开机键检测) */
bool key_is_pressed(key_id_t id);

/* 预置某键为「已按下」状态 (开机键检测用): 后续松开由 key_scan 产生唯一一次短按事件 */
void key_preset_pressed(key_id_t id);

#endif /* KEY_KEY_H_ */
