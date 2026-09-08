# -*- coding: utf-8 -*-
"""生成「代码换算逻辑」Excel 文件 (Hanger LDM 项目)."""
import openpyxl
from openpyxl.styles import Font, Alignment, PatternFill, Border, Side
from openpyxl.utils import get_column_letter

OUT = r"C:\Users\AT403\Desktop\Hanger LDM\Hanegr_ldm\LDM\Hanger_LDM_Level_V0.1\代码换算逻辑.xlsx"

wb = openpyxl.Workbook()

header_fill = PatternFill("solid", fgColor="4472C4")
header_font = Font(bold=True, color="FFFFFF", size=11)
title_font = Font(bold=True, size=13, color="1F4E78")
code_font = Font(name="Consolas", size=10)
thin = Side(style="thin", color="BFBFBF")
border = Border(left=thin, right=thin, top=thin, bottom=thin)
center = Alignment(horizontal="center", vertical="center", wrap_text=True)
left = Alignment(horizontal="left", vertical="top", wrap_text=True)


def style_header(ws, row, cols):
    for j in range(1, cols + 1):
        c = ws.cell(row=row, column=j)
        c.fill = header_fill
        c.font = header_font
        c.alignment = center
        c.border = border


def put(ws, r, c, v, align=None, font=None):
    cell = ws.cell(row=r, column=c, value=v)
    cell.border = border
    cell.alignment = align if align else left
    if font:
        cell.font = font
    return cell


# ============================================================
# Sheet 1: 核心换算 (mm -> base_ft -> 各单位)
# ============================================================
ws1 = wb.active
ws1.title = "核心换算"

ws1["A1"] = "核心换算：毫米 mm → base_ft(0.001ft) → 米/英寸"
ws1["A1"].font = title_font
ws1.merge_cells("A1:E1")

hdr = ["换算项", "C 代码公式", "推导 / 常量含义", "四舍五入", "8.438m 示例"]
for j, h in enumerate(hdr, start=1):
    ws1.cell(row=2, column=j, value=h)
style_header(ws1, 2, 5)

core_rows = [
    (
        "① mm → base_ft\n(0.001 ft)\n\n函数:\nmm_to_ft_milli()\ndisplay.c:43",
        "base_ft = (mm * 10000 + 1524) / 3048",
        "1 ft = 304.8 mm\n1 mm = 1000/304.8 千分之一ft\n  = 10000/3048 (同×10)\n\n3048 = 304.8×10\n10000 = 放大 10 倍\n1524 = 3048/2 (半量)",
        "整数除法前 +1524\n即四舍五入\n(round)",
        "8438 mm\n(8438×10000+1524)/3048\n= 84381524/3048\n= 27684 (0.001ft)\n= 27.684 ft",
    ),
    (
        "② base_ft → 米\n(0.001 m)\n\n用于 米态/厘米态\ndisplay.c:382,398",
        "m_milli = (base_ft * 3048 + 5000) / 10000",
        "1 ft = 0.3048 m\nbase_ft ×0.3048 = 米\n×1000 = 0.001m 单位\n = ×3048/10000\n\n5000 = 10000/2 (半量)",
        "整数除法前 +5000\n即四舍五入\n(round)",
        "27684\n(27684×3048+5000)/10000\n= 84385832/10000\n= 8438 (0.001m)\n= 8.438 m",
    ),
    (
        "③ base_ft → 英寸\n(0.001 in)\n\n用于 英寸两态\ndisplay.c:352,368",
        "total_in = base_ft * 12",
        "1 ft = 12 in\nbase_ft(0.001ft) × 12\n= 千分之一英寸\n(0.001 in)",
        "无 (精确整数)",
        "27684 × 12\n= 332208 (0.001in)\n= 332.208 in",
    ),
    (
        "④ 档位偏移\n(形态 B / C)\n\nconfig.h:64-65\ndisplay.c:598-605",
        "FORM_B: base_ft -= 125\nFORM_C: base_ft -= 375",
        "B = A - 0.125 ft\nC = A - 0.375 ft\n\n125 = 0.125×1000\n375 = 0.375×1000",
        "无 (固定常量)",
        "形态B: 27684-125\n       = 27559\n       = 27.559 ft\n形态C: 27684-375\n       = 27309\n       = 27.309 ft",
    ),
]
for i, row in enumerate(core_rows, start=3):
    for j, v in enumerate(row, start=1):
        put(ws1, i, j, v, align=(left if j >= 3 else center),
            font=(code_font if j == 2 else None))
    ws1.cell(row=i, column=1).font = Font(bold=True)

for col, w in zip("ABCDE", [20, 30, 34, 18, 28]):
    ws1.column_dimensions[col].width = w
ws1.freeze_panes = "A3"

# ============================================================
# Sheet 2: 7 态显示换算 (代码实现)
# ============================================================
ws2 = wb.create_sheet("7态显示换算")

ws2["A1"] = "7 种显示态的代码换算逻辑 (base_ft 为 0.001 ft 单位)"
ws2["A1"].font = title_font
ws2.merge_cells("A1:F1")

hdr2 = ["序号", "显示态", "C 代码公式", "精度/四舍五入", "示例计算 (base_ft=27684)", "LCD 显示"]
for j, h in enumerate(hdr2, start=1):
    ws2.cell(row=2, column=j, value=h)
style_header(ws2, 2, 6)

rows2 = [
    (
        "0", "十进制英尺\nDIGIT_FT_DEC\ndisplay.c:292",
        "ft_int = base_ft / 1000\nfrac = base_ft % 1000\nfrac_len = 3\nwhile frac_len>0 且 frac%10==0:\n    frac/=10; frac_len--",
        "整数 + 小数\n小数去尾零\n(最多 3 位)",
        "ft_int = 27\nfrac = 684\n684%10=4≠0 → 保留\nfrac_len = 3",
        "27.684 ft",
    ),
    (
        "1", "英尺+1/8\nDIGIT_FT_FRAC\ndisplay.c:312",
        "ft_int = base_ft / 1000\nn8 = ((base_ft%1000)*8 + 500)/1000\n约分: gcd(n8,8)",
        "1/8 英尺精度\n+500 = 1000/2\n(四舍五入到 1/8)\n分数约分",
        "ft_int = 27\nn8 = (684*8+500)/1000\n   = 5972/1000 = 5\ngcd(5,8)=1 → 5/8",
        "27 5/8 ft",
    ),
    (
        "2", "英尺+英寸\nDIGIT_FT_IN\ndisplay.c:328",
        "ft_int = base_ft / 1000\nin_milli = (base_ft%1000)*12\nin_10 = (in_milli+50)/100\nin_int = in_10/10\nin_frac = in_10%10",
        "英寸 0.1 精度\n+50 = 100/2\n(四舍五入到 0.1in)",
        "ft_int = 27\nin_milli = 684*12 = 8208\nin_10 = (8208+50)/100 = 82\nin_int = 8, in_frac = 2",
        "27 ft 8.2 in",
    ),
    (
        "3", "十进制英寸\nDIGIT_IN_DEC\ndisplay.c:350",
        "total_in = base_ft * 12\nin_int = total_in / 1000\nin_frac = total_in % 1000\n去尾零 (同态0)",
        "整数 + 小数\n小数去尾零\n(最多 3 位)",
        "total_in = 332208\nin_int = 332\nin_frac = 208\n208%10=8≠0 → 保留",
        "332.208 in",
    ),
    (
        "4", "英寸+1/2\nDIGIT_IN_FRAC\ndisplay.c:366",
        "total_in = base_ft * 12\nin_int = total_in / 1000\nn2 = ((total_in%1000)*2 + 500)/1000",
        "1/2 英寸精度\nn2 只会是 0 或 1\n+500 = 1000/2\n(四舍五入到 1/2)",
        "total_in = 332208\nin_int = 332\nn2 = (208*2+500)/1000\n   = 916/1000 = 0\n→ 无分数",
        "332 in",
    ),
    (
        "5", "米\nDIGIT_M\ndisplay.c:380",
        "m_milli = (base_ft*3048 + 5000)/10000\nm_int = m_milli / 1000\nm_frac = m_milli % 1000\n去尾零 (同态0)",
        "四舍五入到 0.001m\n+5000 = 10000/2\n小数去尾零",
        "m_milli = 8438\nm_int = 8\nm_frac = 438\n438%10=8≠0 → 保留",
        "8.438 m",
    ),
    (
        "6", "厘米\nDIGIT_CM\ndisplay.c:396",
        "m_milli = (base_ft*3048 + 5000)/10000\ncm_tenth = m_milli\ncm_int = cm_tenth / 10\ncm_frac = cm_tenth % 10",
        "0.1 cm 精度\n(0.1cm = 0.001m)\n4位整数+1位小数",
        "m_milli = 8438\ncm_tenth = 8438\ncm_int = 843\ncm_frac = 8",
        "843.8 cm",
    ),
]
for i, row in enumerate(rows2, start=3):
    for j, v in enumerate(row, start=1):
        put(ws2, i, j, v, align=(left if j in (3, 4, 5) else center),
            font=(code_font if j == 3 else None))
    ws2.cell(row=i, column=2).font = Font(bold=True)

for col, w in zip("ABCDEF", [8, 18, 38, 24, 30, 16]):
    ws2.column_dimensions[col].width = w
ws2.freeze_panes = "A3"

# ============================================================
# Sheet 3: 整数四舍五入技巧
# ============================================================
ws3 = wb.create_sheet("整数四舍五入技巧")

ws3["A1"] = "代码里用「整数运算 + 半量」实现四舍五入的技巧汇总"
ws3["A1"].font = title_font
ws3.merge_cells("A1:C1")

hdr3 = ["技巧", "代码", "说明"]
for j, h in enumerate(hdr3, start=1):
    ws3.cell(row=2, column=j, value=h)
style_header(ws3, 2, 3)

tips = [
    (
        "四舍五入到整数\n(x / N 四舍五入)",
        "(x * M + N/2) / N",
        "先 +N/2 再做整数除法。\n例: 5972/1000 → 先 +500 再 /1000 = 5 (原本 5.972)",
    ),
    (
        "mm → base_ft 四舍五入",
        "(mm * 10000 + 1524) / 3048",
        "1524 = 3048/2，向上四舍五入。\n保证米态能精确恢复原 mm 值。",
    ),
    (
        "base_ft → m 四舍五入",
        "(base_ft * 3048 + 5000) / 10000",
        "5000 = 10000/2。四舍五入到 0.001 m。",
    ),
    (
        "小数去尾零",
        "while frac%10==0: frac/=10",
        "把 0.680 → 0.68 → 显示 2 位小数；\n684 末位 4 非 0，保留 3 位。",
    ),
    (
        "1/8 分数约分",
        "gcd(n8, 8) 后 分子/分母 同除",
        "5/8 不约分；4/8 → 1/2；6/8 → 3/4。\n用辗转相除法求 gcd。",
    ),
]
for i, row in enumerate(tips, start=3):
    for j, v in enumerate(row, start=1):
        put(ws3, i, j, v, align=(left if j == 3 else center),
            font=(code_font if j == 2 else None))
    ws3.cell(row=i, column=1).font = Font(bold=True)

ws3.column_dimensions["A"].width = 22
ws3.column_dimensions["B"].width = 32
ws3.column_dimensions["C"].width = 48
ws3.freeze_panes = "A3"

wb.save(OUT)
print("saved:", OUT)
