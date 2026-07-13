#ifndef __USART_H
#define __USART_H

#include "sys.h"

#define USART3_RX_BUF_SIZE 128

extern volatile u8  usart3_rx_buf[USART3_RX_BUF_SIZE];
extern volatile u16 usart3_rx_head;
extern volatile u16 usart3_rx_tail;

void usart3_init(u32 bound);
void USART3_SendString(u8 *str);
u16  usart3_rx_available(void);
u8   usart3_rx_read(void);

#endif
