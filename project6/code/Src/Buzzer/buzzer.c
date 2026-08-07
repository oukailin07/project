/**
 * buzzer.c - 无源蜂鸣器 PWM 驱动实现 (TIM2_CH2, PA1)
 *
 * 原理: 无源蜂鸣器没有内部振荡电路，需要外部提供一定频率的方波信号
 *       才能振动发声。本模块通过 TIM2_CH2 输出 2kHz PWM 方波驱动。
 *
 * PA1 引脚: GPIOA_Pin_1 (默认功能), TIM2_CH2 复用推挽输出
 *
 * 系统时钟假设: HCLK=72MHz, APB1=36MHz (prescaler=2),
 *              TIM2 时钟 = APB1×2 = 72MHz
 *
 * PWM 参数: PSC=71 (72分频 → 1MHz), ARR=499 → f_pwm = 1MHz/500 = 2000Hz
 * 占空比: On=50%(CCR=250), Off=0%(CCR=0)
 */

#include "buzzer.h"

/* ================================================================
 * PWM 参数常量
 * ================================================================ */
#define BUZZER_TIM              TIM2
#define BUZZER_TIM_CHANNEL      2

/* 系统时钟 72MHz → APB1=36MHz → TIM2 clk=72MHz (APB1 prescaler≠1 时倍频) */
#define BUZZER_TIM_PSC          71      /* 预分频: 72MHz / (71+1) = 1MHz  */
#define BUZZER_TIM_ARR          499     /* 重装载: 1MHz / (499+1) = 2kHz  */
#define BUZZER_TIM_CCR_ON       250     /* 50% 占空比 = 发声               */

void Buzzer_Init(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    /* --- 1. 使能时钟 --- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* --- 2. PA1 配置为复用推挽输出 (TIM2_CH2) --- */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;   /* 复用推挽输出 */
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* --- 3. TIM2 时基配置 (PWM 频率 = 2kHz) --- */
    TIM_TimeBaseStructure.TIM_Period        = BUZZER_TIM_ARR;   /* ARR = 499 */
    TIM_TimeBaseStructure.TIM_Prescaler     = BUZZER_TIM_PSC;   /* PSC = 71  */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(BUZZER_TIM, &TIM_TimeBaseStructure);

    /* --- 4. TIM2_CH2 PWM1 模式配置 --- */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;     /* PWM 模式1 */
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;                   /* 初始占空比 0% (不响) */
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High; /* 高电平有效 */
    TIM_OC2Init(BUZZER_TIM, &TIM_OCInitStructure);

    /* 预装载使能 */
    TIM_OC2PreloadConfig(BUZZER_TIM, TIM_OCPreload_Enable);

    /* --- 5. 启动定时器 --- */
    TIM_Cmd(BUZZER_TIM, ENABLE);
}

/**
 * Buzzer_On - 启动蜂鸣器 (PWM 50% 占空比 → 2kHz 方波)
 */
void Buzzer_On(void)
{
    TIM_SetCompare2(BUZZER_TIM, BUZZER_TIM_CCR_ON);  /* 250/499 ≈ 50% */
}

/**
 * Buzzer_Off - 关闭蜂鸣器 (占空比 = 0)
 */
void Buzzer_Off(void)
{
    TIM_SetCompare2(BUZZER_TIM, 0);
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
        Buzzer_On();
        delay_ms(duration_ms);
        Buzzer_Off();

        /* 两次响铃之间加入间隔 (最后一次不延时) */
        if (i < times - 1)
            delay_ms(duration_ms);
    }
}
