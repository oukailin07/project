/**
 * vibrator.c - 震动马达驱动实现 (PB12, 高电平震动)
 *
 * 由 STM32 移植到 APM32F103: 库 API 采用 APM32 StdPeriph
 */

#include "vibrator.h"

/* APM32 外设库 (STM32 由 conf.h 自动包含, APM32 需显式包含) */
#include "apm32f10x_gpio.h"
#include "apm32f10x_rcm.h"

void Vibrator_Init(void)
{
    GPIO_Config_T GPIO_InitStructure;

    /* 使能 GPIOB 时钟 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);

    /* PB12 推挽输出, 初始低电平(不震动) */
    GPIO_InitStructure.pin   = GPIO_PIN_12;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_OUT_PP;
    GPIO_Config(GPIOB, &GPIO_InitStructure);

    VIBRATOR_PIN = 0;
}

void Vibrator_On(void)
{
    VIBRATOR_PIN = 1;
}

void Vibrator_Off(void)
{
    VIBRATOR_PIN = 0;
}
