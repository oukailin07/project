/**
 * indicator_led.c - 三色 LED 指示灯驱动实现
 *
 * 红灯 PB7, 黄灯 PB5, 绿灯 PB3 (需禁用 JTAG)
 */

#include "indicator_led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOB 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* 禁用 JTAG, 释放 PB3 用作 GPIO */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* PB3(绿灯), PB5(黄灯), PB7(红灯) — 全部推挽输出, 初始低电平(灭) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 初始全灭 */
    LED_G_PIN = 0;
    LED_Y_PIN = 0;
    LED_R_PIN = 0;
}

/**
 * LED_Set - 设置指定颜色 LED 的状态
 * @color: LED_RED / LED_YELLOW / LED_GREEN
 * @mode:  LED_OFF / LED_ON / LED_TOGGLE
 */
void LED_Set(LED_Color_t color, LED_Mode_t mode)
{
    uint8_t new_state;

    switch (color)
    {
    case LED_RED:
        if (mode == LED_TOGGLE)
            LED_R_PIN = !LED_R_PIN;
        else
            LED_R_PIN = (mode == LED_ON) ? 1 : 0;
        break;

    case LED_YELLOW:
        if (mode == LED_TOGGLE)
            LED_Y_PIN = !LED_Y_PIN;
        else
            LED_Y_PIN = (mode == LED_ON) ? 1 : 0;
        break;

    case LED_GREEN:
        if (mode == LED_TOGGLE)
            LED_G_PIN = !LED_G_PIN;
        else
            LED_G_PIN = (mode == LED_ON) ? 1 : 0;
        break;
    }
}

/**
 * LED_AllOff - 关闭所有 LED
 */
void LED_AllOff(void)
{
    LED_R_PIN = 0;
    LED_Y_PIN = 0;
    LED_G_PIN = 0;
}
