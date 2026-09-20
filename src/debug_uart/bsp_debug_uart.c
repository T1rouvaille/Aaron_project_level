/*
 * bsp_debug_uart.c
 *
 *  通用 UART 底层收发驱动实现
 */

#include "hal_data.h"
#include "stdio.h"
#include <stdbool.h>
#include "debug_uart/bsp_debug_uart.h"

#define RX_RING_SIZE            (128U)  /* 接收环形缓冲大小 (2 的幂) */
#define UART_TX_WAIT_TIMEOUT_LOOP   (2000000UL)

static volatile uint8_t s_rx_char;                   /* 单字节接收目标 (每次 Read 1 字节) */
static volatile uint8_t s_rx_ring[RX_RING_SIZE];     /* 接收环形缓冲 (中断写入) */
static volatile uint8_t s_rx_head = 0U;              /* 环形缓冲写指针 (中断推进) */
static volatile uint8_t s_rx_tail = 0U;              /* 环形缓冲读指针 (主循环推进) */
static volatile bool    uart_send_complete_flag = true;

/* 串口初始化 */
void Debug_UART9_Init(void)
{
    fsp_err_t err = R_SCI_UART_Open(&g_uart9_ctrl, &g_uart9_cfg);
    assert(FSP_SUCCESS == err);
    err = R_SCI_UART_Read(&g_uart9_ctrl, (uint8_t *)&s_rx_char, 1U);
    assert(FSP_SUCCESS == err);
}

/* ----------------------------------------------------------------------
 *  接收一个字节: 压入环形缓冲 (满则丢弃), 并重新武装下一次接收。
 *  关键: 每收到一字节都立即重新 Read, 避免「一行就绪即停」造成接收空窗,
 *        否则外部 MCU 持续回传测量数据时会触发溢出 (overrun) 导致接收卡死。
 * ---------------------------------------------------------------------- */
static void uart_rx_push(uint8_t c)
{
    uint8_t next = (uint8_t)((s_rx_head + 1U) % RX_RING_SIZE);

    if (next != s_rx_tail)      /* 缓冲未满才写入, 满则丢弃该字节 */
    {
        s_rx_ring[s_rx_head] = c;
        s_rx_head            = next;
    }

    (void)R_SCI_UART_Read(&g_uart9_ctrl, (uint8_t *)&s_rx_char, 1U);
}

/* 串口回调 */
void debug_uart9_callback(uart_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_RX_COMPLETE:    /* 单字节接收完成 (正常路径) */
            uart_rx_push(s_rx_char);
            break;

        case UART_EVENT_RX_CHAR:        /* 无未决 Read 时直接给出字符 (兜底) */
            uart_rx_push((uint8_t)p_args->data);
            break;

        case UART_EVENT_TX_COMPLETE:
            uart_send_complete_flag = true;
            break;

        /* 接收错误 (过载/帧/校验): 丢弃错误字节并重新武装接收, 兜底避免 RX 永久卡死 */
        case UART_EVENT_ERR_OVERFLOW:
        case UART_EVENT_ERR_FRAMING:
        case UART_EVENT_ERR_PARITY:
            (void)R_SCI_UART_Read(&g_uart9_ctrl, (uint8_t *)&s_rx_char, 1U);
            break;

        default:
            break;
    }
}

/* 阻塞发送字符串 */
void uart9_send_blocking(const char *msg)
{
    if ((NULL == msg) || ('\0' == msg[0]))
    {
        return;
    }

    Debug_UART9_SendBlocking((const uint8_t *)msg, (uint32_t)strlen(msg));
}

/* 阻塞发送指定长度字节 */
void Debug_UART9_SendBlocking(const uint8_t *data, uint32_t length)
{
    if ((NULL == data) || (0U == length))
    {
        return;
    }

    uart_send_complete_flag = false;
    R_SCI_UART_Write(&g_uart9_ctrl, (uint8_t *)data, length);
    uint32_t timeout = UART_TX_WAIT_TIMEOUT_LOOP;
    while ((!uart_send_complete_flag) && (timeout > 0U))
    {
        timeout--;
        __NOP();
    }
}

/* 环形缓冲中是否已含一个完整行 (以 '\n' 或 '\r' 结尾) */
bool uart9_line_ready(void)
{
    uint8_t i = s_rx_tail;

    while (i != s_rx_head)
    {
        uint8_t c = s_rx_ring[i];

        if (('\n' == c) || ('\r' == c))
        {
            return true;
        }

        i = (uint8_t)((i + 1U) % RX_RING_SIZE);
    }

    return false;
}

/* 从环形缓冲取走一行 (去掉换行符), 超出 max-1 的字符丢弃 */
void uart9_get_line(char *out, uint8_t max)
{
    uint8_t n = 0U;

    if ((NULL == out) || (0U == max))
    {
        return;
    }

    while (s_rx_tail != s_rx_head)
    {
        uint8_t c = s_rx_ring[s_rx_tail];

        /* 先消费该字节 (读指针推进) */
        s_rx_tail = (uint8_t)((s_rx_tail + 1U) % RX_RING_SIZE);

        if (('\n' == c) || ('\r' == c))
        {
            break;      /* 行结束 */
        }

        if (n < (max - 1U))
        {
            out[n] = (char)c;
            n++;
        }
    }

    out[n] = '\0';
}
