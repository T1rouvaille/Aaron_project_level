#ifndef GPT_GPT_H_
#define GPT_GPT_H_

#include "hal_data.h"

/* ======================================================================
 *  任务周期标志位定义 (位掩码)
 *  由 GPT9 1ms 中断置位, 主循环消费后调用 gpt_flag_clear() 清除
 * ====================================================================== */
#define TASK_FLAG_1MS      (1UL << 0)   /* 1ms    任务 */
#define TASK_FLAG_10MS     (1UL << 1)   /* 10ms   任务 */
#define TASK_FLAG_20MS     (1UL << 2)   /* 20ms   任务 */
#define TASK_FLAG_50MS     (1UL << 3)   /* 50ms   任务 */
#define TASK_FLAG_100MS    (1UL << 4)   /* 100ms  任务 */
#define TASK_FLAG_1000MS   (1UL << 5)   /* 1000ms 任务 */

/* 系统心跳毫秒计数 (上电以来累计) */
extern volatile uint32_t g_sys_tick_ms;

/* 任务周期标志寄存器 (中断置位, 主循环消费) */
extern volatile uint32_t g_task_flags;

/* 初始化 1ms 心跳定时器 (GPT9) */
void gpt_init(void);

/* 清除指定任务标志位 (带临界区保护) */
void gpt_flag_clear(uint32_t mask);

#endif /* GPT_GPT_H_ */
