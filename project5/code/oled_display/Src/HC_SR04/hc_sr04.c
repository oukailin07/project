/**
 * hc_sr04.c - HC-SR04 超声波测距模块驱动
 *
 * 使用 GPIO 轮询 + delay_us 延时实现测距。
 * TRIG: PA0  — 10us 触发脉冲输出
 * ECHO: PA1  — 回波脉宽测量输入
 *
 * 距离(cm) = 回波脉宽(us) / 58
 */

#include "hc_sr04.h"
#include "delay.h"

/* ── 超时常量 ────────────────────────────────────────── */
#define ECHO_TIMEOUT_US   38000U   /* 约 6.5m 超时, 远超 4m 规格 */
#define TRIG_PULSE_US        10U   /* 触发脉冲宽度 10us */

/**
 * @brief  初始化 HC-SR04 GPIO 引脚
 *         TRIG → PA0, 推挽输出
 *         ECHO → PA1, 浮空输入
 */
void HC_SR04_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* TRIG 引脚: PA0, 推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = HC_TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HC_TRIG_PORT, &GPIO_InitStructure);

    /* ECHO 引脚: PA1, 浮空输入 */
    GPIO_InitStructure.GPIO_Pin   = HC_ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(HC_ECHO_PORT, &GPIO_InitStructure);

    /* 确保 TRIG 初始为低电平 */
    HC_TRIG_L();
}

/**
 * @brief  触发一次测量并返回距离值
 *
 * 时序流程:
 *   1. TRIG 发 10us 高电平脉冲
 *   2. 等待 ECHO 上升沿（超时 ≈ 6.5m 对应的时间）
 *   3. 用 delay_us 计数测量 ECHO 高电平持续时间
 *   4. 换算: 距离(cm) = 脉宽(us) / 58.0
 *
 * @return 距离值（单位: cm），超时/无回波 返回 0.0
 */
float HC_SR04_GetDistance(void)
{
    u32 pulse_us = 0;
    u32 timeout  = 0;

    /* ── 第1步: 发送 10us 触发脉冲 ──────────────────── */
    HC_TRIG_L();
    delay_us(2);
    HC_TRIG_H();
    delay_us(TRIG_PULSE_US);
    HC_TRIG_L();

    /* ── 第2步: 等待 ECHO 上升沿 ────────────────────── */
    timeout = 0;
    while (HC_ECHO_READ() == 0)
    {
        delay_us(1);
        if (++timeout > ECHO_TIMEOUT_US)
            return 0.0f;   /* 无回波 — 超出量程或前方无目标 */
    }

    /* ── 第3步: 测量 ECHO 高电平脉宽 ────────────────── */
    pulse_us = 0;
    while (HC_ECHO_READ() == 1)
    {
        delay_us(1);
        pulse_us++;
        if (pulse_us > ECHO_TIMEOUT_US)
            return 0.0f;   /* 持续高电平 — 传感器异常 */
    }

    /* ── 第4步: 换算为厘米 ──────────────────────────── */
    return (float)pulse_us / 58.0f;
}
