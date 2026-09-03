/*
 * lcd.c
 *
 *  AiP31033E LCD 驱动 (4 线 SPI)
 *  硬件 SPI (R_SPI0) + GPIO 手动控制 A0/CS/RST
 *
 *  SPI 时序: Mode 3 (CPOL=1, CPHA=1) — 空闲高电平, 上升沿锁存
 *  数据格式: 每字节 CS 拉低 → 发 8bit (MSB first) → CS 拉高
 *            A0=0 命令, A0=1 数据
 *
 *  说明: R_SPI_Write() 是 FSP 非阻塞 API, 传输完成由 spi_callback 通知,
 *        因此每字节发送后需等待回调标志再拉高 CS。
 *
 *  指令集: 本芯片为 ST7033 (Sitronix) 兼容, 严格按
 *          ENH-SG626066-01-YVNTKF.c 参考代码逐字节移植。
 */

#include "lcd/lcd.h"
#include "config.h"

/* ======================================================================
 *  ST7033 命令字 (A0=0)
 * ====================================================================== */

/* 软件复位 */
#define LCD_CMD_SOFT_RESET      (0xE2U)

/* 指令表切换: 基础表 (0xF0) / 扩展表 (0xF1) */
#define LCD_CMD_TABLE_BASIC     (0xF0U)
#define LCD_CMD_TABLE_EXT       (0xF1U)

/* 扩展表: 振荡器 + 偏置 (参考代码 0x70) */
#define LCD_CMD_OSC_BIAS        (0x70U)

/* 扩展表: VLCD 偏置 / 跟随器 (参考代码 0x62) */
#define LCD_CMD_BIAS            (0x62U)

/* 电源控制 (VC/VR/VF 全开) */
#define LCD_CMD_POWER           (0x2FU)

/* 对比度: 0x80 | EV[4:0], EV 0~31 (参考代码 0x8F, EV=0x0F)
 * 注意: EV 仅 5 位, 超过 0x1F 会越界成 SEG 方向命令 (0xA0) */
#define LCD_CMD_CONTRAST(ev)    ((uint8_t)(0x80U | ((ev) & 0x1FU)))

/* SEG 方向: MX=0 (参考代码 0xA0) */
#define LCD_CMD_SEG_MX0         (0xA0U)
#define LCD_CMD_SEG_MX1         (0xA1U)

/* COM 方向: MY=0 (参考代码 0xC0) */
#define LCD_CMD_COM_MY0         (0xC0U)
#define LCD_CMD_COM_MY1         (0xC8U)

/* 正常显示 / 反转 */
#define LCD_CMD_NORMAL          (0xA4U)
#define LCD_CMD_REVERSE         (0xA5U)

/* 显示开 / 关 */
#define LCD_CMD_DISPLAY_ON      (0xAFU)
#define LCD_CMD_DISPLAY_OFF     (0xAEU)

/* DDRAM 页地址: 0xB0 | page[3:0] (参考代码 0xB0) */
#define LCD_CMD_PAGE(page)      ((uint8_t)(0xB0U | ((page) & 0x0FU)))

/* DDRAM 列地址: 高 3 位 (0x10~0x17), 低 4 位 (0x00~0x0F) */
#define LCD_CMD_COL_HIGH(col)   ((uint8_t)(0x10U | (((col) >> 4U) & 0x07U)))
#define LCD_CMD_COL_LOW(col)    ((uint8_t)(0x00U | ((col) & 0x0FU)))

/* ======================================================================
 *  传输完成标志 (由 SPI 回调置位)
 * ====================================================================== */
static volatile uint32_t s_spi_done = 0U;

/* SPI 传输完成回调 (FSP 非阻塞 API 完成时调用) */
void spi_callback(spi_callback_args_t *p_args)
{
    if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
        s_spi_done = 1U;
    }
}

/* ----------------------------------------------------------------------
 *  底层: 发送一个字节 (a0 指定命令/数据)
 * ---------------------------------------------------------------------- */
static void lcd_spi_write_byte(uint8_t a0, uint8_t byte)
{
    /* 先设置 A0 (满足 tSAS 建立时间) */
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_A0_PIN,
                      (a0 != 0U) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);

    /* 拉低 CS 选中芯片 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_CS_PIN, BSP_IO_LEVEL_LOW);

    /* 启动发送并等待完成 (R_SPI_Write 非阻塞, 完成后回调置位) */
    s_spi_done = 0U;
    R_SPI_Write(&g_spi0_ctrl, &byte, 1U, SPI_BIT_WIDTH_8_BITS);
    while (0U == s_spi_done)
    {
        /* 等待传输完成 */
    }

    /* 拉高 CS (满足 tCSH 保持时间) */
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_CS_PIN, BSP_IO_LEVEL_HIGH);
}

/* 写命令 */
void lcd_write_cmd(uint8_t cmd)
{
    lcd_spi_write_byte(0U, cmd);
}

/* 写数据 (DDRAM) */
void lcd_write_data(uint8_t data)
{
    lcd_spi_write_byte(1U, data);
}

/* 设置 DDRAM 页地址 (0~3) */
void lcd_set_page(uint8_t page)
{
    lcd_write_cmd(LCD_CMD_PAGE(page));
}

/* 设置 DDRAM 列地址 (0~95) */
void lcd_set_column(uint8_t col)
{
    lcd_write_cmd(LCD_CMD_COL_HIGH(col));
    lcd_write_cmd(LCD_CMD_COL_LOW(col));
}

/* 写图案: 从 page 0 列 0 开始写 len 字节 (每字节 = 1 个 SEG 的段码) */
void lcd_write_pattern(const uint8_t *pattern, uint8_t len)
{
    uint8_t i;

    lcd_set_page(0U);
    lcd_set_column(0U);
    for (i = 0U; i < len; i++)
    {
        lcd_write_data(pattern[i]);
    }
}

/* 清屏 (全部段关闭) */
void lcd_clear(void)
{
    uint8_t col;

    lcd_set_page(0U);
    lcd_set_column(0U);
    for (col = 0U; col < LCD_SEG_NUM; col++)
    {
        lcd_write_data(LCD_SEG_OFF);
    }
}

/* 全亮 (全部段点亮, 用于硬件连接测试) */
void lcd_full(void)
{
    uint8_t col;

    lcd_set_page(0U);
    lcd_set_column(0U);
    for (col = 0U; col < LCD_SEG_NUM; col++)
    {
        lcd_write_data(0xFFU);
    }
}

/* 显示开 */
void lcd_display_on(void)
{
    lcd_write_cmd(LCD_CMD_DISPLAY_ON);
}

/* 显示关 */
void lcd_display_off(void)
{
    lcd_write_cmd(LCD_CMD_DISPLAY_OFF);
}

/* 设置对比度 (EV 0~63) */
void lcd_set_contrast(uint8_t ev)
{
    lcd_write_cmd(LCD_CMD_CONTRAST(ev));
}

/* 初始化 LCD */
fsp_err_t lcd_init(void)
{
    fsp_err_t err;

    /* 1. 打开 SPI (阻塞写入, 无 DTC) */
    err = R_SPI_Open(&g_spi0_ctrl, &g_spi0_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 2. 硬件复位: 高 → 低 → 高 (参考代码 RES=1/0/1, 各延时) */
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_RST_PIN, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(2U, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_RST_PIN, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(2U, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_RST_PIN, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(2U, BSP_DELAY_UNITS_MILLISECONDS);

    /* 3. 命令初始化 (严格按 ENH-SG626066-01-YVNTKF.c 顺序) */
    lcd_write_cmd(LCD_CMD_SOFT_RESET);      /* 软件复位 */
    lcd_write_cmd(LCD_CMD_TABLE_EXT);       /* 切扩展表 */
    lcd_write_cmd(LCD_CMD_OSC_BIAS);        /* 振荡器 + 偏置 */
    lcd_write_cmd(LCD_CMD_TABLE_BASIC);     /* 切基础表 */

    lcd_write_cmd(LCD_CMD_POWER);           /* 电源控制全开 */
    lcd_write_cmd(LCD_CMD_CONTRAST(LCD_DEFAULT_EV)); /* 对比度 0x8F */

    lcd_write_cmd(LCD_CMD_SEG_MX0);         /* MX=0 */
    lcd_write_cmd(LCD_CMD_COM_MY0);         /* MY=0 */
    lcd_write_cmd(LCD_CMD_NORMAL);          /* 正常显示 */

    lcd_write_cmd(LCD_CMD_TABLE_EXT);       /* 切扩展表 */
    lcd_write_cmd(LCD_CMD_BIAS);            /* VLCD 偏置 */
    lcd_write_cmd(LCD_CMD_NORMAL);          /* 正常显示 */
    lcd_write_cmd(LCD_CMD_TABLE_BASIC);     /* 切基础表 */

    lcd_write_cmd(LCD_CMD_DISPLAY_ON);      /* 开显示 */

    return FSP_SUCCESS;
}
