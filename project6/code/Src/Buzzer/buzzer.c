/**
 * buzzer.c - 蜂鸣器驱动实现 (PA1, 高电平触发)
 *
 * 移植自 project2 的 Buzzer_Beep 模式
 */

#include "buzzer.h"

void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOA 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA1 推挽输出, 初始低电平(不响) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    BUZZER_PIN = 0;
}

void Buzzer_On(void)
{
    BUZZER_PIN = 1;
}

void Buzzer_Off(void)
{
    BUZZER_PIN = 0;
}

void Buzzer_Toggle(void)
{
    BUZZER_PIN = !BUZZER_PIN;
}

/**
 * Buzzer_Beep - 模式响铃 (阻塞)
 *
 * 每周期: ON (duration_ms) → OFF (duration_ms) → ...
 * 总共响 times 次
 */
void Buzzer_Beep(uint8_t times, uint16_t duration_ms)
{
    uint8_t i;
    for (i = 0; i < times; i++)
    {
        BUZZER_PIN = 1;
        delay_ms(duration_ms);
        BUZZER_PIN = 0;

        /* 两次响铃之间加入间隔 (最后一次不延时间隔) */
        if (i < times - 1)
            delay_ms(duration_ms);
    }
}
