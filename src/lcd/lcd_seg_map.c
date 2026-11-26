/*
 * lcd_seg_map.c
 *
 *  LCD 数字显示实现 (依赖 lcd_seg_map.h 的段映射宏)
 *  新面板: 12 个数字位置 (1~12) + 41 个图标 (T1~T41)。
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
 *  12 个数字位置的七段引脚 (顺序 A B C D E F G)
 *  由 lcd_seg_map.h 的宏展开, 保证与映射表单一来源 (DRY)
 * ---------------------------------------------------------------------- */
static const lcd_seg_pin_t lcd_digit_pins[12][7] =
{
    /* 数字 1 */
    {
        { LCD_1A_SEG, LCD_1A_BIT }, { LCD_1B_SEG, LCD_1B_BIT }, { LCD_1C_SEG, LCD_1C_BIT },
        { LCD_1D_SEG, LCD_1D_BIT }, { LCD_1E_SEG, LCD_1E_BIT }, { LCD_1F_SEG, LCD_1F_BIT },
        { LCD_1G_SEG, LCD_1G_BIT },
    },
    /* 数字 2 */
    {
        { LCD_2A_SEG, LCD_2A_BIT }, { LCD_2B_SEG, LCD_2B_BIT }, { LCD_2C_SEG, LCD_2C_BIT },
        { LCD_2D_SEG, LCD_2D_BIT }, { LCD_2E_SEG, LCD_2E_BIT }, { LCD_2F_SEG, LCD_2F_BIT },
        { LCD_2G_SEG, LCD_2G_BIT },
    },
    /* 数字 3 */
    {
        { LCD_3A_SEG, LCD_3A_BIT }, { LCD_3B_SEG, LCD_3B_BIT }, { LCD_3C_SEG, LCD_3C_BIT },
        { LCD_3D_SEG, LCD_3D_BIT }, { LCD_3E_SEG, LCD_3E_BIT }, { LCD_3F_SEG, LCD_3F_BIT },
        { LCD_3G_SEG, LCD_3G_BIT },
    },
    /* 数字 4 */
    {
        { LCD_4A_SEG, LCD_4A_BIT }, { LCD_4B_SEG, LCD_4B_BIT }, { LCD_4C_SEG, LCD_4C_BIT },
        { LCD_4D_SEG, LCD_4D_BIT }, { LCD_4E_SEG, LCD_4E_BIT }, { LCD_4F_SEG, LCD_4F_BIT },
        { LCD_4G_SEG, LCD_4G_BIT },
    },
    /* 数字 5 */
    {
        { LCD_5A_SEG, LCD_5A_BIT }, { LCD_5B_SEG, LCD_5B_BIT }, { LCD_5C_SEG, LCD_5C_BIT },
        { LCD_5D_SEG, LCD_5D_BIT }, { LCD_5E_SEG, LCD_5E_BIT }, { LCD_5F_SEG, LCD_5F_BIT },
        { LCD_5G_SEG, LCD_5G_BIT },
    },
    /* 数字 6 */
    {
        { LCD_6A_SEG, LCD_6A_BIT }, { LCD_6B_SEG, LCD_6B_BIT }, { LCD_6C_SEG, LCD_6C_BIT },
        { LCD_6D_SEG, LCD_6D_BIT }, { LCD_6E_SEG, LCD_6E_BIT }, { LCD_6F_SEG, LCD_6F_BIT },
        { LCD_6G_SEG, LCD_6G_BIT },
    },
    /* 数字 7 */
    {
        { LCD_7A_SEG, LCD_7A_BIT }, { LCD_7B_SEG, LCD_7B_BIT }, { LCD_7C_SEG, LCD_7C_BIT },
        { LCD_7D_SEG, LCD_7D_BIT }, { LCD_7E_SEG, LCD_7E_BIT }, { LCD_7F_SEG, LCD_7F_BIT },
        { LCD_7G_SEG, LCD_7G_BIT },
    },
    /* 数字 8 */
    {
        { LCD_8A_SEG, LCD_8A_BIT }, { LCD_8B_SEG, LCD_8B_BIT }, { LCD_8C_SEG, LCD_8C_BIT },
        { LCD_8D_SEG, LCD_8D_BIT }, { LCD_8E_SEG, LCD_8E_BIT }, { LCD_8F_SEG, LCD_8F_BIT },
        { LCD_8G_SEG, LCD_8G_BIT },
    },
    /* 数字 9 */
    {
        { LCD_9A_SEG, LCD_9A_BIT }, { LCD_9B_SEG, LCD_9B_BIT }, { LCD_9C_SEG, LCD_9C_BIT },
        { LCD_9D_SEG, LCD_9D_BIT }, { LCD_9E_SEG, LCD_9E_BIT }, { LCD_9F_SEG, LCD_9F_BIT },
        { LCD_9G_SEG, LCD_9G_BIT },
    },
    /* 数字 10 */
    {
        { LCD_10A_SEG, LCD_10A_BIT }, { LCD_10B_SEG, LCD_10B_BIT }, { LCD_10C_SEG, LCD_10C_BIT },
        { LCD_10D_SEG, LCD_10D_BIT }, { LCD_10E_SEG, LCD_10E_BIT }, { LCD_10F_SEG, LCD_10F_BIT },
        { LCD_10G_SEG, LCD_10G_BIT },
    },
    /* 数字 11 */
    {
        { LCD_11A_SEG, LCD_11A_BIT }, { LCD_11B_SEG, LCD_11B_BIT }, { LCD_11C_SEG, LCD_11C_BIT },
        { LCD_11D_SEG, LCD_11D_BIT }, { LCD_11E_SEG, LCD_11E_BIT }, { LCD_11F_SEG, LCD_11F_BIT },
        { LCD_11G_SEG, LCD_11G_BIT },
    },
    /* 数字 12 */
    {
        { LCD_12A_SEG, LCD_12A_BIT }, { LCD_12B_SEG, LCD_12B_BIT }, { LCD_12C_SEG, LCD_12C_BIT },
        { LCD_12D_SEG, LCD_12D_BIT }, { LCD_12E_SEG, LCD_12E_BIT }, { LCD_12F_SEG, LCD_12F_BIT },
        { LCD_12G_SEG, LCD_12G_BIT },
    },
};

/* ----------------------------------------------------------------------
 *  在位置 pos(1~12) 显示数字 digit(0~9)
 * ---------------------------------------------------------------------- */
void lcd_show_digit(uint8_t pos, uint8_t digit, uint8_t *frame)
{
    const lcd_seg_pin_t *pins;
    uint8_t code;
    uint8_t i;

    if ((pos < 1U) || (pos > 12U) || (digit > 9U))
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
 *  在位置 pos(1~12) 以 180° 倒置显示数字 digit(0~9)。
 *  用于反向组的倒置数字显示。
 * ---------------------------------------------------------------------- */
void lcd_show_digit_rot(uint8_t pos, uint8_t digit, uint8_t *frame)
{
    const lcd_seg_pin_t *pins;
    uint8_t code;
    uint8_t i;

    if ((pos < 1U) || (pos > 12U) || (digit > 9U))
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
 *  41 个图标的引脚 (T1~T41)
 *  由 lcd_seg_map.h 的宏展开, 与映射表单一来源 (DRY)
 * ---------------------------------------------------------------------- */
static const lcd_seg_pin_t lcd_icon_pins[LCD_ICON_NUM] =
{
    { LCD_T1_SEG,  LCD_T1_BIT  },   /* T1 */
    { LCD_T2_SEG,  LCD_T2_BIT  },   /* T2 */
    { LCD_T3_SEG,  LCD_T3_BIT  },   /* T3 */
    { LCD_T4_SEG,  LCD_T4_BIT  },   /* T4 */
    { LCD_T5_SEG,  LCD_T5_BIT  },   /* T5 */
    { LCD_T6_SEG,  LCD_T6_BIT  },   /* T6 */
    { LCD_T7_SEG,  LCD_T7_BIT  },   /* T7 */
    { LCD_T8_SEG,  LCD_T8_BIT  },   /* T8 */
    { LCD_T9_SEG,  LCD_T9_BIT  },   /* T9 */
    { LCD_T10_SEG, LCD_T10_BIT },   /* T10 */
    { LCD_T11_SEG, LCD_T11_BIT },   /* T11 */
    { LCD_T12_SEG, LCD_T12_BIT },   /* T12 */
    { LCD_T13_SEG, LCD_T13_BIT },   /* T13 */
    { LCD_T14_SEG, LCD_T14_BIT },   /* T14 */
    { LCD_T15_SEG, LCD_T15_BIT },   /* T15 */
    { LCD_T16_SEG, LCD_T16_BIT },   /* T16 */
    { LCD_T17_SEG, LCD_T17_BIT },   /* T17 */
    { LCD_T18_SEG, LCD_T18_BIT },   /* T18 */
    { LCD_T19_SEG, LCD_T19_BIT },   /* T19 */
    { LCD_T20_SEG, LCD_T20_BIT },   /* T20 */
    { LCD_T21_SEG, LCD_T21_BIT },   /* T21 */
    { LCD_T22_SEG, LCD_T22_BIT },   /* T22 */
    { LCD_T23_SEG, LCD_T23_BIT },   /* T23 */
    { LCD_T24_SEG, LCD_T24_BIT },   /* T24 */
    { LCD_T25_SEG, LCD_T25_BIT },   /* T25 */
    { LCD_T26_SEG, LCD_T26_BIT },   /* T26 */
    { LCD_T27_SEG, LCD_T27_BIT },   /* T27 */
    { LCD_T28_SEG, LCD_T28_BIT },   /* T28 */
    { LCD_T29_SEG, LCD_T29_BIT },   /* T29 */
    { LCD_T30_SEG, LCD_T30_BIT },   /* T30 */
    { LCD_T31_SEG, LCD_T31_BIT },   /* T31 */
    { LCD_T32_SEG, LCD_T32_BIT },   /* T32 */
    { LCD_T33_SEG, LCD_T33_BIT },   /* T33 */
    { LCD_T34_SEG, LCD_T34_BIT },   /* T34 */
    { LCD_T35_SEG, LCD_T35_BIT },   /* T35 */
    { LCD_T36_SEG, LCD_T36_BIT },   /* T36 */
    { LCD_T37_SEG, LCD_T37_BIT },   /* T37 */
    { LCD_T38_SEG, LCD_T38_BIT },   /* T38 */
    { LCD_T39_SEG, LCD_T39_BIT },   /* T39 */
    { LCD_T40_SEG, LCD_T40_BIT },   /* T40 */
    { LCD_T41_SEG, LCD_T41_BIT },   /* T41 */
};

/* ----------------------------------------------------------------------
 *  点亮第 idx 个图标 (0=T1 ... 40=T41)
 * ---------------------------------------------------------------------- */
void lcd_show_icon(uint8_t idx, uint8_t *frame)
{
    if (idx >= LCD_ICON_NUM)
    {
        return; /* 越界忽略 */
    }

    frame[lcd_icon_pins[idx].seg] |= lcd_icon_pins[idx].bit;
}
