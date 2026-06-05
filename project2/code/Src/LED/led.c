#include "led.h"
#include "delay.h"

void Alarm_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    BUZZER_PIN = 0;
    ALARM_LED = 0;
}

void Alarm_On(void)
{
    BUZZER_PIN = 1;
    ALARM_LED = 1;
}

void Alarm_Off(void)
{
    BUZZER_PIN = 0;
    ALARM_LED = 0;
}

void Buzzer_Beep(u8 times, u16 duration_ms)
{
    u8 i;
    for (i = 0; i < times; i++)
    {
        BUZZER_PIN = 1;
        delay_ms(duration_ms);
        BUZZER_PIN = 0;
        if (i < times - 1)
            delay_ms(duration_ms);
    }
}
