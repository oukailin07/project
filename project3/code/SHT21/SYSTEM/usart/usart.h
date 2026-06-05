#ifndef __USART_H
#define __USART_H

#include "sys.h"

void usart3_init(u32 bound);
void USART3_SendString(u8 *str);

#endif
