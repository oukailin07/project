#include "stm32f10x.h"
#include "key.h"
#include "sys.h"
#include "delay.h"

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    KEY_LED = 0;
}

u8 KEY_Scan(u8 mode)
{
    static u8 key_up = 1;
    if (mode) key_up = 1;
    if (key_up && (KEY0 == 1 || KEY1 == 1 || KEY2 == 1 || KEY3 == 1))
    {
        delay_ms(10);
        key_up = 0;
        KEY_LED = 1;
        if (KEY0 == 1) return KEY0_PRES;
        else if (KEY1 == 1) return KEY1_PRES;
        else if (KEY2 == 1) return KEY2_PRES;
        else if (KEY3 == 1) return KEY3_PRES;
    }
    else if (KEY0 == 0 && KEY1 == 0 && KEY2 == 0 && KEY3 == 0)
    {
        key_up = 1;
        KEY_LED = 0;
    }
    return 0;
}
