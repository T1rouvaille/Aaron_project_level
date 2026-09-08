/*
 * power.h
 *
 *  电源控制: LATCH 开机保持 + EN 7V/5V 上电时序 + 无操作自动关机状态机。
 *  引脚定义集中在 config.h (LATCH_PIN / EN_7V_PIN / EN_5V_PIN)。
 */

#ifndef POWER_POWER_H_
#define POWER_POWER_H_

#include <stdbool.h>
#include <stdint.h>

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
 * 查询 LATCH 引脚当前输出电平是否为低 (关机状态)。
 * 供主循环检测关机动作, 统一触发清屏等收尾处理。
 */
bool power_latch_is_low(void);

/*
 * 外部按键输出 P002: 拉低到空闲态 (低电平)。
 * 应在 EN_5V 拉高之后调用。
 */
void power_key_out_init(void);

/*
 * 外部按键输出 P002: 启动一次非阻塞按键模拟。
 * 立即拉高 P002 (上升沿), 持续 duration_ms 后自动拉低释放。
 * 需主循环周期调用 power_key_out_poll() 推进状态机。
 * 若上一次输出尚未结束, 本次触发被忽略。
 */
void power_key_out_start(uint32_t duration_ms);

/*
 * 外部按键输出状态机轮询 (主循环 10ms 任务周期调用)。
 * 计时结束后拉低 P002 回到空闲态。
 */
void power_key_out_poll(void);

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

/*
 * 读取并清除 1s 节拍标志 (主循环调用)。
 * AGT1 每秒置位一次, 用于驱动时间统计等周期任务 (保持中断短小)。
 */
bool power_1s_tick_pending(void);

#endif /* POWER_POWER_H_ */
