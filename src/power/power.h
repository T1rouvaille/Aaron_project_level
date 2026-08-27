/*
 * power.h
 *
 *  电源控制: LATCH 开机保持 + EN 7V/5V 上电时序 + 无操作自动关机状态机。
 *  引脚定义集中在 config.h (LATCH_PIN / EN_7V_PIN / EN_5V_PIN)。
 */

#ifndef POWER_POWER_H_
#define POWER_POWER_H_

#include <stdbool.h>

/* 电源状态机 */
typedef enum
{
    POWER_STATE_ON = 0,        /* 运行中 */
    POWER_STATE_OFF_PENDING,   /* 无操作超时, 待主循环执行关机 */
} power_state_t;

/*
 * LATCH 上电保持: 配置 P301 为输出并拉高开机。
 * 应在系统初始化最前面调用。
 */
void power_latch_init(void);

/*
 * EN 上电时序: 先拉高 EN 7V(P914), 延时 10ms, 再拉高 EN 5V(P915)。
 * 应在按键初始化完成后调用 (原需求指定)。
 */
void power_seq_init(void);

/*
 * LATCH 拉低关机 (P301 -> LOW)。
 */
void power_off(void);

/*
 * 自动关机状态机: 启动 AGT1 1s 定时器, 无按键操作累计 60s 后进入 OFF_PENDING。
 * 应在系统初始化完成后调用。
 */
void power_autooff_init(void);

/*
 * 有按键操作时调用, 重置自动关机状态机回 ON。
 */
void power_activity(void);

/*
 * 查询电源状态 (主循环轮询)。返回 POWER_STATE_OFF_PENDING 时应调用 power_off()。
 */
power_state_t power_get_state(void);

#endif /* POWER_POWER_H_ */
