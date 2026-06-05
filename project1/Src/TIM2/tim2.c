#include "tim2.h"
#include "delay.h"
#include "stm32f10x.h"

/************************************************/
void PWM_Init(u16 arr,u16 psc)
{		 		
	GPIO_InitTypeDef 		 	 GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef		 TIM_TimeBaseStructure;		
	TIM_OCInitTypeDef  	 		 TIM_OCInitStructure;			  

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 


	/*			IO口管脚配置		*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;	  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  				  				
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	
	/*			定时器3配置			*/
	
	TIM_TimeBaseStructure.TIM_Period            = arr;       
	TIM_TimeBaseStructure.TIM_Prescaler         = psc; 		           
	TIM_TimeBaseStructure.TIM_ClockDivision     = 0; 	 
	TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up; 
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0; 
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);                
	
	/*		PWM通道配置		*/
	TIM_OCInitStructure.TIM_OCMode       = TIM_OCMode_PWM1;   //设置pwm的输出模式    	 
 	TIM_OCInitStructure.TIM_OutputState  = TIM_OutputState_Enable; 
	TIM_OCInitStructure.TIM_Pulse        = arr * 0.2;	//设置占空比      	 	 
	TIM_OCInitStructure.TIM_OCPolarity   = TIM_OCPolarity_Low; //设置pwm的输出极性为高	
	
  TIM_OC2Init(TIM2, &TIM_OCInitStructure);  //初始化     											

	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);  ///使能预装载寄存器   	 
	
	TIM_Cmd(TIM2, ENABLE);
	TIM2->CR1|=0x01;   //使能定时器3 

}

