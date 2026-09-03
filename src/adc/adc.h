/*
 * adc.h
 *
 *  ADC 模拟量采集: buck 7V 电压检测 + 电池电量检测 (原始值)。
 *  通道映射 (RA2E1A73):
 *    P014 -> AN009 (ADC_CHANNEL_9)   : buck 7V 电压
 *    P015 -> AN010 (ADC_CHANNEL_10)  : 电池电压
 *  依赖 FSP 已配置的 ADC0 实例 (连续扫描, 12bit, 右对齐, AVCC0 3.3V 参考)。
 */

#ifndef ADC_ADC_H_
#define ADC_ADC_H_

#include <stdint.h>
#include <stdbool.h>

/* ADC 初始化: Open + ScanCfg + ScanStart (连续扫描, 软件触发) */
void adc_init(void);

/* 100ms 周期采样: 读 buck 7V + 电池原始值到缓存 (主循环调用) */
void adc_sample_update(void);

/* 读取 buck 7V 检测原始值 (P014 -> ADC_CHANNEL_9, 0~4095) */
uint16_t adc_read_buck_7v_raw(void);

/* 读取电池电量检测原始值 (P015 -> ADC_CHANNEL_10, 0~4095) */
uint16_t adc_read_battery_raw(void);

/* 电池检测任务 (100ms 周期调用): 回滞更新电量格数 + 低电关机防抖计数 */
void adc_battery_update(void);

/* 获取当前电量格数 (1~3) */
uint8_t adc_get_battery_level(void);

/* 当前是否低于关机阈值 (供低电时长统计) */
bool adc_battery_is_low(void);

/* 是否满足低电关机条件 (连续 3 次低于关机阈值) */
bool adc_battery_need_shutdown(void);

#endif /* ADC_ADC_H_ */
