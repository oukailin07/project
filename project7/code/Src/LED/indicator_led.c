/**
 * indicator_led.c - 三色 LED 指示灯驱动实现
 *
 * 红灯 PB7, 黄灯 PB5, 绿灯 PB3 (需禁用 JTAG)
 *
 * 由 STM32 移植到 APM32F103: 库 API 采用 APM32 StdPeriph
 */

#include "indicator_led.h"

/* APM32 外设库 (STM32 由 conf.h 自动包含, APM32 需显式包含) */
#include "apm32f10x_gpio.h"
#include "apm32f10x_rcm.h"

void LED_Init(void)
{
    GPIO_Config_T GPIO_InitStructure;

    /* 使能 GPIOB 时钟 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB | RCM_APB2_PERIPH_AFIO);

    /* 禁用 JTAG, 释放 PB3 用作 GPIO */
    GPIO_ConfigPinRemap(GPIO_REMAP_SWJ_JTAGDISABLE);

    /* PB3(绿灯), PB5(黄灯), PB7(红灯) — 全部推挽输出, 初始低电平(灭) */
    GPIO_InitStructure.pin   = GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_OUT_PP;
    GPIO_Config(GPIOB, &GPIO_InitStructure);

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
