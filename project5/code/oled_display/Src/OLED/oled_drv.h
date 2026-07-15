/**
 * oled_drv.h - SSD1306 OLED I2C 底层驱动（软件模拟IIC）
 *
 * 硬件: STM32F103C8
 * I2C引脚: SCL-PB6, SDA-PB7
 *
 * 本文件仅负责物理层 I2C 通信和 OLED 硬件初始化。
 * 上层显示函数请使用 oled_disp.h
 */

#ifndef __OLED_DRV_H
#define __OLED_DRV_H

#include "sys.h"

/* ── I2C 引脚宏定义 ─────────────────────────────────── */
#define OLED_SCL_PIN    GPIO_Pin_6    /* PB6 */
#define OLED_SDA_PIN    GPIO_Pin_7    /* PB7 */
#define OLED_GPIO_PORT  GPIOB

#define OLED_SCLK_Clr() GPIO_ResetBits(OLED_GPIO_PORT, OLED_SCL_PIN)
#define OLED_SCLK_Set() GPIO_SetBits(OLED_GPIO_PORT, OLED_SCL_PIN)
#define OLED_SDIN_Clr() GPIO_ResetBits(OLED_GPIO_PORT, OLED_SDA_PIN)
#define OLED_SDIN_Set() GPIO_SetBits(OLED_GPIO_PORT, OLED_SDA_PIN)

/* ── 命令/数据 选择 ──────────────────────────────────── */
#define OLED_CMD  0   /* 写命令 */
#define OLED_DATA 1   /* 写数据 */

/* ── I2C 底层原语 ────────────────────────────────────── */
void OLED_IIC_Start(void);
void OLED_IIC_Stop(void);
void OLED_IIC_Wait_Ack(void);
void Write_IIC_Byte(unsigned char IIC_Byte);
void Write_IIC_Command(unsigned char IIC_Command);
void Write_IIC_Data(unsigned char IIC_Data);
void OLED_WR_Byte(unsigned dat, unsigned cmd);

/* ── OLED 硬件控制 ───────────────────────────────────── */
void OLED_Init(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);

#endif /* __OLED_DRV_H */
