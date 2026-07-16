/**
 * l298n.c - L298N 电机驱动模块（单电机 / 单风扇）
 *
 * 电机: ENA-PA2 (TIM2_CH3), IN1-PB10, IN2-PB11
 *
 * TIM2 PWM: 72MHz / 72 = 1MHz → ARR=999 → 1kHz, 0.1% 分辨率
 * 速度范围 -100~100，映射到 CCR = 0~1000
 */

#include "l298n.h"

/* ── 辅助宏 ──────────────────────────────────────────── */
#define MOTOR_IN1_H()   GPIO_SetBits(L298N_IN1_PORT, L298N_IN1_PIN)
#define MOTOR_IN1_L()   GPIO_ResetBits(L298N_IN1_PORT, L298N_IN1_PIN)
#define MOTOR_IN2_H()   GPIO_SetBits(L298N_IN2_PORT, L298N_IN2_PIN)
#define MOTOR_IN2_L()   GPIO_ResetBits(L298N_IN2_PORT, L298N_IN2_PIN)

/* ── PWM 参数 (动态计算预分频, 兼容 72M/64M/8M 各种时钟) ── */
/* 计数器频率 = 1MHz → PWM = 1kHz @ ARR=999, 分辨率 0.1%      */

/**
 * @brief  初始化 L298N 控制引脚和 TIM2 PWM
 *
 * GPIO:
 *   PA2  → 复用推挽输出 (TIM2_CH3)
 *   PB10, PB11 → 推挽输出 (IN1, IN2)
 *
 * TIM2:
 *   CH3 (PA2) → 电机 PWM
 *   PSC=71, ARR=999 → 1kHz, 分辨率 0.1%
 */
void L298N_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    /* ── 开启时钟 ────────────────────────────────────── */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* ── PWM 引脚: PA2 → 复用推挽 ────────────────────── */
    GPIO_InitStructure.GPIO_Pin   = L298N_ENA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ── 方向引脚: PB10, PB11 → 推挽输出 ────────────── */
    GPIO_InitStructure.GPIO_Pin   = L298N_IN1_PIN | L298N_IN2_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* ── 初始状态: 停止 ──────────────────────────────── */
    MOTOR_IN1_L();
    MOTOR_IN2_L();

    /* ── TIM2 时基: 动态预分频 → 1MHz 计数, PWM=1kHz ── */
    TIM_TimeBaseStructure.TIM_Period        = 999;     /* ARR=999 → 1kHz */
    TIM_TimeBaseStructure.TIM_Prescaler     = (uint16_t)((SystemCoreClock / 1000000U) - 1U);
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* ── TIM2_CH3 (PA2) PWM 输出配置 ─────────────────── */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;    /* 初始占空比 0% */
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);

    /* ── 使能 TIM2 ───────────────────────────────────── */
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief  设置电机转速和方向
 * @param  speed  -100 ~ 100, 正值=正转, 负值=反转, 0=停止
 */
void L298N_Motor_Set(s8 speed)
{
    if (speed > 0)
    {
        /* 正转 */
        MOTOR_IN1_H();
        MOTOR_IN2_L();
        TIM_SetCompare3(TIM2, (u16)speed * 10);  /* speed 0~100 → CCR 0~1000 */
    }
    else if (speed < 0)
    {
        /* 反转 */
        MOTOR_IN1_L();
        MOTOR_IN2_H();
        TIM_SetCompare3(TIM2, (u16)(-speed) * 10);
    }
    else
    {
        /* 停止 */
        L298N_Motor_Stop();
    }
}

/**
 * @brief  停止电机（PWM=0, IN1=IN2=L）
 */
void L298N_Motor_Stop(void)
{
    TIM_SetCompare3(TIM2, 0);
    MOTOR_IN1_L();
    MOTOR_IN2_L();
}
