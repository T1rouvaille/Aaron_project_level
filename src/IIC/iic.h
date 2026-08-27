/*
 * iic.h
 *
 *  通用 I2C 底层驱动接口
 */

#ifndef IIC_IIC_H_
#define IIC_IIC_H_

#include "hal_data.h"

/* I2C 总线初始化 */
fsp_err_t i2c_master_init(void);

/* 动态设置 I2C 从机地址 (7-bit) */
fsp_err_t i2c_set_slave(uint32_t slave);

/* 通用 I2C 底层读写（单字节寄存器） */
fsp_err_t i2c_read_reg(uint8_t reg, uint8_t *data);
fsp_err_t i2c_write_reg(uint8_t reg, uint8_t value);
fsp_err_t i2c_read_bytes(uint8_t reg, uint8_t *buf, uint8_t len);

#endif /* IIC_IIC_H_ */
