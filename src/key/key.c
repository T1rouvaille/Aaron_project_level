/*
 * key.c
 *
 *  按键检测实现 (4 键: SW1~SW4)
 *  四键均高有效 (按下接 VCC)
 *  消抖 20ms + 短按/长按状态机, 10ms 扫描周期
 */

#include "key/key.h"
#include "hal_data.h"
#include "config.h"

/* 按键引脚/检测参数集中定义于 config.h */

/* ======================================================================
 *  按键状态
 * ====================================================================== */
typedef struct
{
    bool         stable;          /* 消抖后稳定电平 (true=按下) */
    uint8_t      debounce_cnt;    /* 消抖计数 */
    bool         pressed;         /* 是否处于按下状态 */
    uint16_t     press_cnt;       /* 按下持续时间 (单位 x10ms) */
    bool         long_triggered;  /* 长按是否已触发 */
    bool         suppress_long;   /* 开机预置: 本次按下不触发长按 */
    key_event_t  pending;         /* 待消费事件 */
} key_slot_t;

static key_slot_t s_keys[KEY_NUM];

/* 按键引脚表 (与 key_id_t 顺序一致) */
static const bsp_io_port_pin_t s_key_pins[KEY_NUM] =
{
    KEY_SW1_PIN,
    KEY_SW2_PIN,
    KEY_SW3_PIN,
    KEY_SW4_PIN,
};

/* 按键按下电平极性表 (与 key_id_t 顺序一致) */
static const bsp_io_level_t s_key_pressed_level[KEY_NUM] =
{
    BSP_IO_LEVEL_HIGH,   /* SW1: 按下接 VCC, 高有效 */
    BSP_IO_LEVEL_HIGH,   /* SW2: 按下接 VCC, 高有效 */
    BSP_IO_LEVEL_HIGH,   /* SW3: 按下接 VCC, 高有效 */
    BSP_IO_LEVEL_HIGH,   /* SW4: 按下接 VCC, 高有效 */
};

/* 按键引脚配置表 (与 key_id_t 顺序一致) */
static const uint32_t s_key_pin_cfg[KEY_NUM] =
{
    KEY_PIN_INPUT_CFG,          /* SW1: 高有效, 需外部下拉 */
    KEY_PIN_INPUT_CFG,          /* SW2: 高有效, 需外部下拉 */
    KEY_PIN_INPUT_CFG,          /* SW3: 高有效, 需外部下拉 */
    KEY_PIN_INPUT_CFG,          /* SW4: 高有效, 需外部下拉 */
};

/* ----------------------------------------------------------------------
 *  读取指定按键原始电平 (高电平 = 按下)
 * ---------------------------------------------------------------------- */
static bool key_read_raw(key_id_t id)
{
    bsp_io_level_t level = BSP_IO_LEVEL_LOW;
    R_IOPORT_PinRead(&g_ioport_ctrl, s_key_pins[id], &level);
    return (s_key_pressed_level[id] == level);
}

/* 初始化按键引脚为输入 */
void key_init(void)
{
    uint8_t i;
    for (i = 0U; i < KEY_NUM; i++)
    {
        R_IOPORT_PinCfg(&g_ioport_ctrl, s_key_pins[i], s_key_pin_cfg[i]);
    }
}

/* 按键扫描: 消抖 + 长短按状态机 (10ms 周期) */
void key_scan(void)
{
    uint8_t i;

    for (i = 0U; i < KEY_NUM; i++)
    {
        key_slot_t *k = &s_keys[i];
        bool raw = key_read_raw((key_id_t)i);

        /* ---- 消抖: 电平变化后需连续 N 次一致才确认 ---- */
        if (raw != k->stable)
        {
            k->debounce_cnt++;
            if (k->debounce_cnt >= KEY_DEBOUNCE_CNT)
            {
                k->stable       = raw;
                k->debounce_cnt = 0U;
            }
        }
        else
        {
            k->debounce_cnt = 0U;
        }

        /* ---- 长按/短按状态机 (基于稳定电平) ---- */
        if (k->stable)
        {
            if (!k->pressed)
            {
                /* 按下沿 */
                k->pressed        = true;
                k->press_cnt      = 0U;
                k->long_triggered = false;
            }
            else
            {
                /* 按下保持: 计数达到阈值立即触发长按。
                 * 开机预置的键 (suppress_long) 本次按下不触发长按。 */
                if (!k->suppress_long && (k->press_cnt < KEY_LONG_PRESS_CNT))
                {
                    k->press_cnt++;
                    if (k->press_cnt >= KEY_LONG_PRESS_CNT)
                    {
                        k->long_triggered = true;
                        k->pending        = KEY_EVENT_LONG_PRESS;
                    }
                }
            }
        }
        else
        {
            if (k->pressed)
            {
                /* 释放沿: 未触发过长按则判为短按 */
                k->pressed       = false;
                k->suppress_long = false;   /* 松开后清除开机预置的抑制 */
                if (!k->long_triggered)
                {
                    k->pending = KEY_EVENT_SHORT_PRESS;
                }
                k->press_cnt = 0U;
            }
        }
    }
}

/* 读取并清除指定按键事件 */
key_event_t key_get_event(key_id_t id)
{
    key_event_t e;

    if (id >= KEY_NUM)
    {
        return KEY_EVENT_NONE;
    }

    e = s_keys[id].pending;
    s_keys[id].pending = KEY_EVENT_NONE;
    return e;
}

/* 查询按键当前是否处于按下状态 (原始电平, 供开机键检测) */
bool key_is_pressed(key_id_t id)
{
    if (id >= KEY_NUM)
    {
        return false;
    }
    return key_read_raw(id);
}

/* ----------------------------------------------------------------------
 *  预置某键为「已按下」状态 (开机键检测用)。
 *  开机键上电时已按住: 预置 stable=true + pressed=true, 松开时由 key_scan
 *  产生唯一一次短按事件, 避免与 boot 特判重复触发 (否则会 toggle 反相)。
 *  stable 预置为 true 可避免首次扫描在消抖完成前误判松开。
 * ---------------------------------------------------------------------- */
void key_preset_pressed(key_id_t id)
{
    if (id >= KEY_NUM)
    {
        return;
    }

    s_keys[id].stable         = true;
    s_keys[id].debounce_cnt   = 0U;
    s_keys[id].pressed        = true;
    s_keys[id].press_cnt      = 0U;
    s_keys[id].long_triggered = false;
    s_keys[id].suppress_long  = true;   /* 开机预置: 本次按下不触发长按关机 */
    s_keys[id].pending        = KEY_EVENT_NONE;
}
