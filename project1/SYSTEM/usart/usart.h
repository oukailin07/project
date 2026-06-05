#ifndef __USART_H
#define __USART_H

#include "stdio.h"	
#include "sys.h" 

#define USART1_REC_LEN  			200  	//定义最大接收字节数 300	  	
extern u8  USART1_RX_BUF[USART1_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART1_RX_STA;         		//接收状态标记


#define USART2_REC_LEN  			200  	//定义最大接收字节数 300	  	
extern u8  USART2_RX_BUF[USART2_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART2_RX_STA;         		//接收状态标记


#define USART3_REC_LEN  			200  	//定义最大接收字节数 300	  	
extern u8  USART3_RX_BUF[USART3_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART3_RX_STA;         		//接收状态标记

#define USART2_MAX_SEND_LEN		600					//最大发送缓存字节数



void USART1_Init_Config(u32 bound);
void UART1_SendString(char *str);

void USART2_Init_Config(u32 bound);
void UART2_SendString(char *str);

void usart3_init(u32 bound);
void USART3_SendString(u8 *str);

#endif

