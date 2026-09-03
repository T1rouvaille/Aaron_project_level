/*
 * bsp_debug_uart.h
 *
 *  通用 UART 底层收发驱动接口
 */

#ifndef _BSP_DEBUG_UART_H_
#define _BSP_DEBUG_UART_H_

#include "stdio.h"
#include "hal_data.h"
#include <stdbool.h>

void Debug_UART9_Init(void);
void debug_uart9_callback(uart_callback_args_t *p_args);

/* 阻塞发送字符串 */
void uart9_send_blocking(const char *msg);
/* 阻塞发送指定长度字节 */
void Debug_UART9_SendBlocking(const uint8_t *data, uint32_t length);

/* 串口接收一行 (数字或指令字符串, 以回车结束), 非阻塞 */
bool uart9_line_ready(void);
void uart9_get_line(char *out, uint8_t max);

#endif /* DEBUG_UART_BSP_DEBUG_UART_H_ */
