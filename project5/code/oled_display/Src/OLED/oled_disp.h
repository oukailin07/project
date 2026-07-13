/**
 * oled_disp.h - OLED 上层显示接口
 *
 * 纯显示功能 — 与 I2C 传输层解耦。
 * 底层依赖 oled_drv.h 提供的 OLED_WR_Byte() 和 OLED_CMD / OLED_DATA 宏。
 *
 * 应用层代码只需包含本头文件即可。
 */

#ifndef __OLED_DISP_H
#define __OLED_DISP_H

#include "sys.h"
#include "oled_drv.h"

/* ── 屏幕尺寸 ───────────────────────────────────────── */
#define OLED_WIDTH      128
#define OLED_HEIGHT      64
#define OLED_PAGES        8

/* ── API 函数声明 ────────────────────────────────────── */

/* 基础控制 */
void OLED_Clear(void);
void OLED_Fill(unsigned char fill_Data);
void OLED_Set_Pos(unsigned char x, unsigned char y);

/* 像素 / 填充 */
void OLED_DrawPoint(u8 x, u8 y, u8 t);
void OLED_FillRect(u8 x1, u8 y1, u8 x2, u8 y2, u8 dot);

/* 文本显示 */
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size);
void OLED_ShowString(u8 x, u8 y, u8 *p, u8 Char_Size);
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size);
void OLED_ShowCHinese(u8 x, u8 y, u8 no);

/* 浮点数显示（便捷函数） */
void OLED_ShowFloat(u8 x, u8 y, float val, u8 intLen, u8 decLen, u8 size);

/* 位图显示 */
void OLED_DrawBMP(unsigned char x0, unsigned char y0,
                  unsigned char x1, unsigned char y1,
                  unsigned char BMP[]);

#endif /* __OLED_DISP_H */
