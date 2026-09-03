/*
 * lcd.h
 *
 *  AiP31033E LCD 驱动接口 (4 线 SPI)
 *  硬件 SPI (R_SPI0) + GPIO 手动控制 A0/CS/RST
 *
 *  注意: 本芯片实为 ST7033 (Sitronix) 指令集兼容,
 *        指令编码与 AiP31033E 数据手册 OCR 结果不同,
 *        一律以 ENH-SG626066-01-YVNTKF.c 参考代码为准。
 */

#ifndef LCD_LCD_H_
#define LCD_LCD_H_

#include "hal_data.h"

/* 引脚 (LCD_A0/RST/CS_PIN) 与默认对比度 (LCD_DEFAULT_EV) 集中定义于 config.h */

/* LCD 几何参数 */
#define LCD_COM_NUM     (2U)    /* COM 数 (1/2 duty, 规格书) */
#define LCD_SEG_NUM     (50U)   /* 有效 SEG 数 (面板实际连接段 SEG[0]~SEG[49]) */

/* 段点亮值: 1/2 duty, 每字节 bit0=COM0段, bit1=COM1段 */
#define LCD_SEG_OFF     (0x00U) /* 段灭 */
#define LCD_SEG_ON      (0x03U) /* 段亮 (2 个 COM 全开) */

/* 初始化 LCD (打开 SPI + 硬件复位 + 命令初始化) */
fsp_err_t lcd_init(void);

/* 写命令 (A0=0) / 写数据 (A0=1) */
void lcd_write_cmd(uint8_t cmd);
void lcd_write_data(uint8_t data);

/* 设置 DDRAM 页地址 (0~3) */
void lcd_set_page(uint8_t page);

/* 设置 DDRAM 列地址 (0~95) */
void lcd_set_column(uint8_t col);

/* 清屏 (全部段关闭) */
void lcd_clear(void);

/* 全亮 (全部段点亮, 用于硬件连接测试) */
void lcd_full(void);

/* 写图案: 从 page 0 列 0 开始写 len 字节 (每字节 = 1 个 SEG 的段码) */
void lcd_write_pattern(const uint8_t *pattern, uint8_t len);

/* 显示开 / 关 */
void lcd_display_on(void);
void lcd_display_off(void);

/* 设置对比度 (EV 0~63) */
void lcd_set_contrast(uint8_t ev);

#endif /* LCD_LCD_H_ */
