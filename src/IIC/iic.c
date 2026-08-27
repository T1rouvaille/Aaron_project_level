/*
 * iic.c
 *
 *  通用 I2C 底层驱动实现
 */

#include "iic/iic.h"

/* ========== 全局变量 ========== */
static volatile bool g_i2c_done = false;

/* ========== I2C 回调 ========== */
void i2c_callback(i2c_master_callback_args_t *p_args)
{
    if ((p_args->event == I2C_MASTER_EVENT_TX_COMPLETE) ||
        (p_args->event == I2C_MASTER_EVENT_RX_COMPLETE) ||
        (p_args->event == I2C_MASTER_EVENT_ABORTED))
    {
        g_i2c_done = true;
    }
}

/* ========== I2C 初始化 ========== */
fsp_err_t i2c_master_init(void)
{
    return R_IIC_MASTER_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);
}

/* ========== 动态设置从机地址 ========== */
fsp_err_t i2c_set_slave(uint32_t slave)
{
    return R_IIC_MASTER_SlaveAddressSet(&g_i2c_master0_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
}

/* ========== 读单字节寄存器 ========== */
fsp_err_t i2c_read_reg(uint8_t reg, uint8_t *data)
{
    fsp_err_t err;

    g_i2c_done = false;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &reg, 1, true);
    if (err != FSP_SUCCESS) return err;
    while(!g_i2c_done);

    g_i2c_done = false;
    err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, data, 1, false);
    if (err != FSP_SUCCESS) return err;
    while(!g_i2c_done);

    return FSP_SUCCESS;
}

/* ========== 写寄存器 ========== */
fsp_err_t i2c_write_reg(uint8_t reg, uint8_t value)
{
    fsp_err_t err;
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = value;

    g_i2c_done = false;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, buf, 2, false);
    if (err != FSP_SUCCESS) return err;
    while(!g_i2c_done);

    return FSP_SUCCESS;
}

/* ========== 连续读 n 字节 ========== */
fsp_err_t i2c_read_bytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    fsp_err_t err;

    g_i2c_done = false;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &reg, 1, true);
    if (err != FSP_SUCCESS) return err;
    while(!g_i2c_done);

    g_i2c_done = false;
    err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, buf, len, false);
    if (err != FSP_SUCCESS) return err;
    while(!g_i2c_done);

    return FSP_SUCCESS;
}
