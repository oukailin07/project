/**
 * buzzer.h - 无源蜂鸣器 PWM 驱动接口 (TIM2_CH2, PA1)
 *
 * 无源蜂鸣器没有内部振荡电路，需要 PWM 方波信号才能发声。
 * 本模块使用 TIM2 通道2 输出 2kHz PWM 驱动无源蜂鸣器。
 *
 * 移植: 若改用其他定时器/通道，修改 main.h 中的宏及 buzzer.c 的 Init。
 */

#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

/* API */
void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);

/**
 * Buzzer_Beep - 模式响铃 (阻塞)
 * @times:       响铃次数
 * @duration_ms: 每次响铃/间隔时间 (ms)
 *
 * 注意: 此函数使用 delay_ms 阻塞延时。
 *       在非阻塞场景下, 应使用 Buzzer_On/Off + 外部定时。
 */
void Buzzer_Beep(uint8_t times, uint16_t duration_ms);

#endif /* __BUZZER_H */
