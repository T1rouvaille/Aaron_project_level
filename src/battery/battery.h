/*
 * battery.h
 *
 *  电源监控应用逻辑: 依赖 adc 驱动层的原始采样值, 完成
 *    电池电量格数回滞 + 低电关机防抖 + BUCK 7V 失效判定。
 *
 *  本层只做「业务判定」, 不直接访问 ADC 硬件;
 *  原始值通过 adc_read_battery_raw()/adc_read_buck_7v_raw() 获取,
 *  阈值集中定义于 config.h。
 */

#ifndef BATTERY_BATTERY_H_
#define BATTERY_BATTERY_H_

#include <stdint.h>
#include <stdbool.h>

/* 电源监控任务 (100ms 周期调用): 更新电量格数 + 低电防抖 + BUCK 失效防抖。
 *   ldm_on: LDM 测量是否开启 (SW3);  t7_on: T7 十字加号是否显示 (SW4)。
 *   阈值随负载状态动态调整 (见 config.h 电池阈值说明)。 */
void battery_update(bool ldm_on, bool t7_on);

/* 获取当前电量格数 (1~3) */
uint8_t battery_get_level(void);

/* 当前是否低于关机阈值 (供低电时长统计) */
bool battery_is_low(void);

/* 是否满足低电关机条件 (连续 N 次低于关机阈值) */
bool battery_need_shutdown(void);

/* BUCK 7V 是否失效 (连续 N 次超出正常范围) */
bool battery_buck_need_shutdown(void);

#endif /* BATTERY_BATTERY_H_ */
