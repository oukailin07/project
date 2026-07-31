/**
 * vibrator.c - 震动马达驱动实现 (PB12, 高电平震动)
 */

#include "vibrator.h"

void Vibrator_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOB 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB12 推挽输出, 初始低电平(不震动) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

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
