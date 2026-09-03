/*
 * command.h
 *
 *  串口命令行统一分发: 指令表驱动, 收到一行数据后解析命令名并分派。
 *  未命中指令时按米值解析 (历史行为), 便于后续扩展新指令 (查表加一行)。
 */

#ifndef COMMAND_COMMAND_H_
#define COMMAND_COMMAND_H_

#include "display/display.h"

/* 统一处理一行串口数据 (指令分发 + 默认米值解析) */
void command_process_line(const char *line, ui_state_t *ui);

#endif /* COMMAND_COMMAND_H_ */
