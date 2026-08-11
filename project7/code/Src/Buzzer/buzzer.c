/**
 * buzzer.c - 无源蜂鸣器 PWM 驱动实现 (TMR2_CH2, PA1)
 *
 * 原理: 无源蜂鸣器没有内部振荡电路，需要外部提供一定频率的方波信号
 *       才能振动发声。本模块通过 TMR2_CH2 输出 2kHz PWM 方波驱动。
 *
 * PA1 引脚: GPIOA_PIN_1 (默认功能), TMR2_CH2 复用推挽输出
 *
 * 系统时钟: HCLK=96MHz, APB1=48MHz (prescaler=2),
 *              TMR2 时钟 = APB1×2 = 96MHz
 *
 * PWM 参数: PSC=95 (96分频 → 1MHz), ARR=499 → f_pwm = 1MHz/500 = 2000Hz
 * 占空比: On=50%(CCR=250), Off=0%(CCR=0)
 *
 * 由 STM32 移植到 APM32F103: TIM2→TMR2, 库 API 采用 APM32 StdPeriph
 */

#include "buzzer.h"

/* APM32 外设库 (STM32 由 conf.h 自动包含, APM32 需显式包含) */
#include "apm32f10x_gpio.h"
#include "apm32f10x_rcm.h"
#include "apm32f10x_tmr.h"

/* ================================================================
 * PWM 参数常量
 * ================================================================ */
#define BUZZER_TMR              TMR2
#define BUZZER_TMR_CHANNEL      2

/* 系统时钟 96MHz → APB1=48MHz → TMR2 clk=96MHz (APB1 prescaler≠1 时倍频) */
#define BUZZER_TMR_PSC          95      /* 预分频: 96MHz / (95+1) = 1MHz  */
#define BUZZER_TMR_ARR          499     /* 重装载: 1MHz / (499+1) = 2kHz  */
#define BUZZER_TMR_CCR_ON       250     /* 50% 占空比 = 发声               */

void Buzzer_Init(void)
{
    GPIO_Config_T   GPIO_InitStructure;
    TMR_BaseConfig_T TMR_TimeBaseStructure;
    TMR_OCConfig_T   TMR_OCInitStructure;

    /* --- 1. 使能时钟 --- */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | RCM_APB2_PERIPH_AFIO);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_TMR2);

    /* --- 2. PA1 配置为复用推挽输出 (TMR2_CH2) --- */
    GPIO_InitStructure.pin   = GPIO_PIN_1;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_AF_PP;   /* 复用推挽输出 */
    GPIO_Config(GPIOA, &GPIO_InitStructure);

    /* --- 3. TMR2 时基配置 (PWM 频率 = 2kHz) --- */
    TMR_TimeBaseStructure.period        = BUZZER_TMR_ARR;   /* ARR = 499 */
    TMR_TimeBaseStructure.division      = BUZZER_TMR_PSC;   /* PSC = 71  */
    TMR_TimeBaseStructure.clockDivision = TMR_CLOCK_DIV_1;
    TMR_TimeBaseStructure.countMode     = TMR_COUNTER_MODE_UP;
    TMR_ConfigTimeBase(BUZZER_TMR, &TMR_TimeBaseStructure);

    /* --- 4. TMR2_CH2 PWM1 模式配置 --- */
    TMR_OCInitStructure.mode        = TMR_OC_MODE_PWM1;     /* PWM 模式1 */
    TMR_OCInitStructure.outputState = TMR_OC_STATE_ENABLE;
    TMR_OCInitStructure.pulse       = 0;                    /* 初始占空比 0% (不响) */
    TMR_OCInitStructure.polarity    = TMR_OC_POLARITY_HIGH; /* 高电平有效 */
    TMR_ConfigOC2(BUZZER_TMR, &TMR_OCInitStructure);

    /* 预装载使能 */
    TMR_ConfigOC2Preload(BUZZER_TMR, TMR_OC_PRELOAD_ENABLE);

    /* --- 5. 启动定时器 --- */
    TMR_Enable(BUZZER_TMR);
}

/**
 * Buzzer_On - 启动蜂鸣器 (PWM 50% 占空比 → 2kHz 方波)
 */
void Buzzer_On(void)
{
    TMR_ConfigCompare2(BUZZER_TMR, BUZZER_TMR_CCR_ON);  /* 250/499 ≈ 50% */
}

/**
 * Buzzer_Off - 关闭蜂鸣器 (占空比 = 0)
 */
void Buzzer_Off(void)
{
    TMR_ConfigCompare2(BUZZER_TMR, 0);
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
