#ifndef __KEY_H
#define __KEY_H
#include "sys.h"

/* README key mapping:
   KEY1 - PB0 (start/menu)
   KEY2 - PA5 (increment/next)
   KEY3 - PA6 (decrement/back)
*/
#define KEY1  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)
#define KEY2  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5)
#define KEY3  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)

#define KEY1_PRES   1
#define KEY2_PRES   2
#define KEY3_PRES   3

#define KEY_LED  PAout(15)

void Key_Init(void);
u8 KEY_Scan(u8 mode);

#endif
