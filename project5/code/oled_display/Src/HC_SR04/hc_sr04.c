/**
 * hc_sr04.c - HC-SR04 超声波测距模块驱动
 *
 * TRIG: PB4  — 10us 触发脉冲输出 (GPIO 推挽)
 * ECHO: PB3  — GPIO 浮空输入 (轮询边沿)
 *
 * 使用 TIM4 硬件计数器 (1MHz) 计时 + GPIO 轮询边沿:
 *   - 脉宽测量由 TIM4->CNT 瞬间读取, 精度 1us
 *   - 不依赖中断, 兼容 Proteus 仿真环境
 *   - 超时检测也由 TIM4 提供真实时间基准
 *
 * 距离(cm) = 回波脉宽(us) / 58
 * 量程: 2cm ~ 400cm, 精度: ~3mm
 */

#include "hc_sr04.h"
#include "delay.h"

/* ── 常量 ──────────────────────────────────────────── */
#define ECHO_TIMEOUT_US   38000U    /* 超时 ≈ 655cm (超过 HC-SR04 4m 规格) */
#define TRIG_PULSE_US        10U    /* 触发脉冲宽度 10us                    */

/*
 * TIM4 时钟 = HCLK = SystemCoreClock (因为 PPRE1_DIV2 时 APB1 定时器倍频)
 * 动态计算预分频，使计数器 = 1MHz = 1us/tick
 *   例: 72MHz → PSC=71, 64MHz → PSC=63, 8MHz → PSC=7
 * 这样可以兼容 HSE+PLL(72M) / HSI+PLL(64M) / HSI(8M) 各种时钟
 */

/* ── 辅助宏: TIM4 计时 ────────────────────────────── */
static uint16_t tim4_read(void)
{
    return TIM_GetCounter(TIM4);
}

/* 计算两次 CNT 读数之间的 us 数 (处理 16 位翻转) */
static uint32_t tim4_elapsed(uint16_t start, uint16_t end)
{
    if (end >= start)
        return end - start;
    else
        return (uint32_t)(0xFFFFU - start) + (uint32_t)end + 1U;
}

/* ═══════════════════════════════════════════════════════
 *  HC_SR04_Init
 * ═══════════════════════════════════════════════════════ */
void HC_SR04_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    /* ── 时钟 ──────────────────────────────────────── */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    /* ── 禁用 JTAG, 释放 PB3/PB4 ────────────────────── */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* ── TRIG: PB4, 推挽输出 ───────────────────────── */
    GPIO_InitStructure.GPIO_Pin   = HC_TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HC_TRIG_PORT, &GPIO_InitStructure);

    /* ── ECHO: PB3, 浮空输入 ───────────────────────── */
    GPIO_InitStructure.GPIO_Pin   = HC_ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(HC_ECHO_PORT, &GPIO_InitStructure);

    HC_TRIG_L();

    /* ── TIM4: 1MHz 自由运行计数器 ─────────────────── */
    TIM_TimeBaseStructure.TIM_Period        = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler     = (uint16_t)((SystemCoreClock / 1000000U) - 1U);
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_Cmd(TIM4, ENABLE);
}

/* ═══════════════════════════════════════════════════════
 *  HC_SR04_GetDistance — 触发一次测距
 *
 *  返回: 距离(cm), 超时/错误返回 0.0
 * ═══════════════════════════════════════════════════════ */
float HC_SR04_GetDistance(void)
{
    uint16_t t_rise;
    uint16_t t_fall;
    uint16_t t_start;
    uint32_t elapsed;
    uint32_t pulse_us;

    /* ── 第1步: 发送 10us 触发脉冲 ──────────────────── */
    HC_TRIG_L();
    delay_us(2);
    HC_TRIG_H();
    delay_us(TRIG_PULSE_US);
    HC_TRIG_L();

    /* ── 第2步: 等待 ECHO 上升沿 (TIM4 真实超时) ────── */
    t_start = tim4_read();
    while (HC_ECHO_READ() == 0)
    {
        elapsed = tim4_elapsed(t_start, tim4_read());
        if (elapsed > ECHO_TIMEOUT_US)
            return 0.0f;
    }
    t_rise = tim4_read();   /* 上升沿时刻 → 瞬间锁存 */

    /* ── 第3步: 等待 ECHO 下降沿 ────────────────────── */
    t_start = tim4_read();
    while (HC_ECHO_READ() == 1)
    {
        elapsed = tim4_elapsed(t_start, tim4_read());
        if (elapsed > ECHO_TIMEOUT_US)
            return 0.0f;
    }
    t_fall = tim4_read();   /* 下降沿时刻 → 瞬间锁存 */

    /* ── 第4步: 计算脉宽 ───────────────────────────── */
    pulse_us = tim4_elapsed(t_rise, t_fall);

    /* ── 第5步: 换算为厘米 ─────────────────────────── */
    return (float)pulse_us / 58.0f;
}
