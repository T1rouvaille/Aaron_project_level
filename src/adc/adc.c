/*
 * adc.c
 *
 *  ADC 模拟量采集实现: buck 7V 电压 + 电池电量 (原始值)。
 *  连续扫描 + 主循环 100ms 周期采样 (adc_sample_update) 读 ADDR 缓存。
 *  电量格数/关机判定业务逻辑已拆分至 battery 应用层模块。
 */

#include "adc.h"
#include "hal_data.h"
#include "config.h"
#include <assert.h>

/* 通道/阈值/回滞/防抖参数集中定义于 config.h */

/* 主循环 100ms 采样缓存的通道原始值 (adc_sample_update 写入) */
static volatile uint16_t s_buck_raw    = 0U;
static volatile uint16_t s_battery_raw = 0U;

void adc_init(void)
{
    fsp_err_t err = R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);
    assert(FSP_SUCCESS == err);

    err = R_ADC_ScanCfg(&g_adc0_ctrl, &g_adc0_channel_cfg);
    assert(FSP_SUCCESS == err);

    /* 软件触发启动连续扫描: 直接写 ADST=1 (Open 后 TRGE=0, 软件触发)。
     * 不用 R_ADC_ScanStart —— 它整体写入 scan_start_adcsr, 其中 TRGE 被
     * 驱动误置 1 (触发启动使能); 而 trigger=DISABLED 无触发源, TRGE=1 会让
     * ADC 卡在等待触发、扫描不启动、ADDR 恒 0 (读到 0 的根因)。 */
    R_ADC0->ADCSR_b.ADST = 1;

    /* 数据读取改为 100ms 主循环 (adc_sample_update), 不使能扫描结束中断 ADIE。
     * 连续扫描下扫描持续运行, ADDR 由硬件持续更新, 主循环 100ms 读一次即可。 */
}

uint16_t adc_read_buck_7v_raw(void)
{
    return s_buck_raw;
}

uint16_t adc_read_battery_raw(void)
{
    return s_battery_raw;
}

/* 滑动平均滤波: 最近 N 次采样取平均, 抑制瞬时跳变导致的电量格数抖动/关机误判。
 * 100ms 采样周期 -> 窗口 4 次 ≈ 0.4s 平滑。 */
#define ADC_FILTER_WINDOW   (4U)

static uint16_t s_buck_buf[ADC_FILTER_WINDOW];
static uint16_t s_batt_buf[ADC_FILTER_WINDOW];
static uint8_t  s_buck_idx  = 0U;
static uint8_t  s_batt_idx  = 0U;
static uint8_t  s_buck_cnt  = 0U;   /* 已累计采样数 (≤ 窗口, 未满时按实际数平均) */
static uint8_t  s_batt_cnt  = 0U;

/* 滑动平均: 写入新值到环形缓冲, 返回最近 cnt 次的平均值。 */
static uint16_t adc_slide_avg(uint16_t *buf, uint16_t new_val,
                              uint8_t *idx, uint8_t *cnt)
{
    uint32_t sum = 0U;
    uint8_t  i;

    buf[*idx] = new_val;
    *idx = (uint8_t)((*idx + 1U) % ADC_FILTER_WINDOW);

    if (*cnt < ADC_FILTER_WINDOW)
    {
        (*cnt)++;
    }

    for (i = 0U; i < *cnt; i++)
    {
        sum += buf[i];
    }

    return (uint16_t)(sum / *cnt);
}

/* 100ms 周期采样: 直接读 ADDR 更新 buck 7V + 电池原始值缓存。
 * 主循环调用, 避免在扫描完成中断里读 ADDR (连续扫描下与硬件写/读后清零竞争)。 */
void adc_sample_update(void)
{
    uint16_t val;

    if (FSP_SUCCESS == R_ADC_Read(&g_adc0_ctrl, ADC_CH_BUCK_7V, &val))
    {
        s_buck_raw = adc_slide_avg(s_buck_buf, val, &s_buck_idx, &s_buck_cnt);
    }

    if (FSP_SUCCESS == R_ADC_Read(&g_adc0_ctrl, ADC_CH_BATTERY, &val))
    {
        s_battery_raw = adc_slide_avg(s_batt_buf, val, &s_batt_idx, &s_batt_cnt);
    }
}

/* ----------------------------------------------------------------------
 *  ADC 扫描完成回调: 数据读取已移至主循环 adc_sample_update() (100ms)。
 *  当前未使能扫描结束中断 ADIE, 此回调不会被调用; 保留仅为兼容
 *  hal_data.c 的 p_callback 引用。若将来重新使能 ADIE, 此处仅清 ADF。
 * ---------------------------------------------------------------------- */
void adc_callback(adc_callback_args_t *p_args)
{
    if (ADC_EVENT_SCAN_COMPLETE != p_args->event)
    {
        return;
    }

    /* 清除扫描结束标志 ADF (bit0): FSP r_adc 的扫描结束 ISR 只清 BSP IRQ 标志,
     * 未清 ADF。按手册: 读 ADF 为 1 后写 0 清除。 */
    if (1U == R_ADC0->ADREF_b.ADF)
    {
        R_ADC0->ADREF_b.ADF = 0U;
    }
}
