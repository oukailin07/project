#include "usart.h"

volatile u8  usart3_rx_buf[USART3_RX_BUF_SIZE];
volatile u16 usart3_rx_head = 0;  /* ISR writes here */
volatile u16 usart3_rx_tail = 0;  /* main loop reads from here */

void usart3_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    USART_DeInit(USART3);

    /* TX: PB10 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* RX: PB11 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART3, &USART_InitStructure);

    /* Enable RXNE interrupt */
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /* NVIC: enable USART3 interrupt (CMSIS core functions) */
    NVIC_SetPriority(USART3_IRQn, 0);
    NVIC_EnableIRQ(USART3_IRQn);

    USART_Cmd(USART3, ENABLE);
}

void USART3_SendString(u8 *str)
{
    while (*str)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
        USART_SendData(USART3, *str++);
    }
}

/* Returns number of bytes available in the circular buffer */
u16 usart3_rx_available(void)
{
    if (usart3_rx_head >= usart3_rx_tail)
        return usart3_rx_head - usart3_rx_tail;
    else
        return USART3_RX_BUF_SIZE - usart3_rx_tail + usart3_rx_head;
}

/* Read one byte from the circular buffer. Call only when available() > 0 */
u8 usart3_rx_read(void)
{
    u8 byte = usart3_rx_buf[usart3_rx_tail];
    usart3_rx_tail = (usart3_rx_tail + 1) % USART3_RX_BUF_SIZE;
    return byte;
}

/* USART3 interrupt handler — stores received byte in circular buffer */
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        u8 byte = (u8)USART_ReceiveData(USART3);

        /* Check for overrun and clear it */
        if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) != RESET)
        {
            (void)USART_ReceiveData(USART3);
        }

        /* Store in circular buffer if not full */
        u16 next_head = (usart3_rx_head + 1) % USART3_RX_BUF_SIZE;
        if (next_head != usart3_rx_tail)
        {
            usart3_rx_buf[usart3_rx_head] = byte;
            usart3_rx_head = next_head;
        }
        /* else: buffer full, byte dropped — prevents deadlock */
    }
}
