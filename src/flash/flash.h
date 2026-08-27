#ifndef FLASH_FLASH_H_
#define FLASH_FLASH_H_

#include "hal_data.h"
#include "stdint.h"

/* ======================================================================
 *  RA2E1 Data Flash 参数
 * ====================================================================== */
#define FLASH_DF_BASE_ADDR       (0x40100000U)   /* Data Flash 基地址 */
#define FLASH_DF_BLOCK_SIZE      (64U)           /* Data Flash 块大小 (字节) */
#define FLASH_DF_WRITE_SIZE      (4U)            /* Data Flash 最小写入粒度 */

/* ======================================================================
 *  对外接口
 * ====================================================================== */
fsp_err_t flash_init(void);
void flash_close(void);

/* 读 Data Flash（内存映射直接读取） */
fsp_err_t flash_read(uint32_t addr, void *buf, uint32_t len);

/* 擦除 Data Flash 指定数量的块（自 addr 起） */
fsp_err_t flash_erase(uint32_t addr, uint32_t num_blocks);

/* 写 Data Flash（写入前需先擦除对应块） */
fsp_err_t flash_write(uint32_t addr, const void *buf, uint32_t len);

#endif /* FLASH_FLASH_H_ */
