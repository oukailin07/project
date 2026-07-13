/**
 * oled_disp.c - OLED 上层显示函数
 *
 * 包含所有显示相关操作：清屏、定位、字符、字符串、
 * 数字、中文、浮点数、位图等。
 *
 * 本文件不包含任何 I2C 或 GPIO 操作代码 —
 * 所有硬件访问通过调用驱动层的 OLED_WR_Byte() 完成。
 */

#include "oled_disp.h"
#include "oledfont.h"
#include "stdlib.h"
#include "stdio.h"

/* ═══════════════════════════════════════════════════════
 *  内部辅助函数
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief  计算 m 的 n 次方（用于数字各位提取）
 */
static u32 disp_pow(u8 m, u8 n)
{
    u32 result = 1;
    while (n--) result *= m;
    return result;
}

/* ═══════════════════════════════════════════════════════
 *  像素级操作
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief  设置光标到指定 (列, 页) 位置
 * @param  x  列地址 (0~127)
 * @param  y  页地址 (0~7)
 */
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte((x & 0x0f), OLED_CMD);
}

/**
 * @brief  在指定位置画一个点
 * @param  x  横坐标 0~127
 * @param  y  纵坐标 0~63
 * @param  t  1 = 点亮, 0 = 熄灭
 */
void OLED_DrawPoint(u8 x, u8 y, u8 t)
{
    OLED_Set_Pos(x, y / 8);
    if (t)
        OLED_WR_Byte(1 << (y % 8), OLED_DATA);
    else
        OLED_WR_Byte(0, OLED_DATA);
}

/**
 * @brief  填充一个矩形区域
 * @param  x1, y1  左上角坐标
 * @param  x2, y2  右下角坐标
 * @param  dot      1 = 填充, 0 = 清除
 */
void OLED_FillRect(u8 x1, u8 y1, u8 x2, u8 y2, u8 dot)
{
    u8 x, y;
    for (y = y1; y <= y2; y++)
    {
        for (x = x1; x <= x2; x++)
        {
            OLED_DrawPoint(x, y, dot);
        }
    }
}

/* ═══════════════════════════════════════════════════════
 *  整屏操作
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief  清屏（全屏填充 0x00）
 */
void OLED_Clear(void)
{
    u8 i, n;
    for (i = 0; i < OLED_PAGES; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);    /* 设置页地址（0~7） */
        OLED_WR_Byte(0x00, OLED_CMD);         /* 设置列低地址 */
        OLED_WR_Byte(0x10, OLED_CMD);         /* 设置列高地址 */
        for (n = 0; n < OLED_WIDTH; n++)
            OLED_WR_Byte(0x00, OLED_DATA);
    }
}

/**
 * @brief  用指定字节填充整个屏幕
 * @param  fill_Data  填充数据（0x00 全黑, 0xFF 全亮）
 */
void OLED_Fill(unsigned char fill_Data)
{
    unsigned char m, n;
    for (m = 0; m < OLED_PAGES; m++)
    {
        OLED_WR_Byte(0xb0 + m, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for (n = 0; n < OLED_WIDTH; n++)
            OLED_WR_Byte(fill_Data, OLED_DATA);
    }
}

/* ═══════════════════════════════════════════════════════
 *  文本显示
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief  在指定位置显示一个 ASCII 字符
 * @param  x          列地址 0~127
 * @param  y          页地址 0~7
 * @param  chr        ASCII 字符
 * @param  Char_Size  字号: 12 (6×8 点阵) 或 16 (8×16 点阵)
 */
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size)
{
    unsigned char c = 0, i = 0;

    c = chr - ' ';  /* 减去空格偏移，得到字库索引 */

    if (x > OLED_WIDTH - 1) { x = 0; y = y + 2; }

    if (Char_Size == 16)
    {
        /* 8×16 点阵：上下两页各8字节 */
        OLED_Set_Pos(x, y);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
        OLED_Set_Pos(x, y + 1);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
    }
    else
    {
        /* 6×8 点阵 */
        OLED_Set_Pos(x, y);
        for (i = 0; i < 6; i++)
            OLED_WR_Byte(F6x8[c][i], OLED_DATA);
    }
}

/**
 * @brief  显示一个以 '\0' 结尾的 ASCII 字符串
 * @param  x, y       起始坐标
 * @param  chr        字符串指针
 * @param  Char_Size  字号: 12 或 16
 */
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 Char_Size)
{
    unsigned char j = 0;
    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j], Char_Size);
        x += 8;
        if (x > 120) { x = 0; y += 2; }   /* 自动换行 */
        j++;
    }
}

/**
 * @brief  显示一个无符号整数（右对齐，前导空格）
 * @param  x, y   起始坐标
 * @param  num    要显示的数值
 * @param  len    显示位数
 * @param  size   字号
 */
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size)
{
    u8 t, temp;
    u8 enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / disp_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (size / 2) * t, y, ' ', size);
                continue;
            }
            else enshow = 1;
        }
        OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size);
    }
}

/**
 * @brief  显示一个浮点数（例如 "23.5"）
 * @param  x, y     起始坐标
 * @param  val      浮点数值
 * @param  intLen   整数部分位数
 * @param  decLen   小数部分位数（0 = 只显示整数）
 * @param  size     字号
 */
void OLED_ShowFloat(u8 x, u8 y, float val, u8 intLen, u8 decLen, u8 size)
{
    char buf[16];
    u8 i;

    if (decLen > 0)
        sprintf(buf, "%0*.*f", (int)(intLen + decLen + 1), (int)decLen, (double)val);
    else
        sprintf(buf, "%0*.0f", (int)intLen, (double)val);

    for (i = 0; buf[i] != '\0'; i++)
    {
        OLED_ShowChar(x, y, (u8)buf[i], size);
        x += 8;
    }
}

/**
 * @brief  显示一个 16×32 中文字符（按索引）
 * @param  x, y  起始坐标
 * @param  no    汉字在 Hzk 字库中的索引号
 * @note   每个汉字占 4 页 × 16 字节 = 64 字节
 */
void OLED_ShowCHinese(u8 x, u8 y, u8 no)
{
    u8 t;

    OLED_Set_Pos(x, y);
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[4 * no][t], OLED_DATA);

    OLED_Set_Pos(x, y + 1);
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[4 * no + 1][t], OLED_DATA);

    OLED_Set_Pos(x, y + 2);
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[4 * no + 2][t], OLED_DATA);

    OLED_Set_Pos(x, y + 3);
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[4 * no + 3][t], OLED_DATA);
}

/* ═══════════════════════════════════════════════════════
 *  位图显示
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief  在 OLED 上绘制位图
 * @param  x0, y0  左上角坐标 (列, 页)
 * @param  x1, y1  右下角坐标 (列, 页)
 * @param  BMP     位图数据数组
 */
void OLED_DrawBMP(unsigned char x0, unsigned char y0,
                  unsigned char x1, unsigned char y1,
                  unsigned char BMP[])
{
    unsigned int j = 0;
    unsigned char x, y;
    unsigned char y_pages;

    if (y1 % 8 == 0)
        y_pages = y1 / 8;
    else
        y_pages = y1 / 8 + 1;

    for (y = y0; y < y_pages; y++)
    {
        OLED_Set_Pos(x0, y);
        for (x = x0; x < x1; x++)
        {
            OLED_WR_Byte(BMP[j++], OLED_DATA);
        }
    }
}
