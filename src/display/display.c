/*
 * display.c
 *
 *  LCD 应用显示层实现: 图标形式 + 7 态单位换算。
 *  只操作 lcd_frame 段码缓冲, 最终调用 lcd_write_pattern 整块刷屏。
 */

#include "display.h"
#include "lcd/lcd.h"
#include "lcd/lcd_seg_map.h"
#include <stdio.h>
#include <string.h>

/* ======================================================================
 *  单位图标索引 (lcd_show_icon 0-based)
 * ====================================================================== */
#define LCD_ICON_FWD_M    (20U)   /* T21 正向 M */
#define LCD_ICON_FWD_FT   (19U)   /* T20 正向 英尺 */
#define LCD_ICON_FWD_IN   (28U)   /* T29 正向 英寸 */
#define LCD_ICON_FWD_CM   (29U)   /* T30 正向 厘米 */
#define LCD_ICON_REV_M    (16U)   /* T17 反向 M */
#define LCD_ICON_REV_FT   (17U)   /* T18 反向 英尺 */
#define LCD_ICON_REV_IN   (14U)   /* T15 反向 英寸 */
#define LCD_ICON_REV_CM   (13U)   /* T14 反向 厘米 */
#define LCD_ICON_SLASH    (23U)   /* T24 斜杠 */
#define LCD_ICON_T7       (6U)    /* T7  十字加号 */

/* 单位枚举: 用于 lcd_show_unit 选择单位图标 */
typedef enum
{
    UNIT_FT = 0,   /* 英尺 */
    UNIT_IN,       /* 英寸 */
    UNIT_M,        /* 米   */
    UNIT_CM,       /* 厘米 */
} unit_t;

/* ----------------------------------------------------------------------
 *  三档基准值 (单位 0.001 ft), 对应图标形态 A/B/C
 * ---------------------------------------------------------------------- */
#define BASE_FT_A  (55125UL)   /* 55.125 ft */
#define BASE_FT_B  (55000UL)   /* 55.000 ft */
#define BASE_FT_C  (54750UL)   /* 54.750 ft */

/* ----------------------------------------------------------------------
 *  获取当前图标形态对应的基准值 (0.001 ft)。IDLE 默认使用 A 档。
 * ---------------------------------------------------------------------- */
static uint32_t lcd_base_ft(form_t form)
{
    switch (form)
    {
        case FORM_B:  return BASE_FT_B;
        case FORM_C:  return BASE_FT_C;
        case FORM_IDLE:
        case FORM_A:
        default:      return BASE_FT_A;
    }
}

/* ----------------------------------------------------------------------
 *  端部组显示整数部分 (右对齐, 高位靠左):
 *    正向: 顶部组 P1/P2/P3 正立, 右对齐到 P3 (16 -> P2=1, P3=6)
 *    反向: 顶部组 P10/P9/P8 倒置, 右对齐到 P8 (16 -> P9=1, P8=6)
 * ---------------------------------------------------------------------- */
static void lcd_show_int_group(uint8_t *frame, const char *num,
                               uint8_t int_len, bool forward)
{
    uint8_t k;

    /* 最多显示 3 位整数, 从最低位 (最右) 往左填充 */
    for (k = 0U; (k < int_len) && (k < 3U); k++)
    {
        uint8_t digit = (uint8_t)(num[int_len - 1U - k] - '0');

        if (forward)
        {
            lcd_show_digit((uint8_t)(3U - k), digit, frame);   /* P3 <- 最低位 */
        }
        else
        {
            lcd_show_digit_rot((uint8_t)(8U + k), digit, frame); /* P8 <- 最低位 */
        }
    }
}

/* ----------------------------------------------------------------------
 *  中间组显示小数部分 (小数点 + 小数数字, 最多 4 位):
 *    数字从小数点右侧第一位开始连续排布, 小数点图标/起始位随位数变化:
 *      1 位: 正向 T25+P6,           反向 T23+P5
 *      2 位: 正向 T25+P6/P7,        反向 T23+P5/P4
 *      3 位: 正向 T22+P5/P6/P7,     反向 T26+P6/P5/P4
 *      4 位: 正向 T16+P4/P5/P6/P7,  反向 T27+P7/P6/P5/P4
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
    static const uint8_t fwd_start[4] = {6U, 6U, 5U, 4U};
    static const uint8_t fwd_dot[4]   = {24U, 24U, 21U, 15U};  /* T25,T25,T22,T16 */
    static const uint8_t rev_start[4] = {5U, 5U, 6U, 7U};
    static const uint8_t rev_dot[4]   = {22U, 22U, 25U, 26U};  /* T23,T23,T26,T27 */

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
            case UNIT_CM: lcd_show_icon(LCD_ICON_FWD_CM, frame); break;
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
            case UNIT_CM: lcd_show_icon(LCD_ICON_REV_CM, frame); break;
            default: break;
        }
    }
}

/* ----------------------------------------------------------------------
 *  分数显示 num/den (如 1/8): 横排 分子左/分母右, 斜杠 T24 两点亮
 *    正向: 分子 P5, 分母 P6, 斜杠 T24 居中
 *    反向: 分子 P6(倒置), 分母 P5(倒置), 斜杠 T24
 * ---------------------------------------------------------------------- */
static void lcd_show_fraction(uint8_t *frame, uint8_t num, uint8_t den, bool forward)
{
    lcd_show_icon(LCD_ICON_SLASH, frame);   /* 斜杠 T24 */

    if (forward)
    {
        lcd_show_digit(5U, num, frame);     /* 分子 P5 */
        lcd_show_digit(6U, den, frame);     /* 分母 P6 */
    }
    else
    {
        lcd_show_digit_rot(6U, num, frame); /* 分子 P6 (倒置) */
        lcd_show_digit_rot(5U, den, frame); /* 分母 P5 (倒置) */
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
 *  副读数显示 (如 1.5 英寸): 用于双单位 "xx ft + y.y in"。
 *    正向: 整数在 P5, 小数点 T25, 小数在 P6
 *    反向: 整数在 P6(倒置), 小数点 T23, 小数在 P5(倒置)
 *    int_val 1 位整数, frac_val 小数, frac_len 0~1 位 (0=纯整数, 隐藏零头)
 * ---------------------------------------------------------------------- */
static void lcd_show_sub_decimal(uint8_t *frame, uint32_t int_val,
                                 uint32_t frac_val, uint8_t frac_len, bool forward)
{
    if (forward)
    {
        lcd_show_digit(5U, (uint8_t)(int_val % 10U), frame);      /* 整数 P5 */
        if (frac_len > 0U)
        {
            lcd_show_icon(24U, frame);                            /* T25 */
            lcd_show_digit(6U, (uint8_t)(frac_val % 10U), frame); /* 小数 P6 */
        }
    }
    else
    {
        lcd_show_digit_rot(6U, (uint8_t)(int_val % 10U), frame);  /* 整数 P6 */
        if (frac_len > 0U)
        {
            lcd_show_icon(22U, frame);                            /* T23 */
            lcd_show_digit_rot(5U, (uint8_t)(frac_val % 10U), frame); /* 小数 P5 */
        }
    }
}

/* ----------------------------------------------------------------------
 *  4 位整数 + 1 位小数 (如 1680.2 cm)。
 *    正向: 整数在中间组 P4 P5 P6 P7, 小数点 T28, 小数在 P9
 *    反向: 整数在中间组 P7 P6 P5 P4 (倒置), 小数点 T19, 小数在 P2 (倒置)
 * ---------------------------------------------------------------------- */
static void lcd_show_4int_1frac(uint8_t *frame, uint32_t int_val,
                                uint32_t frac_val, bool forward)
{
    uint8_t k;

    if (forward)
    {
        /* P7=个位 ... P4=千位 */
        for (k = 0U; k < 4U; k++)
        {
            lcd_show_digit((uint8_t)(7U - k), (uint8_t)(int_val % 10U), frame);
            int_val /= 10U;
        }
        lcd_show_icon(27U, frame);                          /* T28 */
        lcd_show_digit(9U, (uint8_t)(frac_val % 10U), frame);
    }
    else
    {
        /* 镜像: P4=个位 ... P7=千位 */
        for (k = 0U; k < 4U; k++)
        {
            lcd_show_digit_rot((uint8_t)(4U + k), (uint8_t)(int_val % 10U), frame);
            int_val /= 10U;
        }
        lcd_show_icon(18U, frame);                          /* T19 */
        lcd_show_digit_rot(2U, (uint8_t)(frac_val % 10U), frame);
    }
}

/* ----------------------------------------------------------------------
 *  7 态换算显示: 根据 base_ft (0.001 ft) 动态换算并显示对应状态。
 *    0: 十进制英尺 (有效小数位)
 *    1: 英尺 + 分数 (1/8, 约分, 零头隐藏)
 *    2: 英尺 + 英寸 (双单位, 零头隐藏)
 *    3: 十进制英寸
 *    4: 英寸 + 分数 (1/2, 零头隐藏)
 *    5: 米 (有效小数位)
 *    6: 厘米 (4 位整数 + 1 位小数)
 * ---------------------------------------------------------------------- */
static void lcd_convert_show(uint8_t *frame, digital_t state,
                             uint32_t base_ft, bool forward)
{
    switch (state)
    {
        case DIGIT_FT_DEC:   /* 十进制英尺 */
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
            if (0U == frac_len)
            {
                if (forward) { lcd_show_icon(21U, frame); }  /* T22 */
                else         { lcd_show_icon(25U, frame); }  /* T26 */
            }
            lcd_show_unit(frame, UNIT_FT, forward);
            break;
        }

        case DIGIT_FT_FRAC:   /* 英尺 + 分数 (1/8) */
        {
            uint32_t ft_int = base_ft / 1000UL;
            uint32_t n8     = ((base_ft % 1000UL) * 8UL + 500UL) / 1000UL;
            lcd_show_decimal(frame, ft_int, 0U, 0U, forward);
            if (0U != n8)
            {
                uint32_t num = n8, den = 8UL, a = num, b = den, t;
                while (0U != b) { t = a % b; a = b; b = t; }
                lcd_show_fraction(frame, (uint8_t)(num / a),
                                  (uint8_t)(den / a), forward);
            }
            lcd_show_unit(frame, UNIT_FT, forward);
            break;
        }

        case DIGIT_FT_IN:   /* 英尺 + 英寸 (双单位) */
        {
            uint32_t ft_int   = base_ft / 1000UL;
            uint32_t in_milli = (base_ft % 1000UL) * 12UL;
            lcd_show_decimal(frame, ft_int, 0U, 0U, forward);
            lcd_show_unit(frame, UNIT_FT, forward);
            if (0U != in_milli)
            {
                uint32_t in_int   = in_milli / 1000UL;
                uint32_t in_frac  = in_milli % 1000UL;
                uint8_t  frac_len = 3U;
                while ((frac_len > 0U) && (0U == (in_frac % 10UL)))
                {
                    in_frac /= 10UL;
                    frac_len--;
                }
                lcd_show_sub_decimal(frame, in_int, in_frac, frac_len, forward);
                lcd_show_unit(frame, UNIT_IN, forward);
            }
            break;
        }

        case DIGIT_IN_DEC:   /* 十进制英寸 */
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
            lcd_show_decimal(frame, in_int, in_frac, frac_len, forward);
            lcd_show_unit(frame, UNIT_IN, forward);
            break;
        }

        case DIGIT_IN_FRAC:   /* 英寸 + 分数 (1/2) */
        {
            uint32_t total_in = base_ft * 12UL;
            uint32_t in_int   = total_in / 1000UL;
            uint32_t n2       = ((total_in % 1000UL) * 2UL + 500UL) / 1000UL;
            lcd_show_decimal(frame, in_int, 0U, 0U, forward);
            if (0U != n2)
            {
                lcd_show_fraction(frame, (uint8_t)n2, 2U, forward);
            }
            lcd_show_unit(frame, UNIT_IN, forward);
            break;
        }

        case DIGIT_M:   /* 米 (有效小数位) */
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

        case DIGIT_CM:   /* 厘米 (4 位整数 + 1 位小数) */
        {
            uint32_t m_milli  = (base_ft * 3048UL + 5000UL) / 10000UL;
            uint32_t cm_tenth = m_milli;      /* 0.1 cm = 0.001 m */
            uint32_t cm_int   = cm_tenth / 10UL;
            uint32_t cm_frac  = cm_tenth % 10UL;
            lcd_show_4int_1frac(frame, cm_int, cm_frac, forward);
            lcd_show_unit(frame, UNIT_CM, forward);
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
    s->form      = FORM_IDLE;
    s->digital   = DIGIT_FT_DEC;
    s->t7        = T7_OFF;
    s->direction = DIR_FORWARD;
    s->sw3_first = true;
    s->sw4_first = true;

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

        case UI_EVT_SW2_SHORT:
            /* 数字态循环: 0 -> 1 -> ... -> 6 -> 0 */
            s->digital = (digital_t)(((uint8_t)s->digital + 1U) % (uint8_t)DIGIT_NUM);
            break;

        case UI_EVT_SW3_SHORT:
            /* 首次按下忽略, 之后直接 A 形态 + 默认数字 (十进制英尺) */
            if (s->sw3_first)
            {
                s->sw3_first = false;
            }
            else
            {
                s->form    = FORM_A;
                s->digital = DIGIT_FT_DEC;
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
 *  渲染: 按当前状态刷新整屏。
 * ---------------------------------------------------------------------- */
void display_refresh(const ui_state_t *s)
{
    uint8_t frame[LCD_SEG_NUM];
    bool    forward = (DIR_FORWARD == s->direction);

    memset(frame, LCD_SEG_OFF, sizeof(frame));

    /* 常亮固定图标: T2 (水平仪) + T9~T12 (电量框/三格) */
    lcd_show_icon(1U,  frame);  /* T2  水平仪 */
    lcd_show_icon(8U,  frame);  /* T9  电量显示框 */
    lcd_show_icon(9U,  frame);  /* T10 第三格电量 */
    lcd_show_icon(10U, frame);  /* T11 第二格电量 */
    lcd_show_icon(11U, frame);  /* T12 第一格电量 */

    /* 按形式的图标组合 */
    switch (s->form)
    {
        case FORM_IDLE:
            lcd_show_icon(0U, frame);    /* T1  水平向下的箭头 */
            break;

        case FORM_A:
            lcd_show_icon(0U,  frame);   /* T1  水平向下的箭头 */
            lcd_show_icon(3U,  frame);   /* T4  一档基准 */
            lcd_show_icon(4U,  frame);   /* T5  二档基准 */
            lcd_show_icon(7U,  frame);   /* T8  三档校准 */
            lcd_show_icon(12U, frame);   /* T13 校准开关 */
            break;

        case FORM_B:
            lcd_show_icon(2U,  frame);   /* T3  更向下的箭头 */
            lcd_show_icon(4U,  frame);   /* T5  二档基准 */
            lcd_show_icon(7U,  frame);   /* T8  三档校准 */
            lcd_show_icon(12U, frame);   /* T13 校准开关 */
            break;

        case FORM_C:
            lcd_show_icon(5U,  frame);   /* T6  最向下的箭头 */
            lcd_show_icon(7U,  frame);   /* T8  三档校准 */
            lcd_show_icon(12U, frame);   /* T13 校准开关 */
            break;

        default:
            break;
    }

    /* 数字 7 态换算显示: 仅非 IDLE 形态显示 */
    if (FORM_IDLE != s->form)
    {
        lcd_convert_show(frame, s->digital, lcd_base_ft(s->form), forward);
    }

    /* T7 十字加号: 独立于形态/数字 */
    if (T7_ON == s->t7)
    {
        lcd_show_icon(LCD_ICON_T7, frame);
    }

    lcd_write_pattern(frame, LCD_SEG_NUM);
}
