/**
 * usart2.c - USART2 驱动 (PA2=TX, PA3=RX)
 *
 * 用于 lksscope 数据绘图: ASCII 逗号分隔浮点数输出。
 * 不使用 sprintf float 格式化 (避免 MicroLIB float 支持问题)。
 *
 * 硬件: APM32F103CB
 *   USART2 挂载在 APB1 总线上
 *   TX: PA2 (复用推挽输出)
 *   RX: PA3 (浮空输入, 可选)
 */

#include "usart2.h"

/* APM32 外设库 */
#include "apm32f10x_gpio.h"
#include "apm32f10x_rcm.h"
#include "apm32f10x_usart.h"

/* ================================================================
 * 浮点数 → 字符串 (简单实现, 无 stdio float 依赖)
 *
 * 输出格式: snnnn.dd (如 "-12.34", "5.67", "178.90")
 * 精度: 2 位小数
 * 返回字符串长度
 * ================================================================ */
static int ftoa(float val, char *buf)
{
    char *p = buf;
    int int_part, frac_part;

    /* 处理负数 */
    if (val < 0.0f) {
        *p++ = '-';
        val = -val;
    }

    /* 整数和小数部分 */
    int_part  = (int)val;
    frac_part = (int)((val - (float)int_part) * 100.0f + 0.5f);

    /* 四舍五入溢出处理 (e.g. 9.996 → 10.00) */
    if (frac_part >= 100) {
        int_part += 1;
        frac_part = 0;
    }

    /* 输出整数部分 */
    if (int_part >= 1000) {
        *p++ = '0' + (int_part / 1000) % 10;
    }
    if (int_part >= 100) {
        *p++ = '0' + (int_part / 100) % 10;
    }
    if (int_part >= 10) {
        *p++ = '0' + (int_part / 10) % 10;
    }
    *p++ = '0' + (int_part % 10);

    /* 小数点 + 2位小数 */
    *p++ = '.';
    *p++ = '0' + (frac_part / 10) % 10;
    *p++ = '0' + (frac_part % 10);

    return (int)(p - buf);
}

/* ================================================================
 * USART2 初始化
 *
 * PA2 = TX (AF_PP), PA3 = RX (IN_FLOATING)
 * 仅使能 TX 模式, 无需接收中断
 * ================================================================ */
void usart2_init(u32 bound)
{
    GPIO_Config_T  GPIO_InitStructure;
    USART_Config_T USART_InitStructure;

    /* USART2 在 APB1 上, GPIOA 在 APB2 上 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_USART2);

    USART_Reset(USART2);

    /* TX: PA2, 复用推挽输出 */
    GPIO_InitStructure.pin   = GPIO_PIN_2;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_AF_PP;
    GPIO_Config(GPIOA, &GPIO_InitStructure);

    /* RX: PA3, 浮空输入 */
    GPIO_InitStructure.pin  = GPIO_PIN_3;
    GPIO_InitStructure.mode = GPIO_MODE_IN_FLOATING;
    GPIO_Config(GPIOA, &GPIO_InitStructure);

    /* USART 配置: 8N1, 仅 TX */
    USART_InitStructure.baudRate     = bound;
    USART_InitStructure.wordLength   = USART_WORD_LEN_8B;
    USART_InitStructure.stopBits     = USART_STOP_BIT_1;
    USART_InitStructure.parity       = USART_PARITY_NONE;
    USART_InitStructure.hardwareFlow = USART_HARDWARE_FLOW_NONE;
    USART_InitStructure.mode         = USART_MODE_TX;   /* 仅发送 */
    USART_Config(USART2, &USART_InitStructure);

    /* 使能 USART2 */
    USART_Enable(USART2);
}

/* ================================================================
 * 发送字符串 (阻塞)
 * ================================================================ */
void USART2_SendString(char *str)
{
    while (*str)
    {
        while (USART_ReadStatusFlag(USART2, USART_FLAG_TXBE) == RESET);
        USART_TxData(USART2, (uint16_t)(*str++));
    }
}

/* ================================================================
 * 发送数据 (lksscope 格式)
 *
 * 格式: pitch,state\r\n
 *   pitch = 俯仰角 (°, 竖放=0° 前倾为正)
 *   state = 姿态状态码:
 *           0=站立中  1=直立稳定(绿灯)
 *           2=25°告警(黄灯)  3=35°告警(红灯)
 *
 * 例如: "15.23,0\r\n"  "28.45,2\r\n"  "36.10,3\r\n"
 * ================================================================ */
void USART2_SendData(float pitch, uint8_t state)
{
    char buf[16];   /* pitch(8) + ',' + state(1) + \r\n = 12, 16足够 */
    char *p = buf;
    int len;

    /* 俯仰角 */
    p += ftoa(pitch, p);
    /* 状态码 (单个数字字符) */
    *p++ = ',';
    *p++ = '0' + (state > 9 ? 9 : state);  /* 安全: 状态码 0~3 */
    *p++ = '\r';
    *p++ = '\n';
    len = (int)(p - buf);

    /* 发送 */
    {
        char *s = buf;
        while (len-- > 0)
        {
            while (USART_ReadStatusFlag(USART2, USART_FLAG_TXBE) == RESET);
            USART_TxData(USART2, (uint16_t)(*s++));
        }
    }
}
