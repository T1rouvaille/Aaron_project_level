/*
 * param.c
 *
 *  系统参数存储实现: 基础配置 + 运行时统计 -> Data Flash。
 *  详见 param.h 说明。
 */

#include "param/param.h"
#include "flash/flash.h"
#include "config.h"
#include "debug_uart/bsp_debug_uart.h"
#include <string.h>
#include <stdio.h>

/* ======================================================================
 *  存储地址 (Data Flash 基地址 + 偏移)
 * ====================================================================== */
#define PARAM_FLASH_ADDR   (FLASH_DF_BASE_ADDR + PARAM_FLASH_OFFSET)

/* 结构体版本 */
#define PARAM_STRUCT_VER   (1U)

/* ======================================================================
 *  运行时参数实例 (RAM 镜像)
 * ====================================================================== */
static param_t s_param;

/* ======================================================================
 *  默认值填充
 * ====================================================================== */
void param_set_defaults(void)
{
    memset(&s_param, 0, sizeof(s_param));

    s_param.magic      = PARAM_MAGIC;
    s_param.struct_ver = PARAM_STRUCT_VER;

    strncpy(s_param.project_name, PARAM_PROJECT_NAME, sizeof(s_param.project_name) - 1U);
    strncpy(s_param.part_number,  PARAM_PART_NUMBER,  sizeof(s_param.part_number)  - 1U);
    strncpy(s_param.sw_version,   PARAM_SW_VERSION,   sizeof(s_param.sw_version)   - 1U);
}

/* ======================================================================
 *  Flash 读写
 * ====================================================================== */
fsp_err_t param_load(void)
{
    fsp_err_t err = flash_read(PARAM_FLASH_ADDR, &s_param, sizeof(s_param));
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 校验标记: 无效表示 Flash 未初始化或数据损坏 */
    if (PARAM_MAGIC != s_param.magic)
    {
        return FSP_ERR_NOT_FOUND;
    }

    return FSP_SUCCESS;
}

fsp_err_t param_save(void)
{
    fsp_err_t err;

    /* 写入前先擦除整块区域 */
    err = flash_erase(PARAM_FLASH_ADDR, PARAM_FLASH_BLOCKS);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 整体写入 (sizeof 为 4 字节对齐) */
    return flash_write(PARAM_FLASH_ADDR, &s_param, sizeof(s_param));
}

void param_init(void)
{
    /* 读回失败则写入默认配置 (首次上电 / Flash 空) */
    if (FSP_SUCCESS != param_load())
    {
        param_set_defaults();
        (void)param_save();
    }
}

/* ======================================================================
 *  运行时统计 (RAM 累加)
 * ====================================================================== */
void param_key_event(uint8_t key_id, bool is_long)
{
    if (key_id >= 4U)
    {
        return;
    }

    if (is_long)
    {
        s_param.key_long_cnt[key_id]++;
    }
    else
    {
        s_param.key_short_cnt[key_id]++;
    }
}

void param_tick_1s(bool low_batt)
{
    s_param.total_run_sec++;

    if (low_batt)
    {
        s_param.low_batt_sec++;
    }
}

void param_set_shutdown_reason(shutdown_reason_t reason)
{
    if ((reason >= SHUTDOWN_REASON_NUM) || (reason <= SHUTDOWN_NONE))
    {
        return;
    }

    s_param.last_reason = (uint32_t)reason;
    s_param.reason_cnt[reason]++;
}

void param_add_time_slot(uint8_t idx, uint32_t sec)
{
    if (idx >= PARAM_TIME_SLOT_NUM)
    {
        return;
    }

    s_param.time_slots[idx] += sec;
}

const param_t *param_get(void)
{
    return &s_param;
}

/* ======================================================================
 *  参数打印
 * ====================================================================== */

/* 关机原因 -> 字符串 */
static const char *reason_to_str(shutdown_reason_t r)
{
    switch (r)
    {
        case SHUTDOWN_MANUAL:   return "MANUAL";
        case SHUTDOWN_LOW_BATT: return "LOW_BATT";
        case SHUTDOWN_I2C_FAIL: return "I2C_FAIL";
        case SHUTDOWN_BUCK_FAIL:return "BUCK_FAIL";
        default:                return "NONE";
    }
}

void param_print_all(void)
{
    char     buf[96];
    uint8_t  i;

    uart9_send_blocking("\r\n========================================\r\n");
    uart9_send_blocking("  System Parameters\r\n");
    uart9_send_blocking("========================================\r\n");

    sprintf(buf, "Project     : %s\r\n", s_param.project_name);
    uart9_send_blocking(buf);
    sprintf(buf, "Part Number : %s\r\n", s_param.part_number);
    uart9_send_blocking(buf);
    sprintf(buf, "SW Version  : %s\r\n", s_param.sw_version);
    uart9_send_blocking(buf);

    uart9_send_blocking("---- Key Count (short/long) ----\r\n");
    for (i = 0U; i < 4U; i++)
    {
        sprintf(buf, "SW%u: short=%lu long=%lu\r\n", (unsigned)(i + 1U),
                (unsigned long)s_param.key_short_cnt[i],
                (unsigned long)s_param.key_long_cnt[i]);
        uart9_send_blocking(buf);
    }

    uart9_send_blocking("---- Time ----\r\n");
    sprintf(buf, "Total Run : %luh %lum %lus\r\n",
            (unsigned long)(s_param.total_run_sec / 3600U),
            (unsigned long)((s_param.total_run_sec % 3600U) / 60U),
            (unsigned long)(s_param.total_run_sec % 60U));
    uart9_send_blocking(buf);
    sprintf(buf, "Low Batt  : %lu s\r\n", (unsigned long)s_param.low_batt_sec);
    uart9_send_blocking(buf);
    for (i = 0U; i < PARAM_TIME_SLOT_NUM; i++)
    {
        sprintf(buf, "Slot[%u]   : %lu\r\n", (unsigned)i,
                (unsigned long)s_param.time_slots[i]);
        uart9_send_blocking(buf);
    }

    uart9_send_blocking("---- Shutdown Reason ----\r\n");
    sprintf(buf, "Last      : %s\r\n",
            reason_to_str((shutdown_reason_t)s_param.last_reason));
    uart9_send_blocking(buf);
    for (i = 1U; i < (uint8_t)SHUTDOWN_REASON_NUM; i++)
    {
        sprintf(buf, "%s: %lu\r\n", reason_to_str((shutdown_reason_t)i),
                (unsigned long)s_param.reason_cnt[i]);
        uart9_send_blocking(buf);
    }

    uart9_send_blocking("========================================\r\n");
}
