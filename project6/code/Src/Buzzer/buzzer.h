/**
 * buzzer.h - 蜂鸣器抽象接口
 *
 * 职责: 蜂鸣器硬件控制，支持开关和模式响铃。
 * 移植: 修改 main.h 中的 BUZZER_PIN 宏。
 */

#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

/* API */
void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Toggle(void);

/**
 * Buzzer_Beep - 模式响铃
 * @times:       响铃次数
 * @duration_ms: 每次响铃/间隔时间 (ms)
 *
 * 注意: 此函数使用 delay_ms 阻塞延时。
 *       在非阻塞场景下, 应使用 Buzzer_On/Off + 外部定时。
 */
void Buzzer_Beep(uint8_t times, uint16_t duration_ms);

#endif /* __BUZZER_H */
