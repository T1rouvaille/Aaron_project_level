/*
 * param.h
 *
 *  系统参数存储: 基础配置 + 运行时统计, 持久化到 Data Flash。
 *
 *  存储内容:
 *    - 基础信息: 项目名称 / Part Number / 软件版本
 *    - 按键统计: SW1~SW4 短按/长按次数
 *    - 时间统计: 累计运行时长 / 低电时长 / 预留 10 个时间槽
 *    - 关机原因: 最近一次关机原因 + 各原因累计次数
 *
 *  写回策略:
 *    - 统计数据在 RAM 中累加, 关机时调用 param_save() 整体写回 Flash
 *      (Data Flash 擦写寿命有限, 避免频繁写)。
 *    - 开机时 param_init() 读回并校验, 首次上电写入默认配置。
 *
 *  串口指令 (由 command 模块统一分发):
 *    "AT+GET_ALL_INFO" 读出全部参数; "AT+SAVE" 将 RAM 统计立即写回 Flash。
 */

#ifndef PARAM_PARAM_H_
#define PARAM_PARAM_H_

#include <stdint.h>
#include <stdbool.h>
#include "hal_data.h"

/* ======================================================================
 *  结构体标识
 * ====================================================================== */
#define PARAM_MAGIC          (0x4C444D31UL)   /* "LDM1" 校验标记 */

/* 时间槽数量 (预留) */
#define PARAM_TIME_SLOT_NUM  (10U)

/* 时间槽用途: 前 4 个用于 LDM/laser 分时状态统计 (单位: 秒), 剩余 6 个预留。
 * 4 态互斥穷尽: 每秒按当前 LDM/laser 开关组合累加其一。 */
typedef enum
{
    TIME_SLOT_LDM_ONLY = 0,   /* LDM 单独开启 (ldm_on && !t7) */
    TIME_SLOT_LASER_ONLY,     /* laser 单独开启 (!ldm_on && t7) */
    TIME_SLOT_BOTH_ON,        /* LDM + laser 同时开启 (ldm_on && t7) */
    TIME_SLOT_NEITHER,        /* 都不开启 (!ldm_on && !t7) */
    TIME_SLOT_STATE_NUM,      /* 分时状态槽数量 (4) */
} time_slot_id_t;

/* ======================================================================
 *  关机原因枚举
 * ====================================================================== */
typedef enum
{
    SHUTDOWN_NONE = 0,     /* 无 / 未关机 */
    SHUTDOWN_MANUAL,       /* 人为关机 */
    SHUTDOWN_LOW_BATT,     /* 低电量关机 */
    SHUTDOWN_I2C_FAIL,     /* I2C 通讯失败关机 */
    SHUTDOWN_BUCK_FAIL,    /* BUCK 电路失效关机 */
    SHUTDOWN_REASON_NUM,   /* 原因总数 */
} shutdown_reason_t;

/* ======================================================================
 *  参数结构体 (整体写入 Data Flash, 需 4 字节对齐)
 * ====================================================================== */
typedef struct
{
    uint32_t magic;                          /* 校验标记 */
    uint32_t struct_ver;                     /* 结构体版本 */

    /* 基础信息 */
    char     project_name[24];               /* 项目名称 */
    char     part_number[16];                /* Part Number */
    char     sw_version[16];                 /* 软件版本 */

    /* 按键统计 (SW1~SW4) */
    uint32_t key_short_cnt[4];               /* 短按次数 */
    uint32_t key_long_cnt[4];                /* 长按次数 */

    /* 时间统计 (单位: 秒) */
    uint32_t total_run_sec;                  /* 累计运行时长 */
    uint32_t low_batt_sec;                   /* 低电量累计时长 */
    uint32_t time_slots[PARAM_TIME_SLOT_NUM];/* 预留时间槽 */

    /* 关机原因 */
    uint32_t last_reason;                    /* 最近一次关机原因 */
    uint32_t reason_cnt[SHUTDOWN_REASON_NUM];/* 各原因累计次数 */

    /* 单位记忆 */
    uint32_t last_digital;                   /* 上次显示单位 (digital_t 值 0~6) */
} param_t;

/* ======================================================================
 *  接口
 * ====================================================================== */

/* 开机加载: 读 Flash -> 校验, 无效则写入默认配置 (需 flash 已初始化) */
void param_init(void);

/* 从 Flash 读入 RAM (校验失败返回错误) */
fsp_err_t param_load(void);

/* 将 RAM 写回 Flash (擦除 + 写入) */
fsp_err_t param_save(void);

/* 填充默认配置 (基础信息 + 统计清零) */
void param_set_defaults(void);

/* ----------------------------------------------------------------------
 *  运行时统计 (在 RAM 中累加, 由主循环/事件驱动)
 * ---------------------------------------------------------------------- */

/* 记录一次按键事件 (key_id: 0~3, is_long: true=长按 / false=短按) */
void param_key_event(uint8_t key_id, bool is_long);

/* 每秒调用: 累计运行时长; low_batt=true 时同时累计低电时长;
 * 并按 ldm_on/t7_on 组合累加 LDM/laser 分时状态时长。 */
void param_tick_1s(bool low_batt, bool ldm_on, bool t7_on);

/* 记录关机原因 (设置 last_reason 并累计对应次数) */
void param_set_shutdown_reason(shutdown_reason_t reason);

/* 向预留时间槽累加秒数 (idx: 0~PARAM_TIME_SLOT_NUM-1) */
void param_add_time_slot(uint8_t idx, uint32_t sec);

/* 获取参数指针 (只读访问) */
const param_t *param_get(void);

/* ----------------------------------------------------------------------
 *  单位记忆 (供显示层读写上次使用的显示单位)
 * ---------------------------------------------------------------------- */

/* 读回上次显示单位 (digital_t 值 0~6) */
uint8_t param_get_digital(void);

/* 设置当前显示单位 (写入 RAM 镜像, 关机 param_save 时持久化) */
void param_set_digital(uint8_t digital);

/* ----------------------------------------------------------------------
 *  参数打印
 * ---------------------------------------------------------------------- */

/* 打印全部参数到串口 (供 AT+GET_ALL_INFO 指令 / 调试) */
void param_print_all(void);

#endif /* PARAM_PARAM_H_ */
