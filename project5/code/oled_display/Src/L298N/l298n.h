/**
 * l298n.h - L298N 双H桥电机驱动模块
 *
 * 硬件: STM32F103C8
 * 电机A: ENA-PA2 (TIM2_CH3 PWM), IN1-PB0, IN2-PB1
 * 电机B: ENB-PA3 (TIM2_CH4 PWM), IN3-PB10, IN4-PB11
 *
 * 控制逻辑:
 *   IN1=H, IN2=L → 电机A正转
 *   IN1=L, IN2=H → 电机A反转
 *   IN1=L, IN2=L → 电机A停止
 *   ENA → PWM 调速 (0~100%)
 *
 * TIM2: PSC=71, ARR=999 → 1kHz PWM, 0.1% 分辨率
 */

#ifndef __L298N_H
#define __L298N_H

#include "sys.h"

/* ── 电机A引脚 ──────────────────────────────────────── */
#define L298N_ENA_PORT    GPIOA
#define L298N_ENA_PIN     GPIO_Pin_2    /* TIM2_CH3 PWM */

#define L298N_IN1_PORT    GPIOB
#define L298N_IN1_PIN     GPIO_Pin_0

#define L298N_IN2_PORT    GPIOB
#define L298N_IN2_PIN     GPIO_Pin_1

/* ── 电机B引脚 ──────────────────────────────────────── */
#define L298N_ENB_PORT    GPIOA
#define L298N_ENB_PIN     GPIO_Pin_3    /* TIM2_CH4 PWM */

#define L298N_IN3_PORT    GPIOB
#define L298N_IN3_PIN     GPIO_Pin_10

#define L298N_IN4_PORT    GPIOB
#define L298N_IN4_PIN     GPIO_Pin_11

/* ── API 函数声明 ────────────────────────────────────── */

void L298N_Init(void);

void L298N_MotorA_Set(s8 speed);    /* speed: -100 ~ 100, 负值=反转 */
void L298N_MotorB_Set(s8 speed);
void L298N_MotorA_Stop(void);
void L298N_MotorB_Stop(void);
void L298N_Stop(void);              /* 两电机同时停止 */

#endif /* __L298N_H */
