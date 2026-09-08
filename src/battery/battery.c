/*
 * battery.c
 *
 *  电源监控应用逻辑实现: 电量格数回滞 + 低电关机防抖 + BUCK 失效判定。
 *  依赖 adc 驱动层原始值 (adc_read_battery_raw / adc_read_buck_7v_raw),
 *  阈值见 config.h。
 */

#include "battery/battery.h"
#include "adc/adc.h"
#include "config.h"

/* 电池检测状态 */
static uint8_t s_batt_level = 3U;   /* 当前电量格数 (1~3) */
static uint8_t s_low_cnt    = 0U;   /* 低电连续计数 */
static bool   s_batt_low    = false;/* 当前是否低于关机阈值 (供低电时长统计) */

/* BUCK 失效检测状态 */
static uint8_t s_buck_fail_cnt = 0U;/* BUCK 连续超范围计数 */

/* ----------------------------------------------------------------------
 *  BUCK 7V 失效检测: 连续超范围 (过高/过低) 计数, 达阈值判失效。
 * ---------------------------------------------------------------------- */
static void buck_update(void)
{
    uint16_t raw = adc_read_buck_7v_raw();

    if ((raw < ADC_BUCK_RAW_MIN) || (raw > ADC_BUCK_RAW_MAX))
    {
        if (s_buck_fail_cnt < ADC_BUCK_FAIL_CNT_MAX)
        {
            s_buck_fail_cnt++;
        }
    }
    else
    {
        s_buck_fail_cnt = 0U;
    }
}

void battery_update(void)
{
    uint16_t raw = adc_read_battery_raw();

    /* 电量格数回滞判定 (3 <-> 2 <-> 1) */
    switch (s_batt_level)
    {
        case 3U:
            if (raw < ADC_BATT_3TO2_RAW)
            {
                s_batt_level = 2U;   /* 3 -> 2 下降 */
            }
            break;

        case 2U:
            if (raw < ADC_BATT_2TO1_RAW)
            {
                s_batt_level = 1U;   /* 2 -> 1 下降 */
            }
            else if (raw >= (ADC_BATT_3TO2_RAW + ADC_BATT_HYST))
            {
                s_batt_level = 3U;   /* 2 -> 3 上升 */
            }
            break;

        case 1U:
        default:
            if (raw >= (ADC_BATT_2TO1_RAW + ADC_BATT_HYST))
            {
                s_batt_level = 2U;   /* 1 -> 2 上升 */
            }
            break;
    }

    /* 低电关机防抖: 连续低于关机阈值计数 */
    if (raw < ADC_BATT_OFF_RAW)
    {
        if (s_low_cnt < ADC_BATT_LOW_CNT_MAX)
        {
            s_low_cnt++;
        }
    }
    else
    {
        s_low_cnt = 0U;
    }

    /* 当前低电状态 (供 1s 低电时长统计) */
    s_batt_low = (raw < ADC_BATT_OFF_RAW);

    /* BUCK 7V 失效检测 */
    buck_update();
}

uint8_t battery_get_level(void)
{
    return s_batt_level;
}

bool battery_is_low(void)
{
    return s_batt_low;
}

bool battery_need_shutdown(void)
{
    return s_low_cnt >= ADC_BATT_LOW_CNT_MAX;
}

bool battery_buck_need_shutdown(void)
{
    return s_buck_fail_cnt >= ADC_BUCK_FAIL_CNT_MAX;
}
