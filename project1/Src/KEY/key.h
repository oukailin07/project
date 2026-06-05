#ifndef __KEY_H
#define __KEY_H
#include "sys.h"

#define KEY0  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)
#define KEY1  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)
#define KEY2  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5)
#define KEY3  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)

#define KEY0_PRES   1
#define KEY1_PRES   2
#define KEY2_PRES   3
#define KEY3_PRES   4

#define KEY_LED  PAout(15)

void Key_Init(void);
u8 KEY_Scan(u8 mode);

#endif
