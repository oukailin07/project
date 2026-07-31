/**
 * vibrator.h - 震动马达抽象接口
 *
 * 职责: 震动马达开关控制 (PB12, 高电平震动)。
 * 移植: 修改 main.h 中的 VIBRATOR_PIN 宏。
 */

#ifndef __VIBRATOR_H
#define __VIBRATOR_H

#include "main.h"

/* API */
void Vibrator_Init(void);
void Vibrator_On(void);
void Vibrator_Off(void);

#endif /* __VIBRATOR_H */
