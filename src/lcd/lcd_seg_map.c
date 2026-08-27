/*
 * lcd_seg_map.c
 *
 *  LCD 数字显示实现 (依赖 lcd_seg_map.h 的段映射宏)
 */

#include "lcd/lcd_seg_map.h"

/* ----------------------------------------------------------------------
 *  数字 0~9 的七段码
 *  bit0=A, bit1=B, bit2=C, bit3=D, bit4=E, bit5=F, bit6=G
 * ---------------------------------------------------------------------- */
static const uint8_t lcd_digit_code[10] =
{
    0x3FU, /* 0: A B C D E F        */
    0x06U, /* 1: B C                */
    0x5BU, /* 2: A B G E D          */
    0x4FU, /* 3: A B C D G          */
    0x66U, /* 4: F G B C            */
    0x6DU, /* 5: A F G C D          */
    0x7DU, /* 6: A F E D C G        */
    0x07U, /* 7: A B C              */
    0x7FU, /* 8: A B C D E F G      */
    0x6FU, /* 9: A B C D F G        */
};

/* ----------------------------------------------------------------------
 *  10 个数字位置的七段引脚 (顺序 A B C D E F G)
 *  由 lcd_seg_map.h 的宏展开, 保证与映射表单一来源 (DRY)
 * ---------------------------------------------------------------------- */
static const lcd_seg_pin_t lcd_digit_pins[10][7] =
{
    /* 数字 1 (P1: 反向的最后一位数字) */
    {
        { LCD_1A_SEG, LCD_1A_BIT }, { LCD_1B_SEG, LCD_1B_BIT }, { LCD_1C_SEG, LCD_1C_BIT },
        { LCD_1D_SEG, LCD_1D_BIT }, { LCD_1E_SEG, LCD_1E_BIT }, { LCD_1F_SEG, LCD_1F_BIT },
        { LCD_1G_SEG, LCD_1G_BIT },
    },
    /* 数字 2 (P2: 反向的中间一位数字) */
    {
        { LCD_2A_SEG, LCD_2A_BIT }, { LCD_2B_SEG, LCD_2B_BIT }, { LCD_2C_SEG, LCD_2C_BIT },
        { LCD_2D_SEG, LCD_2D_BIT }, { LCD_2E_SEG, LCD_2E_BIT }, { LCD_2F_SEG, LCD_2F_BIT },
        { LCD_2G_SEG, LCD_2G_BIT },
    },
    /* 数字 3 (P3: 反向第一位数字) */
    {
        { LCD_3A_SEG, LCD_3A_BIT }, { LCD_3B_SEG, LCD_3B_BIT }, { LCD_3C_SEG, LCD_3C_BIT },
        { LCD_3D_SEG, LCD_3D_BIT }, { LCD_3E_SEG, LCD_3E_BIT }, { LCD_3F_SEG, LCD_3F_BIT },
        { LCD_3G_SEG, LCD_3G_BIT },
    },
    /* 数字 4 (P4: 中间正向第一位/反向最后一位数字) */
    {
        { LCD_4A_SEG, LCD_4A_BIT }, { LCD_4B_SEG, LCD_4B_BIT }, { LCD_4C_SEG, LCD_4C_BIT },
        { LCD_4D_SEG, LCD_4D_BIT }, { LCD_4E_SEG, LCD_4E_BIT }, { LCD_4F_SEG, LCD_4F_BIT },
        { LCD_4G_SEG, LCD_4G_BIT },
    },
    /* 数字 5 (P5: 正向中间第二位/反向第三位数字) */
    {
        { LCD_5A_SEG, LCD_5A_BIT }, { LCD_5B_SEG, LCD_5B_BIT }, { LCD_5C_SEG, LCD_5C_BIT },
        { LCD_5D_SEG, LCD_5D_BIT }, { LCD_5E_SEG, LCD_5E_BIT }, { LCD_5F_SEG, LCD_5F_BIT },
        { LCD_5G_SEG, LCD_5G_BIT },
    },
    /* 数字 6 (P6: 正向中间第三位/反向第二位数字) */
    {
        { LCD_6A_SEG, LCD_6A_BIT }, { LCD_6B_SEG, LCD_6B_BIT }, { LCD_6C_SEG, LCD_6C_BIT },
        { LCD_6D_SEG, LCD_6D_BIT }, { LCD_6E_SEG, LCD_6E_BIT }, { LCD_6F_SEG, LCD_6F_BIT },
        { LCD_6G_SEG, LCD_6G_BIT },
    },
    /* 数字 7 (P7: 中间正向最后一位/反向第一位数字) */
    {
        { LCD_7A_SEG, LCD_7A_BIT }, { LCD_7B_SEG, LCD_7B_BIT }, { LCD_7C_SEG, LCD_7C_BIT },
        { LCD_7D_SEG, LCD_7D_BIT }, { LCD_7E_SEG, LCD_7E_BIT }, { LCD_7F_SEG, LCD_7F_BIT },
        { LCD_7G_SEG, LCD_7G_BIT },
    },
    /* 数字 8 (P8: 正向第一位数字) */
    {
        { LCD_8A_SEG, LCD_8A_BIT }, { LCD_8B_SEG, LCD_8B_BIT }, { LCD_8C_SEG, LCD_8C_BIT },
        { LCD_8D_SEG, LCD_8D_BIT }, { LCD_8E_SEG, LCD_8E_BIT }, { LCD_8F_SEG, LCD_8F_BIT },
        { LCD_8G_SEG, LCD_8G_BIT },
    },
    /* 数字 9 (P9: 正向第二位数字) */
    {
        { LCD_9A_SEG, LCD_9A_BIT }, { LCD_9B_SEG, LCD_9B_BIT }, { LCD_9C_SEG, LCD_9C_BIT },
        { LCD_9D_SEG, LCD_9D_BIT }, { LCD_9E_SEG, LCD_9E_BIT }, { LCD_9F_SEG, LCD_9F_BIT },
        { LCD_9G_SEG, LCD_9G_BIT },
    },
    /* 数字 10 (P10: 正向第三位数字) */
    {
        { LCD_10A_SEG, LCD_10A_BIT }, { LCD_10B_SEG, LCD_10B_BIT }, { LCD_10C_SEG, LCD_10C_BIT },
        { LCD_10D_SEG, LCD_10D_BIT }, { LCD_10E_SEG, LCD_10E_BIT }, { LCD_10F_SEG, LCD_10F_BIT },
        { LCD_10G_SEG, LCD_10G_BIT },
    },
};

/* ----------------------------------------------------------------------
 *  在位置 pos(1~10) 显示数字 digit(0~9)
 * ---------------------------------------------------------------------- */
void lcd_show_digit(uint8_t pos, uint8_t digit, uint8_t *frame)
{
    const lcd_seg_pin_t *pins;
    uint8_t code;
    uint8_t i;

    if ((pos < 1U) || (pos > 10U) || (digit > 9U))
    {
        return; /* 越界忽略 */
    }

    pins = &lcd_digit_pins[pos - 1U][0];
    code = lcd_digit_code[digit];

    /* 先清除该位置的 7 段, 避免残留 */
    for (i = 0U; i < 7U; i++)
    {
        frame[pins[i].seg] &= (uint8_t)~pins[i].bit;
    }

    /* 再点亮数字所需的段 */
    for (i = 0U; i < 7U; i++)
    {
        if (0U != (code & (uint8_t)(1U << i)))
        {
            frame[pins[i].seg] |= pins[i].bit;
        }
    }
}

/* ----------------------------------------------------------------------
 *  180° 旋转七段码: 用于反向组数字倒置显示。
 *  位映射 (bit0=A ... bit6=G): A<->D, B<->E, C<->F, G<->G
 * ---------------------------------------------------------------------- */
static uint8_t lcd_digit_rotate_code(uint8_t code)
{
    uint8_t r = 0U;

    if (0U != (code & 0x08U)) { r |= 0x01U; } /* A <- D */
    if (0U != (code & 0x10U)) { r |= 0x02U; } /* B <- E */
    if (0U != (code & 0x20U)) { r |= 0x04U; } /* C <- F */
    if (0U != (code & 0x01U)) { r |= 0x08U; } /* D <- A */
    if (0U != (code & 0x02U)) { r |= 0x10U; } /* E <- B */
    if (0U != (code & 0x04U)) { r |= 0x20U; } /* F <- C */
    if (0U != (code & 0x40U)) { r |= 0x40U; } /* G <- G */

    return r;
}

/* ----------------------------------------------------------------------
 *  在位置 pos(1~10) 以 180° 倒置显示数字 digit(0~9)。
 *  用于反向组 (P1~P3) 的倒置数字显示。
 * ---------------------------------------------------------------------- */
void lcd_show_digit_rot(uint8_t pos, uint8_t digit, uint8_t *frame)
{
    const lcd_seg_pin_t *pins;
    uint8_t code;
    uint8_t i;

    if ((pos < 1U) || (pos > 10U) || (digit > 9U))
    {
        return; /* 越界忽略 */
    }

    pins = &lcd_digit_pins[pos - 1U][0];
    code = lcd_digit_rotate_code(lcd_digit_code[digit]);

    /* 先清除该位置的 7 段, 避免残留 */
    for (i = 0U; i < 7U; i++)
    {
        frame[pins[i].seg] &= (uint8_t)~pins[i].bit;
    }

    /* 再点亮旋转后的数字所需段 */
    for (i = 0U; i < 7U; i++)
    {
        if (0U != (code & (uint8_t)(1U << i)))
        {
            frame[pins[i].seg] |= pins[i].bit;
        }
    }
}

/* ----------------------------------------------------------------------
 *  30 个图标的引脚 (T1~T30)
 *  由 lcd_seg_map.h 的宏展开, 与映射表单一来源 (DRY)
 * ---------------------------------------------------------------------- */
static const lcd_seg_pin_t lcd_icon_pins[LCD_ICON_NUM] =
{
    { LCD_T1_SEG,  LCD_T1_BIT  },   /* T1  水平向下的箭头 */
    { LCD_T2_SEG,  LCD_T2_BIT  },   /* T2  水平仪 */
    { LCD_T3_SEG,  LCD_T3_BIT  },   /* T3  更向下的箭头 */
    { LCD_T4_SEG,  LCD_T4_BIT  },   /* T4  一档基准 */
    { LCD_T5_SEG,  LCD_T5_BIT  },   /* T5  二档基准 */
    { LCD_T6_SEG,  LCD_T6_BIT  },   /* T6  最向下的箭头 */
    { LCD_T7_SEG,  LCD_T7_BIT  },   /* T7  十字加号 */
    { LCD_T8_SEG,  LCD_T8_BIT  },   /* T8  三档校准 */
    { LCD_T9_SEG,  LCD_T9_BIT  },   /* T9  电量显示框 */
    { LCD_T10_SEG, LCD_T10_BIT },   /* T10 第三格电量 */
    { LCD_T11_SEG, LCD_T11_BIT },   /* T11 第二格电量 */
    { LCD_T12_SEG, LCD_T12_BIT },   /* T12 第一格电量 */
    { LCD_T13_SEG, LCD_T13_BIT },   /* T13 校准开关 */
    { LCD_T14_SEG, LCD_T14_BIT },   /* T14 反向的 CM */
    { LCD_T15_SEG, LCD_T15_BIT },   /* T15 反向的英寸 */
    { LCD_T16_SEG, LCD_T16_BIT },   /* T16 中间最前面正向的小数点 */
    { LCD_T17_SEG, LCD_T17_BIT },   /* T17 反向的 M */
    { LCD_T18_SEG, LCD_T18_BIT },   /* T18 反向的英尺 */
    { LCD_T19_SEG, LCD_T19_BIT },   /* T19 反向的小数点 */
    { LCD_T20_SEG, LCD_T20_BIT },   /* T20 正向的英尺 */
    { LCD_T21_SEG, LCD_T21_BIT },   /* T21 正向的 M */
    { LCD_T22_SEG, LCD_T22_BIT },   /* T22 正向的第二个小数点 */
    { LCD_T23_SEG, LCD_T23_BIT },   /* T23 反向最后一个小数点 */
    { LCD_T24_SEG, LCD_T24_BIT },   /* T24 斜杠符号 */
    { LCD_T25_SEG, LCD_T25_BIT },   /* T25 正向最后一个小数点 */
    { LCD_T26_SEG, LCD_T26_BIT },   /* T26 反向第二个小数点 */
    { LCD_T27_SEG, LCD_T27_BIT },   /* T27 反向第一个小数点 */
    { LCD_T28_SEG, LCD_T28_BIT },   /* T28 正向的小数点 */
    { LCD_T29_SEG, LCD_T29_BIT },   /* T29 正向的英寸 */
    { LCD_T30_SEG, LCD_T30_BIT },   /* T30 正向的 CM */
};

/* ----------------------------------------------------------------------
 *  点亮第 idx 个图标 (0=T1 ... 29=T30)
 * ---------------------------------------------------------------------- */
void lcd_show_icon(uint8_t idx, uint8_t *frame)
{
    if (idx >= LCD_ICON_NUM)
    {
        return; /* 越界忽略 */
    }

    frame[lcd_icon_pins[idx].seg] |= lcd_icon_pins[idx].bit;
}
