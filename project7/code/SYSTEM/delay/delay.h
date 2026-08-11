#ifndef __DELAY_H
#define __DELAY_H

#include "sys.h"

/*
 * 使用 SysTick 普通(非 OS)模式延时, 适用于 APM32F10x 系列
 * 提供 delay_us, delay_ms
 */

void delay_init(void);
void delay_ms(u16 nms);
void delay_us(u32 nus);

#endif
