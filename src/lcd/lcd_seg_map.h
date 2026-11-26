/*
 * lcd_seg_map.h
 *
 *  LCD 段码映射表 (新面板, AiP31033E 驱动, 1/2 duty)
 *
 *  段码屏 1/2 duty, 每个 SEG 引脚在 COM0/COM1 各接一根段:
 *    COM0 -> bit0 (0x01)
 *    COM1 -> bit1 (0x02)
 *
 *  命名规则:
 *    LCD_<段名>_SEG  = SEG 索引 (0~95)
 *    LCD_<段名>_BIT  = COM bit 掩码 (0x01 或 0x02)
 *
 *  段名:
 *    数字段 = 数字位置(1~12) + 七段(A~G), 如 LCD_8A_SEG
 *    图标段 = T + 编号(1~41),           如 LCD_T27_SEG
 *
 *  用法:
 *    lcd_frame[LCD_8A_SEG] |= LCD_8A_BIT;   // 点亮数字 8 的 A 段
 *
 *  说明: 图标 T1~T41 的物理含义待新面板语义确认后再补充注释。
 */

#ifndef LCD_SEG_MAP_H_
#define LCD_SEG_MAP_H_

#include <stdint.h>

/* 段引脚描述: SEG 索引 + COM bit 掩码 */
typedef struct
{
    uint8_t seg;
    uint8_t bit;
} lcd_seg_pin_t;

/* ======================================================================
 *  数字段 (1~12, 每数字 A~G 七段)
 * ====================================================================== */

/* ---- 数字 1 ---- */
#define LCD_1A_SEG  (35U)
#define LCD_1A_BIT  (0x01U)
#define LCD_1B_SEG  (34U)
#define LCD_1B_BIT  (0x01U)
#define LCD_1C_SEG  (34U)
#define LCD_1C_BIT  (0x02U)
#define LCD_1D_SEG  (37U)
#define LCD_1D_BIT  (0x02U)
#define LCD_1E_SEG  (36U)
#define LCD_1E_BIT  (0x02U)
#define LCD_1F_SEG  (36U)
#define LCD_1F_BIT  (0x01U)
#define LCD_1G_SEG  (35U)
#define LCD_1G_BIT  (0x02U)

/* ---- 数字 2 ---- */
#define LCD_2A_SEG  (32U)
#define LCD_2A_BIT  (0x01U)
#define LCD_2B_SEG  (31U)
#define LCD_2B_BIT  (0x01U)
#define LCD_2C_SEG  (31U)
#define LCD_2C_BIT  (0x02U)
#define LCD_2D_SEG  (30U)
#define LCD_2D_BIT  (0x02U)
#define LCD_2E_SEG  (33U)
#define LCD_2E_BIT  (0x02U)
#define LCD_2F_SEG  (33U)
#define LCD_2F_BIT  (0x01U)
#define LCD_2G_SEG  (32U)
#define LCD_2G_BIT  (0x02U)

/* ---- 数字 3 ---- */
#define LCD_3A_SEG  (27U)
#define LCD_3A_BIT  (0x01U)
#define LCD_3B_SEG  (26U)
#define LCD_3B_BIT  (0x01U)
#define LCD_3C_SEG  (26U)
#define LCD_3C_BIT  (0x02U)
#define LCD_3D_SEG  (25U)
#define LCD_3D_BIT  (0x02U)
#define LCD_3E_SEG  (28U)
#define LCD_3E_BIT  (0x02U)
#define LCD_3F_SEG  (28U)
#define LCD_3F_BIT  (0x01U)
#define LCD_3G_SEG  (27U)
#define LCD_3G_BIT  (0x02U)

/* ---- 数字 4 ---- */
#define LCD_4A_SEG  (23U)
#define LCD_4A_BIT  (0x01U)
#define LCD_4B_SEG  (22U)
#define LCD_4B_BIT  (0x01U)
#define LCD_4C_SEG  (22U)
#define LCD_4C_BIT  (0x02U)
#define LCD_4D_SEG  (21U)
#define LCD_4D_BIT  (0x02U)
#define LCD_4E_SEG  (24U)
#define LCD_4E_BIT  (0x02U)
#define LCD_4F_SEG  (24U)
#define LCD_4F_BIT  (0x01U)
#define LCD_4G_SEG  (23U)
#define LCD_4G_BIT  (0x02U)

/* ---- 数字 5 ---- */
#define LCD_5A_SEG  (15U)
#define LCD_5A_BIT  (0x01U)
#define LCD_5B_SEG  (14U)
#define LCD_5B_BIT  (0x01U)
#define LCD_5C_SEG  (14U)
#define LCD_5C_BIT  (0x02U)
#define LCD_5D_SEG  (13U)
#define LCD_5D_BIT  (0x02U)
#define LCD_5E_SEG  (16U)
#define LCD_5E_BIT  (0x02U)
#define LCD_5F_SEG  (16U)
#define LCD_5F_BIT  (0x01U)
#define LCD_5G_SEG  (15U)
#define LCD_5G_BIT  (0x02U)

/* ---- 数字 6 ---- */
#define LCD_6A_SEG  (11U)
#define LCD_6A_BIT  (0x01U)
#define LCD_6B_SEG  (10U)
#define LCD_6B_BIT  (0x01U)
#define LCD_6C_SEG  (10U)
#define LCD_6C_BIT  (0x02U)
#define LCD_6D_SEG  (1U)
#define LCD_6D_BIT  (0x02U)
#define LCD_6E_SEG  (12U)
#define LCD_6E_BIT  (0x02U)
#define LCD_6F_SEG  (12U)
#define LCD_6F_BIT  (0x01U)
#define LCD_6G_SEG  (11U)
#define LCD_6G_BIT  (0x02U)

/* ---- 数字 7 ---- */
#define LCD_7A_SEG  (8U)
#define LCD_7A_BIT  (0x01U)
#define LCD_7B_SEG  (7U)
#define LCD_7B_BIT  (0x01U)
#define LCD_7C_SEG  (7U)
#define LCD_7C_BIT  (0x02U)
#define LCD_7D_SEG  (6U)
#define LCD_7D_BIT  (0x02U)
#define LCD_7E_SEG  (9U)
#define LCD_7E_BIT  (0x02U)
#define LCD_7F_SEG  (9U)
#define LCD_7F_BIT  (0x01U)
#define LCD_7G_SEG  (8U)
#define LCD_7G_BIT  (0x02U)

/* ---- 数字 8 ---- */
#define LCD_8A_SEG  (4U)
#define LCD_8A_BIT  (0x01U)
#define LCD_8B_SEG  (3U)
#define LCD_8B_BIT  (0x01U)
#define LCD_8C_SEG  (3U)
#define LCD_8C_BIT  (0x02U)
#define LCD_8D_SEG  (2U)
#define LCD_8D_BIT  (0x02U)
#define LCD_8E_SEG  (5U)
#define LCD_8E_BIT  (0x02U)
#define LCD_8F_SEG  (5U)
#define LCD_8F_BIT  (0x01U)
#define LCD_8G_SEG  (4U)
#define LCD_8G_BIT  (0x02U)

/* ---- 数字 9 ---- */
#define LCD_9A_SEG  (50U)
#define LCD_9A_BIT  (0x02U)
#define LCD_9B_SEG  (49U)
#define LCD_9B_BIT  (0x02U)
#define LCD_9C_SEG  (49U)
#define LCD_9C_BIT  (0x01U)
#define LCD_9D_SEG  (48U)
#define LCD_9D_BIT  (0x01U)
#define LCD_9E_SEG  (47U)
#define LCD_9E_BIT  (0x01U)
#define LCD_9F_SEG  (47U)
#define LCD_9F_BIT  (0x02U)
#define LCD_9G_SEG  (48U)
#define LCD_9G_BIT  (0x02U)

/* ---- 数字 10 ---- */
#define LCD_10A_SEG  (54U)
#define LCD_10A_BIT  (0x02U)
#define LCD_10B_SEG  (53U)
#define LCD_10B_BIT  (0x02U)
#define LCD_10C_SEG  (53U)
#define LCD_10C_BIT  (0x01U)
#define LCD_10D_SEG  (52U)
#define LCD_10D_BIT  (0x01U)
#define LCD_10E_SEG  (51U)
#define LCD_10E_BIT  (0x01U)
#define LCD_10F_SEG  (51U)
#define LCD_10F_BIT  (0x02U)
#define LCD_10G_SEG  (52U)
#define LCD_10G_BIT  (0x02U)

/* ---- 数字 11 ---- */
#define LCD_11A_SEG  (56U)
#define LCD_11A_BIT  (0x02U)
#define LCD_11B_SEG  (59U)
#define LCD_11B_BIT  (0x02U)
#define LCD_11C_SEG  (59U)
#define LCD_11C_BIT  (0x01U)
#define LCD_11D_SEG  (58U)
#define LCD_11D_BIT  (0x01U)
#define LCD_11E_SEG  (57U)
#define LCD_11E_BIT  (0x01U)
#define LCD_11F_SEG  (57U)
#define LCD_11F_BIT  (0x02U)
#define LCD_11G_SEG  (58U)
#define LCD_11G_BIT  (0x02U)

/* ---- 数字 12 ---- */
#define LCD_12A_SEG  (55U)
#define LCD_12A_BIT  (0x02U)
#define LCD_12B_SEG  (62U)
#define LCD_12B_BIT  (0x02U)
#define LCD_12C_SEG  (62U)
#define LCD_12C_BIT  (0x01U)
#define LCD_12D_SEG  (61U)
#define LCD_12D_BIT  (0x01U)
#define LCD_12E_SEG  (60U)
#define LCD_12E_BIT  (0x01U)
#define LCD_12F_SEG  (60U)
#define LCD_12F_BIT  (0x02U)
#define LCD_12G_SEG  (61U)
#define LCD_12G_BIT  (0x02U)

/* ======================================================================
 *  图标段 (T1~T41)
 *
 *  新面板图标含义对照 (数字分三排, 每排 4 个):
 *    T1  水平向下的箭头         T2  水平仪
 *    T3  更向下的箭头           T4  一档基准
 *    T5  二档基准               T6  最向下的箭头
 *    T7  三档基准               T8  十字加号
 *    T9  电量显示框             T10 第一格电量
 *    T11 第二格电量             T12 第三格电量
 *    T13 校准开关               T14 反向的引号
 *    T15 正向向上的箭头         T16 中间最前面的小数点 (反向角度符号)
 *    T17 反向的双引号           T18 反向的 mm
 *    T19 反向的 M               T20 反向的英寸
 *    T21 反向的英尺             T22 反向的单引号
 *    T23 正向下排数字第一个小数点
 *    T24 正向下排数字反过来的第一个小数点
 *    T25 正向下排数字的斜杠
 *    T26 正向下排数字的第二个小数点
 *    T27 正向中排数字第一个小数点
 *    T28 反向中排数字第一个小数点
 *    T29 正向的引号             T30 正向向下的箭头
 *    T31 正向的双引号           T32 中间最后面的小数点 (正向角度符号)
 *    T33 正向上排数字的反向第二个小数点
 *    T34 正向上排数字的斜杠符号
 *    T35 正向上排数字第一个小数点
 *    T36 正向上排数字的反向第一格小数点
 *    T37 正向的单引号           T38 正向的英尺
 *    T39 正向的英寸             T40 正向的 M
 *    T41 正向的 MM
 * ====================================================================== */

#define LCD_T1_SEG   (38U)
#define LCD_T1_BIT   (0x01U)
#define LCD_T2_SEG   (39U)
#define LCD_T2_BIT   (0x01U)
#define LCD_T3_SEG   (38U)
#define LCD_T3_BIT   (0x02U)
#define LCD_T4_SEG   (37U)
#define LCD_T4_BIT   (0x01U)
#define LCD_T5_SEG   (39U)
#define LCD_T5_BIT   (0x02U)
#define LCD_T6_SEG   (40U)
#define LCD_T6_BIT   (0x01U)
#define LCD_T7_SEG   (40U)
#define LCD_T7_BIT   (0x02U)
#define LCD_T8_SEG   (41U)
#define LCD_T8_BIT   (0x01U)
#define LCD_T9_SEG   (43U)
#define LCD_T9_BIT   (0x01U)
#define LCD_T10_SEG  (42U)
#define LCD_T10_BIT  (0x01U)
#define LCD_T11_SEG  (42U)
#define LCD_T11_BIT  (0x02U)
#define LCD_T12_SEG  (43U)
#define LCD_T12_BIT  (0x02U)
#define LCD_T13_SEG  (41U)
#define LCD_T13_BIT  (0x02U)
#define LCD_T14_SEG  (17U)
#define LCD_T14_BIT  (0x01U)
#define LCD_T15_SEG  (18U)
#define LCD_T15_BIT  (0x01U)
#define LCD_T16_SEG  (18U)
#define LCD_T16_BIT  (0x02U)
#define LCD_T17_SEG  (17U)
#define LCD_T17_BIT  (0x02U)
#define LCD_T18_SEG  (46U)
#define LCD_T18_BIT  (0x02U)
#define LCD_T19_SEG  (45U)
#define LCD_T19_BIT  (0x02U)
#define LCD_T20_SEG  (45U)
#define LCD_T20_BIT  (0x01U)
#define LCD_T21_SEG  (44U)
#define LCD_T21_BIT  (0x01U)
#define LCD_T22_SEG  (46U)
#define LCD_T22_BIT  (0x01U)
#define LCD_T23_SEG  (50U)
#define LCD_T23_BIT  (0x01U)
#define LCD_T24_SEG  (54U)
#define LCD_T24_BIT  (0x01U)
#define LCD_T25_SEG  (55U)
#define LCD_T25_BIT  (0x01U)
#define LCD_T26_SEG  (56U)
#define LCD_T26_BIT  (0x01U)
#define LCD_T27_SEG  (13U)
#define LCD_T27_BIT  (0x01U)
#define LCD_T28_SEG  (6U)
#define LCD_T28_BIT  (0x01U)
#define LCD_T29_SEG  (0U)
#define LCD_T29_BIT  (0x02U)
#define LCD_T30_SEG  (0U)
#define LCD_T30_BIT  (0x01U)
#define LCD_T31_SEG  (2U)
#define LCD_T31_BIT  (0x01U)
#define LCD_T32_SEG  (1U)
#define LCD_T32_BIT  (0x01U)
#define LCD_T33_SEG  (30U)
#define LCD_T33_BIT  (0x01U)
#define LCD_T34_SEG  (29U)
#define LCD_T34_BIT  (0x01U)
#define LCD_T35_SEG  (29U)
#define LCD_T35_BIT  (0x02U)
#define LCD_T36_SEG  (25U)
#define LCD_T36_BIT  (0x01U)
#define LCD_T37_SEG  (21U)
#define LCD_T37_BIT  (0x01U)
#define LCD_T38_SEG  (19U)
#define LCD_T38_BIT  (0x01U)
#define LCD_T39_SEG  (20U)
#define LCD_T39_BIT  (0x01U)
#define LCD_T40_SEG  (20U)
#define LCD_T40_BIT  (0x02U)
#define LCD_T41_SEG  (19U)
#define LCD_T41_BIT  (0x02U)

/* ======================================================================
 *  数字显示函数
 * ====================================================================== */

/*
 * 在数字位置 pos(1~12) 显示数字 digit(0~9)。
 * 只修改 frame 缓冲 (不刷屏), 调用者随后用 lcd_write_pattern 整块写入。
 * 越界参数 (pos<1 或 pos>12 或 digit>9) 直接忽略。
 */
void lcd_show_digit(uint8_t pos, uint8_t digit, uint8_t *frame);

/*
 * 在数字位置 pos(1~12) 以 180° 倒置显示数字 digit(0~9)。
 * 段码旋转映射: A<->D, B<->E, C<->F, G<->G。用于反向组倒置显示。
 * 只修改 frame 缓冲 (不刷屏), 越界参数 (pos<1 或 pos>12 或 digit>9) 直接忽略。
 */
void lcd_show_digit_rot(uint8_t pos, uint8_t digit, uint8_t *frame);

/* ======================================================================
 *  图标显示
 * ====================================================================== */

#define LCD_ICON_NUM  (41U)   /* 图标 T1~T41 数量 */

/*
 * 点亮第 idx 个图标 (idx: 0=T1, 1=T2, ... 40=T41)。
 * 只修改 frame 缓冲 (不刷屏), 调用者随后用 lcd_write_pattern 整块写入。
 * 越界参数 (idx>=41) 直接忽略。
 */
void lcd_show_icon(uint8_t idx, uint8_t *frame);

#endif /* LCD_SEG_MAP_H_ */
