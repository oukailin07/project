/**
 * oled_drv.c - SSD1306 OLED I2C 底层驱动（软件模拟IIC）
 *
 * 硬件: STM32F103C8
 * OLED  : SSD1306, 128×64, I2C 接口
 * SCL   : PB6
 * SDA   : PB7
 */

#include "oled_drv.h"
#include "delay.h"

/* ═══════════════════════════════════════════════════════
 *  I2C 软件模拟基本操作
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief  产生 I2C 起始信号
 */
void OLED_IIC_Start(void)
{
    OLED_SCLK_Set();
    OLED_SDIN_Set();
    OLED_SDIN_Clr();
    OLED_SCLK_Clr();
}

/**
 * @brief  产生 I2C 停止信号
 */
void OLED_IIC_Stop(void)
{
    OLED_SCLK_Set();
    OLED_SDIN_Clr();
    OLED_SDIN_Set();
}

/**
 * @brief  等待 I2C 应答（简化版，不做ACK检测以保证软件模拟稳定性）
 */
void OLED_IIC_Wait_Ack(void)
{
    OLED_SCLK_Set();
    OLED_SCLK_Clr();
}

/**
 * @brief  通过 I2C 发送一个字节（高位在前）
 */
void Write_IIC_Byte(unsigned char IIC_Byte)
{
    unsigned char i;
    unsigned char m, da;
    da = IIC_Byte;
    OLED_SCLK_Clr();
    for (i = 0; i < 8; i++)
    {
        m = da;
        m = m & 0x80;
        if (m == 0x80)
            OLED_SDIN_Set();
        else
            OLED_SDIN_Clr();
        da = da << 1;
        OLED_SCLK_Set();
        OLED_SCLK_Clr();
    }
}

/**
 * @brief  向 SSD1306 写入一个命令字节
 *         帧格式: SA0=0 (0x78), 控制字节=0x00, 命令数据
 */
void Write_IIC_Command(unsigned char IIC_Command)
{
    OLED_IIC_Start();
    Write_IIC_Byte(0x78);        /* 从机地址, SA0=0 */
    OLED_IIC_Wait_Ack();
    Write_IIC_Byte(0x00);        /* 控制字节: 命令 */
    OLED_IIC_Wait_Ack();
    Write_IIC_Byte(IIC_Command);
    OLED_IIC_Wait_Ack();
    OLED_IIC_Stop();
}

/**
 * @brief  向 SSD1306 GDDRAM 写入一个数据字节
 *         帧格式: SA0=0 (0x78), 控制字节=0x40, 数据
 */
void Write_IIC_Data(unsigned char IIC_Data)
{
    OLED_IIC_Start();
    Write_IIC_Byte(0x78);        /* 从机地址, SA0=0 */
    OLED_IIC_Wait_Ack();
    Write_IIC_Byte(0x40);        /* 控制字节: 数据 */
    OLED_IIC_Wait_Ack();
    Write_IIC_Byte(IIC_Data);
    OLED_IIC_Wait_Ack();
    OLED_IIC_Stop();
}

/**
 * @brief  统一写入接口 — 命令或数据
 * @param  dat  数据值
 * @param  cmd  0 = 命令, 1 = 数据
 */
void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
    if (cmd)
        Write_IIC_Data(dat);
    else
        Write_IIC_Command(dat);
}

/* ═══════════════════════════════════════════════════════
 *  OLED 硬件控制
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief  开启 OLED 显示
 */
void OLED_Display_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);   /* 设置DC-DC电荷泵 */
    OLED_WR_Byte(0x14, OLED_CMD);   /* 电荷泵开启 */
    OLED_WR_Byte(0xAF, OLED_CMD);   /* 显示开启 */
}

/**
 * @brief  关闭 OLED 显示
 */
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);   /* 设置DC-DC电荷泵 */
    OLED_WR_Byte(0x10, OLED_CMD);   /* 电荷泵关闭 */
    OLED_WR_Byte(0xAE, OLED_CMD);   /* 显示关闭 */
}

/**
 * @brief  初始化 SSD1306 OLED（I2C 软件模拟）
 *
 * 配置 GPIOB 6/7 为推挽输出，然后发送 SSD1306 初始化序列。
 */
void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = OLED_SCL_PIN | OLED_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;       /* 推挽输出 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       /* 速度50MHz */
    GPIO_Init(OLED_GPIO_PORT, &GPIO_InitStructure);

    /* ── SSD1306 初始化命令序列 ──────────────────────── */
    OLED_WR_Byte(0xAE, OLED_CMD);   /* 关闭显示 */
    OLED_WR_Byte(0x00, OLED_CMD);   /* 设置列低地址 */
    OLED_WR_Byte(0x10, OLED_CMD);   /* 设置列高地址 */
    OLED_WR_Byte(0x40, OLED_CMD);   /* 设置起始行地址 */
    OLED_WR_Byte(0xB0, OLED_CMD);   /* 设置页地址 */
    OLED_WR_Byte(0x81, OLED_CMD);   /* 对比度控制 */
    OLED_WR_Byte(0xFF, OLED_CMD);   /* 对比度 = 128 */
    OLED_WR_Byte(0xA1, OLED_CMD);   /* 段重映射（左右翻转） */
    OLED_WR_Byte(0xA6, OLED_CMD);   /* 正常显示（非反转） */
    OLED_WR_Byte(0xA8, OLED_CMD);   /* 设置复用比 */
    OLED_WR_Byte(0x3F, OLED_CMD);   /* 1/64 占空比 */
    OLED_WR_Byte(0xC8, OLED_CMD);   /* COM扫描方向（上下翻转） */
    OLED_WR_Byte(0xD3, OLED_CMD);   /* 设置显示偏移 */
    OLED_WR_Byte(0x00, OLED_CMD);   /* 偏移 = 0 */
    OLED_WR_Byte(0xD5, OLED_CMD);   /* 设置振荡器分频 */
    OLED_WR_Byte(0x80, OLED_CMD);
    OLED_WR_Byte(0xD8, OLED_CMD);   /* 设置区域颜色模式关闭 */
    OLED_WR_Byte(0x05, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);   /* 设置预充电周期 */
    OLED_WR_Byte(0xF1, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);   /* 设置COM引脚配置 */
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);   /* 设置Vcomh */
    OLED_WR_Byte(0x30, OLED_CMD);
    OLED_WR_Byte(0x8D, OLED_CMD);   /* 使能电荷泵 */
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);   /* 开启OLED面板 */
}
