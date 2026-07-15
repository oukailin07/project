/**
 * hc_sr04.h - HC-SR04 超声波测距模块驱动
 *
 * 硬件: STM32F103C8
 * TRIG: PB4  (GPIO 推挽输出)
 * ECHO: PB3  (GPIO 浮空输入)
 *
 * 工作原理:
 *   1. TRIG 发送 10us 高电平脉冲
 *   2. HC-SR04 发射 8 个 40kHz 超声波脉冲
 *   3. ECHO 输出高电平，持续时间与距离成正比
 *   4. 距离(cm) = 脉宽(us) / 58
 *
 * 量程: 2cm ~ 400cm, 精度: ~3mm
 */

#ifndef __HC_SR04_H
#define __HC_SR04_H

#include "sys.h"

/* ── 引脚定义 ───────────────────────────────────────── */
#define HC_TRIG_PORT    GPIOB
#define HC_TRIG_PIN     GPIO_Pin_4
#define HC_ECHO_PORT    GPIOB
#define HC_ECHO_PIN     GPIO_Pin_3

#define HC_TRIG_H()     GPIO_SetBits(HC_TRIG_PORT, HC_TRIG_PIN)
#define HC_TRIG_L()     GPIO_ResetBits(HC_TRIG_PORT, HC_TRIG_PIN)
#define HC_ECHO_READ()  GPIO_ReadInputDataBit(HC_ECHO_PORT, HC_ECHO_PIN)

/* ── API 函数声明 ────────────────────────────────────── */

void  HC_SR04_Init(void);
float HC_SR04_GetDistance(void);

#endif /* __HC_SR04_H */
