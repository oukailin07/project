/**
 * l298n.h - L298N 电机驱动模块（单电机 / 单风扇）
 *
 * 硬件: STM32F103C8
 * 电机: ENA-PA2 (TIM2_CH3 PWM), IN1-PB10, IN2-PB11
 *
 * 说明:
 *   - PB10/PB11 为 5V 耐受引脚，适配 L298N 5V 逻辑电平
 *   - 仅驱动单路电机（风扇），省略 IN3/IN4
 *
 * 控制逻辑:
 *   IN1=H, IN2=L → 电机正转
 *   IN1=L, IN2=H → 电机反转
 *   IN1=L, IN2=L → 电机停止
 *   ENA → PWM 调速 (0~100%)
 *
 * TIM2: PSC=71, ARR=999 → 1kHz PWM, 0.1% 分辨率
 */

#ifndef __L298N_H
#define __L298N_H

#include "sys.h"

/* ── 电机引脚 ───────────────────────────────────────── */
#define L298N_ENA_PORT    GPIOA
#define L298N_ENA_PIN     GPIO_Pin_2    /* TIM2_CH3 PWM */

#define L298N_IN1_PORT    GPIOB
#define L298N_IN1_PIN     GPIO_Pin_10   /* 5V 耐受 */

#define L298N_IN2_PORT    GPIOB
#define L298N_IN2_PIN     GPIO_Pin_11   /* 5V 耐受 */

/* ── API 函数声明 ────────────────────────────────────── */

void L298N_Init(void);

void L298N_Motor_Set(s8 speed);     /* speed: -100 ~ 100, 负值=反转 */
void L298N_Motor_Stop(void);

#endif /* __L298N_H */
