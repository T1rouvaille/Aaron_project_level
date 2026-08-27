/*
 * gpt.c
 *
 *  1ms 心跳定时器 (GPT9) 与周期任务标志位
 */

#include "gpt/gpt.h"
#include <assert.h>

volatile uint32_t g_sys_tick_ms = 0U;   /* 系统毫秒计数 */
volatile uint32_t g_task_flags = 0U;    /* 任务周期标志位 */

/* GPT9 中断回调: 每 1ms 触发一次 */
void GPT9_CALLBACK(timer_callback_args_t *p_args)
{
    uint32_t tick;

    if (TIMER_EVENT_CYCLE_END != p_args->event)
    {
        return;
    }

    g_sys_tick_ms++;
    tick = g_sys_tick_ms;

    /* 置 1ms 标志位 */
    g_task_flags |= TASK_FLAG_1MS;

    /* 软件分频产生各周期标志位 */
    if (0U == (tick % 10U))
    {
        g_task_flags |= TASK_FLAG_10MS;
    }
    if (0U == (tick % 20U))
    {
        g_task_flags |= TASK_FLAG_20MS;
    }
    if (0U == (tick % 50U))
    {
        g_task_flags |= TASK_FLAG_50MS;
    }
    if (0U == (tick % 100U))
    {
        g_task_flags |= TASK_FLAG_100MS;
    }
    if (0U == (tick % 1000U))
    {
        g_task_flags |= TASK_FLAG_1000MS;
    }
}

/* 初始化 1ms 心跳定时器 */
void gpt_init(void)
{
    fsp_err_t err = R_GPT_Open(&g_timer9_ctrl, &g_timer9_cfg);
    assert(FSP_SUCCESS == err);

    err = R_GPT_Start(&g_timer9_ctrl);
    assert(FSP_SUCCESS == err);
}

/* 清除指定任务标志位 (关中断保证读-改-写原子性) */
void gpt_flag_clear(uint32_t mask)
{
    __disable_irq();
    g_task_flags &= ~mask;
    __enable_irq();
}
