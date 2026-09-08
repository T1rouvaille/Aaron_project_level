/*
 * command.c
 *
 *  串口命令行统一分发实现。
 *  所有串口收到的数据都进入 command_process_line() 这一个入口:
 *    1. 拆出命令名 + 参数;
 *    2. 查指令表 s_commands, 命中则调用对应 handler;
 *    3. 未命中且为纯数字串则按米值解析并更新 LCD。
 *
 *  扩展新指令只需两步:
 *    1. 写一个 static void cmd_xxx(const char *arg) 处理函数;
 *    2. 在 s_commands 表里加一行 { "XXX", cmd_xxx, "说明" }。
 */

#include "command.h"
#include "param/param.h"
#include "adc/adc.h"
#include "battery/battery.h"
#include "flash/flash.h"
#include "power/power.h"
#include "debug_uart/bsp_debug_uart.h"
#include "config.h"
#include <stdio.h>
#include <stdbool.h>

/* ----------------------------------------------------------------------
 *  指令处理函数签名: 入参为命令名之后的参数字符串 (可为空)
 * ---------------------------------------------------------------------- */
typedef void (*cmd_handler_t)(const char *arg);

typedef struct
{
    const char   *name;    /* 命令名 (大小写不敏感) */
    cmd_handler_t handler;
    const char   *help;    /* 帮助说明 (HELP 指令打印) */
} command_entry_t;

/* ---- 各指令处理函数 ---- */
static void cmd_read(const char *arg)
{
    (void)arg;
    param_print_all();
}

static void cmd_save(const char *arg)
{
    (void)arg;

    if (FSP_SUCCESS == param_save())
    {
        uart9_send_blocking("[CMD ] save OK\r\n");
    }
    else
    {
        uart9_send_blocking("[CMD ] save FAIL\r\n");
    }
}

static void cmd_batt(const char *arg)
{
    char buf[64];

    (void)arg;

    sprintf(buf, "[ADC ] battery raw = %u (%u bars), buck 7V raw = %u\r\n",
            (unsigned)adc_read_battery_raw(),
            (unsigned)battery_get_level(),
            (unsigned)adc_read_buck_7v_raw());
    uart9_send_blocking(buf);
}

static void cmd_clear_all_flash(const char *arg)
{
    (void)arg;

    /* 擦除整个 Data Flash, 重置参数为默认并写回 (恢复出厂) */
    if (FSP_SUCCESS == flash_erase(FLASH_DF_BASE_ADDR, FLASH_DF_TOTAL_BLOCKS))
    {
        param_set_defaults();

        if (FSP_SUCCESS == param_save())
        {
            uart9_send_blocking("[CMD ] clear all flash OK\r\n");
        }
        else
        {
            uart9_send_blocking("[CMD ] clear all flash FAIL (save)\r\n");
        }
    }
    else
    {
        uart9_send_blocking("[CMD ] clear all flash FAIL (erase)\r\n");
    }
}

/* 外部按键输出 P002: 短按模拟 (上升沿 + 120ms 高电平) */
static void cmd_key_short(const char *arg)
{
    (void)arg;

    uart9_send_blocking("[CMD ] key short press\r\n");
    power_key_out_start(KEY_OUT_SHORT_MS);
}

/* 外部按键输出 P002: 长按 3s 模拟 */
static void cmd_key_long3(const char *arg)
{
    (void)arg;

    uart9_send_blocking("[CMD ] key long press 3s\r\n");
    power_key_out_start(KEY_OUT_LONG3_MS);
}

/* 外部按键输出 P002: 长按 5s 模拟 */
static void cmd_key_long5(const char *arg)
{
    (void)arg;

    uart9_send_blocking("[CMD ] key long press 5s\r\n");
    power_key_out_start(KEY_OUT_LONG5_MS);
}

static void cmd_help(const char *arg);

/* ----------------------------------------------------------------------
 *  指令表 (扩展新指令: 在此加一行即可)
 * ---------------------------------------------------------------------- */
static const command_entry_t s_commands[] =
{
    { "AT+GET_ALL_INFO",    cmd_read,            "print all parameters"       },
    { "AT+SAVE",            cmd_save,            "save statistics to flash"   },
    { "AT+BATT",            cmd_batt,            "print battery/buck ADC raw" },
    { "AT+CLEAR_ALL_FLASH", cmd_clear_all_flash, "erase all data flash"       },
    { "AT+KEY_SHORT",       cmd_key_short,       "simulate ext key short press" },
    { "AT+KEY_LONG3",       cmd_key_long3,       "simulate ext key long press 3s" },
    { "AT+KEY_LONG5",       cmd_key_long5,       "simulate ext key long press 5s" },
    { "AT+HELP",            cmd_help,            "list all commands"          },
};

#define COMMAND_COUNT (sizeof(s_commands) / sizeof(s_commands[0]))

/* ----------------------------------------------------------------------
 *  严格解析米字符串 -> 毫米整数。
 *  仅接受 "纯数字 + 最多一个小数点" (如 "1.2564897"、"12"、"0.5")。
 *  含字母、多个小数点、其他符号、空串均视为非法, 返回 false。
 *  小数超过 3 位时截断到毫米精度。
 * ---------------------------------------------------------------------- */
static bool parse_meter_to_mm(const char *s, uint32_t *out_mm)
{
    uint32_t int_part    = 0U;
    uint32_t frac_part   = 0U;
    uint8_t  frac_digits = 0U;
    uint8_t  digit_count = 0U;
    bool     in_frac     = false;

    if ((NULL == s) || ('\0' == *s) || (NULL == out_mm))
    {
        return false;
    }

    while ('\0' != *s)
    {
        char c = *s;

        if ('.' == c)
        {
            if (in_frac)
            {
                /* 已在小数部分, 再遇小数点 -> 非法 */
                return false;
            }
            in_frac = true;
        }
        else if ((c >= '0') && (c <= '9'))
        {
            digit_count++;
            if (in_frac)
            {
                if (frac_digits < 3U)
                {
                    frac_part = frac_part * 10U + (uint32_t)(c - '0');
                    frac_digits++;
                }
            }
            else
            {
                int_part = int_part * 10U + (uint32_t)(c - '0');
            }
        }
        else
        {
            /* 字母/符号/空格等 -> 非法 */
            return false;
        }
        s++;
    }

    if (0U == digit_count)
    {
        /* 只有小数点或空 -> 非法 */
        return false;
    }

    while (frac_digits < 3U)
    {
        frac_part *= 10U;
        frac_digits++;
    }

    *out_mm = int_part * 1000U + frac_part;
    return true;
}

/* ----------------------------------------------------------------------
 *  工具: 大小写不敏感字符串比较 (返回 true 表示相等)
 * ---------------------------------------------------------------------- */
static bool cmd_name_match(const char *a, const char *b)
{
    while (('\0' != *a) && ('\0' != *b))
    {
        char ca = *a;
        char cb = *b;

        if ((ca >= 'a') && (ca <= 'z'))
        {
            ca = (char)(ca - 'a' + 'A');
        }
        if ((cb >= 'a') && (cb <= 'z'))
        {
            cb = (char)(cb - 'a' + 'A');
        }

        if (ca != cb)
        {
            return false;
        }
        a++;
        b++;
    }

    return ('\0' == *a) && ('\0' == *b);
}

/* ----------------------------------------------------------------------
 *  工具: 拆分行 -> 命令名 + 参数
 *    - 跳过行首空白, 命令名到第一个空白为止
 *    - 参数指向命令名后第一个非空白字符 (无参数时指向 '\0')
 * ---------------------------------------------------------------------- */
static void split_cmd(const char *line, char *cmd, uint8_t cmd_max, const char **arg)
{
    uint8_t i = 0U;

    /* 跳过行首空白 */
    while ((' ' == *line) || ('\t' == *line))
    {
        line++;
    }

    /* 提取命令名 */
    while (('\0' != *line) && (' ' != *line) && ('\t' != *line) && (i < (cmd_max - 1U)))
    {
        cmd[i] = *line;
        i++;
        line++;
    }
    cmd[i] = '\0';

    /* 跳过命令名后的空白, 得到参数 */
    while ((' ' == *line) || ('\t' == *line))
    {
        line++;
    }
    *arg = line;
}

/* ----------------------------------------------------------------------
 *  HELP 指令: 打印指令表 (引用了 s_commands, 故定义在表之后)
 * ---------------------------------------------------------------------- */
static void cmd_help(const char *arg)
{
    uint32_t i;
    char     buf[64];

    (void)arg;

    uart9_send_blocking("\r\nCommands:\r\n");
    for (i = 0U; i < COMMAND_COUNT; i++)
    {
        sprintf(buf, "  %-20s - %s\r\n", s_commands[i].name, s_commands[i].help);
        uart9_send_blocking(buf);
    }
}

/* ----------------------------------------------------------------------
 *  统一入口: 处理一行串口数据
 *    - 先查指令表, 命中则调用对应 handler
 *    - 未命中且为严格数字串, 则按米值解析并更新 LCD
 *    - 其余非法输入 (含字母/多小数点等) 静默忽略
 * ---------------------------------------------------------------------- */
void command_process_line(const char *line, ui_state_t *ui)
{
    char        cmd[32];
    const char *arg = NULL;
    uint32_t    i;
    uint32_t    mm;

    split_cmd(line, cmd, sizeof(cmd), &arg);

    for (i = 0U; i < COMMAND_COUNT; i++)
    {
        if (cmd_name_match(cmd, s_commands[i].name))
        {
            s_commands[i].handler(arg);
            return;
        }
    }

    /* 未命中指令: 仅当是严格数字串才作为米值处理 */
    if (parse_meter_to_mm(line, &mm))
    {
        ui_state_set_value(ui, mm);
    }
}
