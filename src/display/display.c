/*
 * display.c
 *
 *  LCD 应用显示层实现: 图标形式 + 7 态单位换算。
 *  只操作 lcd_frame 段码缓冲, 最终调用 lcd_write_pattern 整块刷屏。
 */

#include "display.h"
#include "lcd/lcd.h"
#include "lcd/lcd_seg_map.h"
#include "config.h"
#include "param/param.h"
#include <stdio.h>
#include <string.h>

/* 基准点闪烁状态: 灭相标志 (true=灭) 与 50ms 节拍计数。
 * 收到距离数据触发一次闪烁 (灭 250ms), tick 到点后恢复常亮。 */
static bool    s_blink_phase = false;
static uint8_t s_blink_cnt   = 0U;

/* ======================================================================
 *  单位图标索引 (lcd_show_icon 0-based, 新面板 T1~T41 语义)
 * ====================================================================== */
#define LCD_ICON_FWD_FT   (37U)   /* T38 正向 英尺 */
#define LCD_ICON_FWD_IN   (38U)   /* T39 正向 英寸 */
#define LCD_ICON_FWD_M    (39U)   /* T40 正向 M */
#define LCD_ICON_FWD_MM   (40U)   /* T41 正向 MM */
#define LCD_ICON_REV_FT   (20U)   /* T21 反向 英尺 */
#define LCD_ICON_REV_IN   (19U)   /* T20 反向 英寸 */
#define LCD_ICON_REV_M    (18U)   /* T19 反向 M */
#define LCD_ICON_REV_MM   (17U)   /* T18 反向 MM */
#define LCD_ICON_SLASH_FWD (24U)   /* T25 正向(下排)分数斜杠 */
#define LCD_ICON_SLASH_REV (33U)   /* T34 反向(上排)分数斜杠 */
#define LCD_ICON_T7       (7U)    /* T8  十字加号 (t7 激光十字线指示; 三档基准为 T7=idx6) */
#define LCD_ICON_QUOTE_FWD   (36U)   /* T37 正向 单引号 (英尺标记) */
#define LCD_ICON_QUOTE_REV   (21U)   /* T22 反向 单引号 */
#define LCD_ICON_DQUOTE_FWD  (30U)   /* T31 正向 双引号 (英寸标记) */
#define LCD_ICON_DQUOTE_REV  (16U)   /* T17 反向 双引号 */
#define LCD_ICON_DOT_BOT_FWD (22U)   /* T23 正向下排数字第一个小数点 */
#define LCD_ICON_DOT_BOT_REV (23U)   /* T24 正向下排数字反过来的第一个小数点 */
#define LCD_ICON_DOT_TOP_REV (35U)   /* T36 反向显示时上排小数点 (1#/4# 反向小数) */
#define LCD_ICON_SEP_FWD     (28U)   /* T29 正向 引号 (mm>9999 米/毫米分隔符) */
#define LCD_ICON_SEP_REV     (13U)   /* T14 反向 引号 (mm>9999 米/毫米分隔符) */

/* 单位枚举: 用于 lcd_show_unit 选择单位图标 */
typedef enum
{
    UNIT_FT = 0,   /* 英尺 */
    UNIT_IN,       /* 英寸 */
    UNIT_M,        /* 米   */
    UNIT_MM,       /* 毫米 */
} unit_t;

/* ----------------------------------------------------------------------
 *  毫米 -> 千分之一英尺 (0.001 ft), 四舍五入:
 *     value_mm * 1000 / 304.8 = (value_mm * 10000 + 1524) / 3048
 *  四舍五入可保证档位减法后米态显示精确恢复原值 (消除 1mm 累积误差)。
 * ---------------------------------------------------------------------- */
static uint32_t mm_to_ft_milli(uint32_t value_mm)
{
    return (value_mm * 10000UL + 1524UL) / 3048UL;
}

/* ----------------------------------------------------------------------
 *  端部组显示整数部分 (右对齐, 高位靠左):
 *    正向: 上排 1/2/3/4 正立, 右对齐到 pos4 (16 -> pos3=1, pos4=6)
 *    反向: 下排 9/10/11/12 倒置, 右对齐到 pos9 (16 -> pos10=1, pos9=6)
 * ---------------------------------------------------------------------- */
static void lcd_show_int_group(uint8_t *frame, const char *num,
                               uint8_t int_len, bool forward)
{
    uint8_t k;

    /* 最多显示 4 位整数, 从最低位 (最右) 往左填充 */
    for (k = 0U; (k < int_len) && (k < 4U); k++)
    {
        uint8_t digit = (uint8_t)(num[int_len - 1U - k] - '0');

        if (forward)
        {
            lcd_show_digit((uint8_t)(4U - k), digit, frame);     /* pos4 <- 最低位 */
        }
        else
        {
            lcd_show_digit_rot((uint8_t)(9U + k), digit, frame); /* pos9 <- 最低位 */
        }
    }
}

/* ----------------------------------------------------------------------
 *  MM 态整数毫米显示: 端部排 + 中排 8 位窗口, 右对齐。
 *    正向: 上排 pos1~4 + 中排 pos5~8, 个位 pos8。
 *    反向: 下排 pos9~12 + 中排 pos5~8, 个位 pos5。
 *    超过 8 位时仅取最低 8 位 (量程内应不超 99999999mm)。
 * ---------------------------------------------------------------------- */
static void lcd_show_mm_group(uint8_t *frame, const char *num,
                              uint8_t len, bool forward)
{
    static const uint8_t fwd_pos[8] = {8U, 7U, 6U, 5U, 4U, 3U, 2U, 1U};   /* index0=个位 */
    static const uint8_t rev_pos[8] = {5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U}; /* index0=个位 */
    uint8_t k;

    for (k = 0U; (k < len) && (k < 8U); k++)
    {
        uint8_t digit = (uint8_t)(num[len - 1U - k] - '0');

        if (forward)
        {
            lcd_show_digit(fwd_pos[k], digit, frame);
        }
        else
        {
            lcd_show_digit_rot(rev_pos[k], digit, frame);
        }
    }
}

/* ----------------------------------------------------------------------
 *  中排显示整数部分 (右对齐, 高位靠左):
 *    正向: 中排 5/6/7/8 正立, 右对齐到 pos8 (131 -> pos6/7/8)
 *    反向: 中排 5/6/7/8 倒置, 右对齐到 pos5 (131 -> pos5/6/7)
 * ---------------------------------------------------------------------- */
static void lcd_show_mid_int(uint8_t *frame, uint32_t val, bool forward)
{
    char    num[8];
    uint8_t len, k;

    sprintf(num, "%lu", (unsigned long)val);
    len = (uint8_t)strlen(num);

    for (k = 0U; (k < len) && (k < 4U); k++)
    {
        uint8_t digit = (uint8_t)(num[len - 1U - k] - '0');

        if (forward)
        {
            lcd_show_digit((uint8_t)(8U - k), digit, frame);   /* 个位 pos8 */
        }
        else
        {
            lcd_show_digit_rot((uint8_t)(5U + k), digit, frame); /* 个位 pos5 */
        }
    }
}

/* ----------------------------------------------------------------------
 *  下排显示小数数字 (最多 3 位):
 *    正向: 下排 pos10/11/12, 小数点 T23, 数字紧跟小数点左起。
 *    反向: 上排 1/2/3/4 倒置, 小数点 T36, 数字右对齐到 pos1。
 * ---------------------------------------------------------------------- */
static void lcd_show_bot_decimal(uint8_t *frame, uint32_t frac_val,
                                 uint8_t frac_len, bool forward)
{
    char    num[4];
    uint8_t k;

    if (frac_len > 3U)
    {
        frac_len = 3U;
    }
    sprintf(num, "%0*lu", (int)frac_len, (unsigned long)frac_val);

    if (forward)
    {
        /* 正向: 显示小数点 T23 + 小数数字 (1#/4# 正向 .125/.136) */
        lcd_show_icon(LCD_ICON_DOT_BOT_FWD, frame);   /* T23 正向下排小数点 */
        for (k = 0U; k < frac_len; k++)
        {
            lcd_show_digit((uint8_t)(10U + k), (uint8_t)(num[k] - '0'), frame);
        }
    }
    else
    {
        /* 反向: 显示小数点 T36 + 小数数字 (1#/4# 反向 .125/.136) */
        lcd_show_icon(LCD_ICON_DOT_TOP_REV, frame);   /* T36 反向上排小数点 */
        for (k = 0U; k < frac_len; k++)
        {
            lcd_show_digit_rot((uint8_t)(3U - k), (uint8_t)(num[k] - '0'), frame);
        }
    }
}

/* ----------------------------------------------------------------------
 *  中排显示小数部分 (小数点 + 小数数字, 最多 4 位):
 *    中排 5/6/7/8 正反共享; 小数点固定 T27(正向)/T28(反向)。
 *    数字从小数点右侧第一位开始连续排布:
 *      1 位: 正向 pos7,             反向 pos6
 *      2 位: 正向 pos7/8,           反向 pos6/5
 *      3 位: 正向 pos6/7/8,         反向 pos7/6/5
 *      4 位: 正向 pos5/6/7/8,       反向 pos8/7/6/5
 * ---------------------------------------------------------------------- */
static void lcd_show_frac_group(uint8_t *frame, const char *num,
                                uint8_t int_len, uint8_t len, bool forward)
{
    uint8_t frac_len = (uint8_t)(len - int_len);
    const char *frac = &num[int_len];
    uint8_t k;

    /* 最多显示 4 位小数 */
    if (frac_len > 4U)
    {
        frac_len = 4U;
    }
    if (0U == frac_len)
    {
        return;
    }

    /* 查表索引: [0]=1位, [1]=2位, [2]=3位, [3]=4位 */
    static const uint8_t fwd_start[4] = {7U, 7U, 6U, 5U};
    static const uint8_t fwd_dot[4]   = {26U, 26U, 26U, 26U};  /* T27 */
    static const uint8_t rev_start[4] = {6U, 6U, 7U, 8U};
    static const uint8_t rev_dot[4]   = {27U, 27U, 27U, 27U};  /* T28 */

    uint8_t idx = (uint8_t)(frac_len - 1U);

    if (forward)
    {
        lcd_show_icon(fwd_dot[idx], frame);
        for (k = 0U; k < frac_len; k++)
        {
            uint8_t digit = (uint8_t)(frac[k] - '0');
            lcd_show_digit((uint8_t)(fwd_start[idx] + k), digit, frame);
        }
    }
    else
    {
        lcd_show_icon(rev_dot[idx], frame);
        for (k = 0U; k < frac_len; k++)
        {
            uint8_t digit = (uint8_t)(frac[k] - '0');
            lcd_show_digit_rot((uint8_t)(rev_start[idx] - k), digit, frame);
        }
    }
}

/* ----------------------------------------------------------------------
 *  单位图标显示: 根据单位与方向点亮对应的单位图标
 * ---------------------------------------------------------------------- */
static void lcd_show_unit(uint8_t *frame, unit_t unit, bool forward)
{
    if (forward)
    {
        switch (unit)
        {
            case UNIT_FT: lcd_show_icon(LCD_ICON_FWD_FT, frame); break;
            case UNIT_IN: lcd_show_icon(LCD_ICON_FWD_IN, frame); break;
            case UNIT_M:  lcd_show_icon(LCD_ICON_FWD_M,  frame); break;
            case UNIT_MM: lcd_show_icon(LCD_ICON_FWD_MM, frame); break;
            default: break;
        }
    }
    else
    {
        switch (unit)
        {
            case UNIT_FT: lcd_show_icon(LCD_ICON_REV_FT, frame); break;
            case UNIT_IN: lcd_show_icon(LCD_ICON_REV_IN, frame); break;
            case UNIT_M:  lcd_show_icon(LCD_ICON_REV_M,  frame); break;
            case UNIT_MM: lcd_show_icon(LCD_ICON_REV_MM, frame); break;
            default: break;
        }
    }
}

/* ----------------------------------------------------------------------
 *  英尺/英寸单位标记 (单引号/双引号):
 *    正向: T37 单引号 / T31 双引号; 反向: T22 单引号 / T17 双引号。
 * ---------------------------------------------------------------------- */
static void lcd_show_ft_mark(uint8_t *frame, bool forward)
{
    lcd_show_icon(forward ? LCD_ICON_QUOTE_FWD : LCD_ICON_QUOTE_REV, frame);
}

static void lcd_show_in_mark(uint8_t *frame, bool forward)
{
    lcd_show_icon(forward ? LCD_ICON_DQUOTE_FWD : LCD_ICON_DQUOTE_REV, frame);
}

/* ----------------------------------------------------------------------
 *  约分: num/den 除以最大公约数。
 * ---------------------------------------------------------------------- */
static void frac_reduce(uint32_t num, uint32_t den, uint8_t *p_num, uint8_t *p_den)
{
    uint32_t a = num, b = den, t;

    while (0U != b)
    {
        t = a % b;
        a = b;
        b = t;
    }

    *p_num = (uint8_t)(num / a);
    *p_den = (uint8_t)(den / a);
}

/* ----------------------------------------------------------------------
 *  分数显示 num/den (支持两位分子/分母, 约分后调用, 如 15/16、7/8):
 *    正向: 分数在下排 pos9~12, 斜杠 T25; 分子右对齐紧贴斜杠, 分母左对齐紧贴斜杠:
 *      分子十位 pos9/个位 pos10; 分母两位十位 pos11/个位 pos12, 一位时个位 pos11
 *    反向: 分数在上排 pos1~4 (倒置), 斜杠 T34; 倒置后分子在右、分母在左:
 *      分子十位 pos4/个位 pos3; 分母两位十位 pos2/个位 pos1, 一位时个位 pos2
 * ---------------------------------------------------------------------- */
static void lcd_show_fraction2(uint8_t *frame, uint8_t num, uint8_t den, bool forward)
{
    uint8_t n_tens = (uint8_t)(num / 10U);   /* 分子十位 (0~1) */
    uint8_t n_ones = (uint8_t)(num % 10U);   /* 分子个位 */
    uint8_t d_tens = (uint8_t)(den / 10U);   /* 分母十位 (0~1) */
    uint8_t d_ones = (uint8_t)(den % 10U);   /* 分母个位 */

    if (forward)
    {
        lcd_show_icon(LCD_ICON_SLASH_FWD, frame);   /* T25 下排斜杠 */
        if (0U != n_tens) { lcd_show_digit(9U, n_tens, frame); }  /* 分子十位 pos9 */
        lcd_show_digit(10U, n_ones, frame);                        /* 分子个位 pos10 (紧贴斜杠) */
        if (0U != d_tens)                                        /* 分母两位 */
        {
            lcd_show_digit(11U, d_tens, frame);                    /* 分母十位 pos11 (紧贴斜杠) */
            lcd_show_digit(12U, d_ones, frame);                    /* 分母个位 pos12 */
        }
        else                                                      /* 分母一位, 紧贴斜杠 */
        {
            lcd_show_digit(11U, d_ones, frame);                    /* 分母个位 pos11 */
        }
    }
    else
    {
        lcd_show_icon(LCD_ICON_SLASH_REV, frame);   /* T34 上排斜杠 */
        if (0U != n_tens) { lcd_show_digit_rot(4U, n_tens, frame); }  /* 分子十位 pos4 */
        lcd_show_digit_rot(3U, n_ones, frame);                        /* 分子个位 pos3 (紧贴斜杠) */
        if (0U != d_tens)                                             /* 分母两位 */
        {
            lcd_show_digit_rot(2U, d_tens, frame);                    /* 分母十位 pos2 (紧贴斜杠) */
            lcd_show_digit_rot(1U, d_ones, frame);                    /* 分母个位 pos1 */
        }
        else                                                          /* 分母一位, 紧贴斜杠 */
        {
            lcd_show_digit_rot(2U, d_ones, frame);                    /* 分母个位 pos2 */
        }
    }
}

/* ----------------------------------------------------------------------
 *  通用十进制显示: 端部组整数 + 中间组小数 (带小数点)。
 *    int_val 整数值, frac_val 小数值(去掉小数点), frac_len 小数位数(0=无小数)。
 *    frac_len=0 时仅显示整数 (不点亮小数点)。
 * ---------------------------------------------------------------------- */
static void lcd_show_decimal(uint8_t *frame, uint32_t int_val,
                             uint32_t frac_val, uint8_t frac_len, bool forward)
{
    char    digits[12];
    uint8_t int_len;

    if (0U == frac_len)
    {
        sprintf(digits, "%lu", (unsigned long)int_val);
        int_len = (uint8_t)strlen(digits);
        lcd_show_int_group(frame, digits, int_len, forward);
    }
    else
    {
        sprintf(digits, "%lu%0*lu", (unsigned long)int_val,
                (int)frac_len, (unsigned long)frac_val);
        int_len = (uint8_t)(strlen(digits) - frac_len);
        lcd_show_int_group(frame, digits, int_len, forward);
        lcd_show_frac_group(frame, digits, int_len,
                            (uint8_t)(int_len + frac_len), forward);
    }
}

/* ----------------------------------------------------------------------
 *  英尺+英寸 公共部分: 计算并渲染上排(英尺整数 + FT + IN)与中排(英寸整数 + 双引号)。
 *  返回 1/8 英寸分子 n8 (0~7, 0=无零头), 由调用方据此渲染下排分数或小数。
 * ---------------------------------------------------------------------- */
static uint32_t lcd_show_ft_in_group(uint8_t *frame, uint32_t base_ft, bool forward)
{
    uint32_t ft_int   = base_ft / 1000UL;
    uint32_t in_milli = (base_ft % 1000UL) * 12UL;
    uint32_t in_int   = in_milli / 1000UL;
    uint32_t n8       = ((in_milli % 1000UL) * 8UL + 500UL) / 1000UL;

    if (n8 >= 8UL)                    /* 分数进位到整数英寸 */
    {
        in_int += 1UL;
        n8      = 0UL;
    }
    if (in_int >= 12UL)               /* 整数英寸进位到英尺 */
    {
        ft_int += 1UL;
        in_int  = 0UL;
    }

    /* 上排: 英尺整数 + 单引号 + FT 单位 + IN 单位 (双单位) */
    lcd_show_decimal(frame, ft_int, 0U, 0U, forward);
    lcd_show_ft_mark(frame, forward);   /* 单引号 T37 / T22 */
    lcd_show_unit(frame, UNIT_FT, forward);
    lcd_show_unit(frame, UNIT_IN, forward);

    /* 中排: 英寸整数 + 双引号 */
    lcd_show_mid_int(frame, in_int, forward);
    lcd_show_in_mark(frame, forward);

    return n8;
}

/* ----------------------------------------------------------------------
 *  7 态换算显示: 根据 base_ft (0.001 ft) 动态换算并显示对应状态。
 *    0: 英尺 + 英寸 (1/8 分数)   上排 英尺整数+FT+IN, 中排 英寸整数+双引号, 下排 分数
 *    1: 英尺 + 英寸 (十进制小数) 上排 英尺整数+FT+IN, 中排 英寸整数+双引号, 下排 小数
 *    2: 十进制英尺               上排 整数+单引号+FT, 中排 小数
 *    3: 英寸 + 分数 (1/16, 约分) 上排 仅 IN, 中排 整数+双引号, 下排 分数
 *    4: 十进制英寸               上排 仅 IN, 中排 整数+双引号, 下排 小数
 *    5: 米 (有效小数位)
 *    6: 毫米 (整数)
 * ---------------------------------------------------------------------- */
static void lcd_convert_show(uint8_t *frame, digital_t state,
                             uint32_t base_ft, bool forward)
{
    switch (state)
    {
        case DIGIT_FT_IN_FRAC:   /* 0# 英尺 + 英寸 (1/8 分数) */
        {
            uint32_t n8 = lcd_show_ft_in_group(frame, base_ft, forward);
            if (0U != n8)
            {
                uint8_t num, den;
                frac_reduce(n8, 8UL, &num, &den);
                lcd_show_fraction2(frame, num, den, forward);
            }
            break;
        }

        case DIGIT_FT_IN_DEC:   /* 1# 英尺 + 英寸 (十进制小数) */
        {
            uint32_t n8 = lcd_show_ft_in_group(frame, base_ft, forward);
            if (0U != n8)
            {
                lcd_show_bot_decimal(frame, n8 * 125UL, 3U, forward);
            }
            break;
        }

        case DIGIT_FT_DEC:   /* 2# 十进制英尺: 上排整数 + 单引号 + FT, 中排小数 */
        {
            uint32_t ft_int   = base_ft / 1000UL;
            uint32_t frac     = base_ft % 1000UL;
            uint8_t  frac_len = 3U;
            while ((frac_len > 0U) && (0U == (frac % 10UL)))
            {
                frac /= 10UL;
                frac_len--;
            }
            lcd_show_decimal(frame, ft_int, frac, frac_len, forward);
            lcd_show_ft_mark(frame, forward);   /* 单引号 T37 / T22 */
            lcd_show_unit(frame, UNIT_FT, forward);
            break;
        }

        case DIGIT_IN_FRAC:   /* 3# 英寸 + 分数 (1/16, 约分, 零头隐藏) */
        {
            uint32_t total_in = base_ft * 12UL;
            uint32_t in_int   = total_in / 1000UL;
            uint32_t n16      = ((total_in % 1000UL) * 16UL + 500UL) / 1000UL;
            if (n16 >= 16UL)                 /* 四舍五入进位到整数英寸 */
            {
                in_int += 1UL;
                n16     = 0UL;
            }

            lcd_show_unit(frame, UNIT_IN, forward);   /* 上排仅 IN 单位 */
            lcd_show_mid_int(frame, in_int, forward);  /* 中排整数 */
            lcd_show_in_mark(frame, forward);          /* 双引号 T31 / T17 */

            if (0U != n16)
            {
                uint8_t num, den;
                frac_reduce(n16, 16UL, &num, &den);
                lcd_show_fraction2(frame, num, den, forward);   /* 下排分数 */
            }
            break;
        }

        case DIGIT_IN_DEC:   /* 4# 十进制英寸: 上排仅 IN 单位, 中排整数 + 双引号, 下排小数 */
        {
            uint32_t total_in = base_ft * 12UL;
            uint32_t in_int   = total_in / 1000UL;
            uint32_t in_frac  = total_in % 1000UL;
            uint8_t  frac_len = 3U;
            while ((frac_len > 0U) && (0U == (in_frac % 10UL)))
            {
                in_frac /= 10UL;
                frac_len--;
            }

            lcd_show_unit(frame, UNIT_IN, forward);   /* 上排仅 IN 单位 */
            lcd_show_mid_int(frame, in_int, forward);  /* 中排整数 */
            lcd_show_in_mark(frame, forward);          /* 双引号 T31 / T17 */

            if (0U != frac_len)
            {
                lcd_show_bot_decimal(frame, in_frac, frac_len, forward);
            }
            break;
        }

        case DIGIT_M:   /* 5# 米 (有效小数位) */
        {
            uint32_t m_milli  = (base_ft * 3048UL + 5000UL) / 10000UL;
            uint32_t m_int    = m_milli / 1000UL;
            uint32_t m_frac   = m_milli % 1000UL;
            uint8_t  frac_len = 3U;
            while ((frac_len > 0U) && (0U == (m_frac % 10UL)))
            {
                m_frac /= 10UL;
                frac_len--;
            }
            lcd_show_decimal(frame, m_int, m_frac, frac_len, forward);
            lcd_show_unit(frame, UNIT_M, forward);
            break;
        }

        case DIGIT_MM:   /* 6# 毫米 (整数) */
        {
            uint32_t m_milli = (base_ft * 3048UL + 5000UL) / 10000UL;  /* 毫米 */

            if (m_milli > 9999UL)
            {
                /* >9999mm (≥10m): 米整数(中排) + 分隔符(T29/T14) + 毫米3位(端部排)
                 * 正向: 12 | 555 => 中排 pos7/8 + T29 + 下排 pos10/11/12
                 * 反向: 12 | 555 => 中排 pos6/5 + T14 + 上排 pos3/2/1 (倒置) */
                uint32_t m_int   = m_milli / 1000UL;   /* 米整数 */
                uint32_t mm_frac = m_milli % 1000UL;   /* 毫米 3 位 */
                char     mm[4];
                uint8_t  k;

                lcd_show_mid_int(frame, m_int, forward);
                lcd_show_icon(forward ? LCD_ICON_SEP_FWD : LCD_ICON_SEP_REV, frame);

                sprintf(mm, "%03lu", (unsigned long)mm_frac);
                for (k = 0U; k < 3U; k++)
                {
                    uint8_t digit = (uint8_t)(mm[k] - '0');
                    if (forward)
                    {
                        lcd_show_digit((uint8_t)(10U + k), digit, frame);
                    }
                    else
                    {
                        lcd_show_digit_rot((uint8_t)(3U - k), digit, frame);
                    }
                }
            }
            else
            {
                /* ≤9999mm: 整数毫米显示在「端部排 + 中排」8 位窗口, 右对齐 */
                char     digits[12];
                sprintf(digits, "%lu", (unsigned long)m_milli);
                lcd_show_mm_group(frame, digits, (uint8_t)strlen(digits), forward);
            }

            lcd_show_unit(frame, UNIT_MM, forward);   /* MM 图标 (正向 T41 / 反向 T18) */
            break;
        }

        default:
            break;
    }
}

/* ----------------------------------------------------------------------
 *  状态机: 初始化到开机初始态并刷屏。
 * ---------------------------------------------------------------------- */
void ui_state_init(ui_state_t *s)
{
    s->form          = FORM_IDLE;
    s->digital       = (digital_t)param_get_digital();   /* 记忆上次单位 */
    s->t7            = T7_OFF;
    s->direction     = DIR_FORWARD;
    s->value_ft      = 0U;
    s->battery_level = 3U;
    s->sw4_first     = true;
    s->ldm_on        = false;
    s->ldm_ever_on   = false;

    display_refresh(s);
}

/* ----------------------------------------------------------------------
 *  状态机: 分派一个 UI 事件, 执行状态转移并刷屏。
 * ---------------------------------------------------------------------- */
void ui_state_dispatch(ui_state_t *s, ui_event_t ev)
{
    switch (ev)
    {
        case UI_EVT_SW1_SHORT:
            /* 数字态循环: 0 -> 1 -> ... -> 6 -> 0 */
            s->digital = (digital_t)(((uint8_t)s->digital + 1U) % (uint8_t)DIGIT_NUM);
            param_set_digital((uint8_t)s->digital);   /* 记忆切换后的单位 */
            break;

        case UI_EVT_SW2_SHORT:
            /* LDM 从未开启过时 SW2 无效, 不切换形态 */
            if (!s->ldm_ever_on)
            {
                break;
            }

            /* 图标形式循环: IDLE -> A -> B -> C -> A */
            if (FORM_IDLE == s->form)
            {
                s->form = FORM_A;
            }
            else
            {
                s->form = (form_t)((uint8_t)FORM_A +
                          (((uint8_t)s->form - (uint8_t)FORM_A + 1U) %
                           ((uint8_t)FORM_NUM - 1U)));
            }
            break;

        case UI_EVT_SW3_SHORT:
            /* LDM 测量开/关往复: 开启瞬间基准点亮 */
            s->ldm_on = !s->ldm_on;
            if (s->ldm_on)
            {
                s_blink_phase  = false;   /* 开 LDM 不闪, 基准点常亮, 收到数据才闪 */
                s->ldm_ever_on = true;    /* 记录: LDM 曾开启过 */
            }
            break;

        case UI_EVT_SW4_SHORT:
            /* 首次按下忽略, 之后 T7 十字加号往复切换 */
            if (s->sw4_first)
            {
                s->sw4_first = false;
            }
            else
            {
                s->t7 = (T7_ON == s->t7) ? T7_OFF : T7_ON;
            }
            break;

        default:
            break;
    }

    display_refresh(s);
}

/* ----------------------------------------------------------------------
 *  状态机: 设置电池电量格数, 变化时才刷屏。
 * ---------------------------------------------------------------------- */
void ui_state_set_battery(ui_state_t *s, uint8_t level)
{
    if (level > 3U)
    {
        level = 3U;
    }

    if (level != s->battery_level)
    {
        s->battery_level = level;
        display_refresh(s);
    }
}

/* ----------------------------------------------------------------------
 *  状态机: 关闭 LDM 测量 (若已开)。用于无操作超时自动关闭。
 *  返回 true 表示实际执行了关闭 (调用方需补发 P002 脉冲通知外部 MCU)。
 * ---------------------------------------------------------------------- */
bool ui_state_ldm_off(ui_state_t *s)
{
    if (s->ldm_on)
    {
        s->ldm_on = false;
        display_refresh(s);
        return true;
    }
    return false;
}

/* ----------------------------------------------------------------------
 *  状态机: 关闭 T7 十字加号 (若已显示)。用于无操作超时自动关闭。
 *  display_refresh 内含 lcd_t7_ctrl(false), 即 P000 拉低关 laser。
 * ---------------------------------------------------------------------- */
void ui_state_t7_off(ui_state_t *s)
{
    if (T7_ON == s->t7)
    {
        s->t7 = T7_OFF;
        display_refresh(s);
    }
}

/* ----------------------------------------------------------------------
 *  状态机: 设置显示方向, 方向变化时才刷屏。
 * ---------------------------------------------------------------------- */
void ui_state_set_direction(ui_state_t *s, direction_t dir)
{
    if (dir != s->direction)
    {
        s->direction = dir;
        display_refresh(s);
    }
}

/* ----------------------------------------------------------------------
 *  状态机: 设置测量值 (单位毫米), 内部换算为 A 档基准 (0.001 ft)。
 *  仅 LDM 开启时接受并显示; 未开启则静默忽略串口数据。
 *  开机 IDLE 态收到首个数值时自动进入 A 形态, 单位保持记忆值 (param)。
 * ---------------------------------------------------------------------- */
void ui_state_set_value(ui_state_t *s, uint32_t value_mm)
{
    /* 未开 LDM 不显示距离 (收到数据直接忽略) */
    if (!s->ldm_on)
    {
        return;
    }

    s->value_ft = mm_to_ft_milli(value_mm);

    /* 收到数据闪一次: 置灭相并清零计数, 由 blink_tick 在 250ms 后恢复 */
    s_blink_phase = true;
    s_blink_cnt   = 0U;

    if (FORM_IDLE == s->form)
    {
        s->form = FORM_A;
    }

    display_refresh(s);
}

/* ----------------------------------------------------------------------
 *  渲染: 按当前状态刷新整屏。
 * ---------------------------------------------------------------------- */
void display_refresh(const ui_state_t *s)
{
    uint8_t frame[LCD_SEG_NUM];
    bool    forward = (DIR_FORWARD == s->direction);

    memset(frame, LCD_SEG_OFF, sizeof(frame));

    /* 常亮固定图标: T2 (水平仪) + T9 (电量显示框) */
    lcd_show_icon(1U,  frame);  /* T2  水平仪 */
    lcd_show_icon(8U,  frame);  /* T9  电量显示框 */

    /* 电量格: 满 3 格 (T10+T11+T12), 2 格 (T10+T11), 1 格 (T10), 0 格全灭。
     * 新面板: T10 第一格(最低), T11 第二格, T12 第三格(最高)。 */
    if (s->battery_level >= 1U)
    {
        lcd_show_icon(9U,  frame);  /* T10 第一格 (最低) */
    }
    if (s->battery_level >= 2U)
    {
        lcd_show_icon(10U, frame);  /* T11 第二格 */
    }
    if (s->battery_level >= 3U)
    {
        lcd_show_icon(11U, frame);  /* T12 第三格 (最高) */
    }

    /* 按形式的图标组合
     *   箭头始终显示;
     *   基准点默认常亮, 收到数据时闪一次 (灭相短暂熄灭)。 */
    switch (s->form)
    {
        case FORM_IDLE:
            lcd_show_icon(0U, frame);    /* T1 水平向下的箭头 */
            break;

        case FORM_A:
            lcd_show_icon(0U, frame);    /* T1 水平向下的箭头 */
            if (!s_blink_phase)
            {
                lcd_show_icon(3U,  frame);   /* T4  一档基准 */
                lcd_show_icon(4U,  frame);   /* T5  二档基准 */
                lcd_show_icon(6U,  frame);   /* T7  三档基准 */
                lcd_show_icon(12U, frame);   /* T13 校准开关 */
            }
            break;

        case FORM_B:
            lcd_show_icon(2U, frame);    /* T3 更向下的箭头 */
            if (!s_blink_phase)
            {
                lcd_show_icon(4U,  frame);   /* T5  二档基准 */
                lcd_show_icon(6U,  frame);   /* T7  三档基准 */
                lcd_show_icon(12U, frame);   /* T13 校准开关 */
            }
            break;

        case FORM_C:
            lcd_show_icon(5U, frame);    /* T6 最向下的箭头 */
            if (!s_blink_phase)
            {
                lcd_show_icon(6U,  frame);   /* T7  三档基准 */
                lcd_show_icon(12U, frame);   /* T13 校准开关 */
            }
            break;

        default:
            break;
    }

    /* 数字 7 态换算显示: 仅非 IDLE 形态显示 */
    if (FORM_IDLE != s->form)
    {
        /* 按形态计算档位长度: B = A - 0.125ft, C = A - 0.375ft */
        uint32_t base_ft = s->value_ft;
        if (FORM_B == s->form)
        {
            base_ft -= FORM_OFFSET_B;
        }
        else if (FORM_C == s->form)
        {
            base_ft -= FORM_OFFSET_C;
        }
        lcd_convert_show(frame, s->digital, base_ft, forward);
    }

    /* T7 十字加号: 独立于形态/数字, 同步驱动 P000 指示输出 */
    if (T7_ON == s->t7)
    {
        lcd_show_icon(LCD_ICON_T7, frame);
        lcd_t7_ctrl(true);
    }
    else
    {
        lcd_t7_ctrl(false);
    }

    lcd_write_pattern(frame, LCD_SEG_NUM);
}

/* ----------------------------------------------------------------------
 *  基准点闪烁节拍: 由 50ms 任务周期调用。
 *  收到数据置灭相后, 每 LCD_BLINK_TICK_CNT 次 (250ms) 恢复常亮并重绘。
 * ---------------------------------------------------------------------- */
void ui_state_blink_tick(ui_state_t *s)
{
    /* 仅闪烁灭相时倒计时, 到点恢复常亮 */
    if (!s_blink_phase)
    {
        return;
    }

    s_blink_cnt++;
    if (s_blink_cnt >= LCD_BLINK_TICK_CNT)
    {
        s_blink_cnt   = 0U;
        s_blink_phase = false;
        display_refresh(s);
    }
}
