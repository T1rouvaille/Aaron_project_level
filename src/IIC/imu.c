/*
 * imu.c
 *
 *  IMU 传感器识别与数据采集驱动实现
 *
 *  TDK (ICM-426xx 架构): IIM-42351 / ICM-40608
 *    WHO_AM_I   : 0x75
 *    PWR_MGMT0  : 0x4E
 *    ACCEL_CFG0 : 0x50
 *    加速度数据 : 0x1F (6 字节, 大端)
 *
 *  ST LSM6DSO / LSM6DSOTR:
 *    WHO_AM_I   : 0x0F
 *    CTRL1_XL   : 0x10
 *    CTRL3_C    : 0x12
 *    加速度数据 : 0x28 (6 字节, 小端)
 */

#include "iic/imu.h"
#include "iic/iic.h"
#include "debug_uart/bsp_debug_uart.h"
#include <stdio.h>
#include <stdbool.h>

/* ======================================================================
 *  TDK (ICM-426xx 架构) 寄存器定义
 * ====================================================================== */
#define TDK_WHO_AM_I_REG        0x75U
#define TDK_PWR_MGMT0_REG       0x4EU
#define TDK_ACCEL_CONFIG0_REG   0x50U
#define TDK_ACCEL_DATA_X1_REG   0x1FU

#define TDK_WHO_AM_I_IIM42351   0x6CU
#define TDK_WHO_AM_I_ICM40608   0x39U

/* ICM-426xx 加速度配置 (ODR=1kHz, ±4g) */
#define TDK_ACCEL_CONFIG0_VALUE 0x68U
/* ICM-426xx 电源管理: 加速度计 + 陀螺仪 低噪声模式 */
#define TDK_PWR_MGMT0_VALUE     0x03U

/* ======================================================================
 *  ST LSM6DSO / LSM6DSOTR 寄存器定义
 * ====================================================================== */
#define LSM6DSO_WHO_AM_I_REG    0x0FU
#define LSM6DSO_CTRL1_XL_REG    0x10U
#define LSM6DSO_CTRL3_C_REG     0x12U
#define LSM6DSO_OUTX_L_XL_REG   0x28U

#define LSM6DSO_WHO_AM_I_VAL    0x6CU

/* LSM6DSO 加速度配置 (ODR=416Hz, ±2g) */
#define LSM6DSO_CTRL1_XL_VALUE  0x60U
/* LSM6DSO 控制: BDU=1, 自动地址递增 */
#define LSM6DSO_CTRL3_C_VALUE   0x44U

/* ======================================================================
 *  全局变量
 * ====================================================================== */
static imu_sensor_t g_imu_sensor = IMU_SENSOR_NONE;

/* ======================================================================
 *  候选 I2C 从机地址 (7-bit)
 *  ST LSM6DSO: SA0=0 -> 0x6A, SA0=1 -> 0x6B
 *  TDK:        AD0=0 -> 0x68, AD0=1 -> 0x69
 * ====================================================================== */
#define IMU_SLAVE_ADDR_COUNT  4U
static const uint32_t s_imu_slave_addrs[IMU_SLAVE_ADDR_COUNT] =
{
    0x6AU, 0x6BU,   /* ST LSM6DSO 优先 */
    0x68U, 0x69U,   /* TDK            */
};

/* ======================================================================
 *  内部辅助: 读取 WHO_AM_I 并比对
 * ====================================================================== */
static bool imu_who_am_i_match(uint8_t reg, uint8_t expected)
{
    uint8_t val = 0U;
    if (FSP_SUCCESS != i2c_read_reg(reg, &val))
    {
        return false;
    }
    return (val == expected);
}

/* ======================================================================
 *  TDK (ICM-426xx 架构) 芯片初始化
 * ====================================================================== */
static void tdk_chip_init(void)
{
    /* 配置加速度计, 使能低噪声模式 */
    i2c_write_reg(TDK_ACCEL_CONFIG0_REG, TDK_ACCEL_CONFIG0_VALUE);
    i2c_write_reg(TDK_PWR_MGMT0_REG, TDK_PWR_MGMT0_VALUE);
}

/* ======================================================================
 *  ST LSM6DSO 芯片初始化
 * ====================================================================== */
static void lsm6dso_chip_init(void)
{
    /* 使能 BDU 与地址自动递增 */
    i2c_write_reg(LSM6DSO_CTRL3_C_REG, LSM6DSO_CTRL3_C_VALUE);
    /* 使能加速度计 */
    i2c_write_reg(LSM6DSO_CTRL1_XL_REG, LSM6DSO_CTRL1_XL_VALUE);
}

/* ======================================================================
 *  TDK (ICM-426xx 架构) 读取三轴加速度 (大端)
 * ====================================================================== */
static int32_t tdk_get_acc(int16_t *acc)
{
    uint8_t buf[6];
    if (FSP_SUCCESS != i2c_read_bytes(TDK_ACCEL_DATA_X1_REG, buf, 6U))
    {
        return -1;
    }

    /* ICM-426xx 加速度数据为大端: [X_H X_L Y_H Y_L Z_H Z_L] */
    acc[0] = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    acc[1] = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
    acc[2] = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);

    return 0;
}

/* ======================================================================
 *  ST LSM6DSO 读取三轴加速度 (小端)
 * ====================================================================== */
static int32_t lsm6dso_get_acc(int16_t *acc)
{
    uint8_t buf[6];
    if (FSP_SUCCESS != i2c_read_bytes(LSM6DSO_OUTX_L_XL_REG, buf, 6U))
    {
        return -1;
    }

    /* LSM6DSO 加速度数据为小端: [X_L X_H Y_L Y_H Z_L Z_H] */
    acc[0] = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
    acc[1] = (int16_t)((uint16_t)buf[3] << 8 | buf[2]);
    acc[2] = (int16_t)((uint16_t)buf[5] << 8 | buf[4]);

    return 0;
}

/* ======================================================================
 *  IMU 初始化: 自动识别芯片型号并完成基本配置
 * ====================================================================== */
imu_sensor_t imu_init(void)
{
    char buf[48];

    g_imu_sensor = IMU_SENSOR_NONE;

    for (uint32_t i = 0U; i < IMU_SLAVE_ADDR_COUNT; i++)
    {
        /* 切换到候选从机地址 */
        if (FSP_SUCCESS != i2c_set_slave(s_imu_slave_addrs[i]))
        {
            continue;
        }

        /* 1. 按 ICM-426xx 架构 (0x75) 识别 TDK 芯片 */
        if (imu_who_am_i_match(TDK_WHO_AM_I_REG, TDK_WHO_AM_I_IIM42351))
        {
            g_imu_sensor = IMU_SENSOR_IIM42351;
            tdk_chip_init();
            sprintf(buf, "[IMU ] IIM-42351 @ 0x%02lX\r\n", (unsigned long)s_imu_slave_addrs[i]);
            uart9_send_blocking(buf);
            break;
        }

        if (imu_who_am_i_match(TDK_WHO_AM_I_REG, TDK_WHO_AM_I_ICM40608))
        {
            g_imu_sensor = IMU_SENSOR_ICM40608;
            tdk_chip_init();
            sprintf(buf, "[IMU ] ICM-40608 @ 0x%02lX\r\n", (unsigned long)s_imu_slave_addrs[i]);
            uart9_send_blocking(buf);
            break;
        }

        /* 2. 按 ST 架构 (0x0F) 识别 LSM6DSO */
        if (imu_who_am_i_match(LSM6DSO_WHO_AM_I_REG, LSM6DSO_WHO_AM_I_VAL))
        {
            g_imu_sensor = IMU_SENSOR_LSM6DSO;
            lsm6dso_chip_init();
            sprintf(buf, "[IMU ] LSM6DSOTR @ 0x%02lX\r\n", (unsigned long)s_imu_slave_addrs[i]);
            uart9_send_blocking(buf);
            break;
        }
    }

    if (IMU_SENSOR_NONE == g_imu_sensor)
    {
        uart9_send_blocking("[IMU ] Not Found\r\n");
    }

    return g_imu_sensor;
}

/* ======================================================================
 *  获取当前识别到的传感器类型
 * ====================================================================== */
imu_sensor_t imu_get_sensor(void)
{
    return g_imu_sensor;
}

/* ======================================================================
 *  读取三轴加速度 (按识别到的芯片分派)
 * ====================================================================== */
int32_t imu_get_acc(int16_t *acc)
{
    if (NULL == acc)
    {
        return -1;
    }

    switch (g_imu_sensor)
    {
        case IMU_SENSOR_IIM42351:
        case IMU_SENSOR_ICM40608:
            return tdk_get_acc(acc);

        case IMU_SENSOR_LSM6DSO:
            return lsm6dso_get_acc(acc);

        default:
            acc[0] = 0;
            acc[1] = 0;
            acc[2] = 0;
            return -1;
    }
}
