#ifndef __TIM2_H
#define __TIM2_H			  	 
#include "sys.h"
void PWM_Init(u16 arr,u16 psc);
#define BeepOn TIM2->CCR1=3600;
#define DeepOff TIM2->CCR1=3600;	`	
#endif

