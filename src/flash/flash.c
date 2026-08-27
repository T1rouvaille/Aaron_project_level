#include "flash/flash.h"
#include "r_flash_lp.h"
#include "string.h"

/* ======================================================================
 *  flash_init
 * ====================================================================== */
fsp_err_t flash_init(void)
{
    return R_FLASH_LP_Open(&g_flash0_ctrl, &g_flash0_cfg);
}

/* ======================================================================
 *  flash_close
 * ====================================================================== */
void flash_close(void)
{
    R_FLASH_LP_Close(&g_flash0_ctrl);
}

/* ======================================================================
 *  读 Data Flash（内存映射直接读取）
 * ====================================================================== */
fsp_err_t flash_read(uint32_t addr, void *buf, uint32_t len)
{
    if ((buf == NULL) || (len == 0U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    memcpy(buf, (const void *)addr, len);
    return FSP_SUCCESS;
}

/* ======================================================================
 *  擦除 Data Flash 指定数量的块
 * ====================================================================== */
fsp_err_t flash_erase(uint32_t addr, uint32_t num_blocks)
{
    return R_FLASH_LP_Erase(&g_flash0_ctrl, addr, num_blocks);
}

/* ======================================================================
 *  写 Data Flash（写入前需先擦除对应块）
 * ====================================================================== */
fsp_err_t flash_write(uint32_t addr, const void *buf, uint32_t len)
{
    if ((buf == NULL) || (len == 0U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    return R_FLASH_LP_Write(&g_flash0_ctrl, (uint32_t)buf, addr, len);
}
