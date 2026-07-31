/**
 * indicator_led.h - 三色 LED 指示灯抽象接口
 *
 * 职责: LED 硬件初始化与开关控制。
 * 不关心何时亮/灭 (业务逻辑由 posture_detect 模块负责)。
 *
 * 移植: 修改 main.h 中的 LED_R_PIN / LED_Y_PIN / LED_G_PIN 宏。
 */

#ifndef __INDICATOR_LED_H
#define __INDICATOR_LED_H

#include "main.h"

/* LED 颜色枚举 */
typedef enum {
    LED_RED    = 0,
    LED_YELLOW = 1,
    LED_GREEN  = 2
} LED_Color_t;

/* LED 模式 */
typedef enum {
    LED_OFF = 0,
    LED_ON  = 1,
    LED_TOGGLE = 2
} LED_Mode_t;

/* API */
void LED_Init(void);
void LED_Set(LED_Color_t color, LED_Mode_t mode);
void LED_AllOff(void);

#endif /* __INDICATOR_LED_H */
