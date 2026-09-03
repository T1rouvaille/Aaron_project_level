/*
 * bsp_debug_uart.c
 *
 *  通用 UART 底层收发驱动实现
 */

#include "hal_data.h"
#include "stdio.h"
#include <stdbool.h>
#include "debug_uart/bsp_debug_uart.h"

#define RX_LINE_MAX             (32U)   /* 一行最大字符数 (数字/指令) */
#define UART_TX_WAIT_TIMEOUT_LOOP   (2000000UL)

static volatile uint8_t s_rx_char;                   /* 单字节接收缓冲 */
static volatile char    s_rx_line[RX_LINE_MAX + 1U]; /* 行缓冲 (数字/指令字符串) */
static volatile uint8_t s_rx_line_len = 0U;          /* 当前行长度 */
static volatile bool    s_rx_line_ready = false;     /* 行接收完成标志 */
static volatile bool    uart_send_complete_flag = true;

/* 串口初始化 */
void Debug_UART9_Init(void)
{
    fsp_err_t err = R_SCI_UART_Open(&g_uart9_ctrl, &g_uart9_cfg);
    assert(FSP_SUCCESS == err);
    err = R_SCI_UART_Read(&g_uart9_ctrl, (uint8_t *)&s_rx_char, 1U);
    assert(FSP_SUCCESS == err);
}

/* 串口回调 */
void debug_uart9_callback(uart_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_RX_COMPLETE:
        {
            /* 单字节接收完成: 累积数字字符, 回车结束一行 */
            uint8_t c = s_rx_char;

            if (('\r' == c) || ('\n' == c))
            {
                /* 回车: 已有数字则标记一行完成 */
                if (s_rx_line_len > 0U)
                {
                    s_rx_line[s_rx_line_len] = '\0';
                    s_rx_line_ready = true;
                }
            }
            else if ((c >= 0x20U) && (c <= 0x7EU))
            {
                /* 可打印 ASCII 字符 (数字/字母/符号): 累积 (超长丢弃) */
                if (s_rx_line_len < RX_LINE_MAX)
                {
                    s_rx_line[s_rx_line_len] = (char)c;
                    s_rx_line_len++;
                }
            }

            /* 未完成则继续接收下一字节 */
            if (!s_rx_line_ready)
            {
                R_SCI_UART_Read(&g_uart9_ctrl, (uint8_t *)&s_rx_char, 1U);
            }
            break;
        }

        case UART_EVENT_TX_COMPLETE:
        {
            uart_send_complete_flag = true;
            break;
        }

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

/* 是否收到完整一行 (数字字符串, 回车结束) */
bool uart9_line_ready(void)
{
    return s_rx_line_ready;
}

/* 取走一行并复位, 重新开始接收 */
void uart9_get_line(char *out, uint8_t max)
{
    uint8_t i;
    uint8_t n;

    if ((NULL == out) || (0U == max))
    {
        return;
    }

    n = s_rx_line_len;
    if (n >= max)
    {
        n = (uint8_t)(max - 1U);
    }

    for (i = 0U; i < n; i++)
    {
        out[i] = s_rx_line[i];
    }
    out[n] = '\0';

    /* 复位并重新开始接收 */
    s_rx_line_len = 0U;
    s_rx_line_ready = false;
    R_SCI_UART_Read(&g_uart9_ctrl, (uint8_t *)&s_rx_char, 1U);
}
