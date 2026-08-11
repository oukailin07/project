#include "usart.h"

/* APM32 外设库 (STM32 由 conf.h 自动包含, APM32 需显式包含) */
#include "apm32f10x_gpio.h"
#include "apm32f10x_rcm.h"
#include "apm32f10x_usart.h"

/*
 * USART3 驱动 (PB10=TX, PB11=RX)
 * 由 STM32 移植到 APM32F103: 库 API 采用 APM32 StdPeriph
 */

volatile u8  usart3_rx_buf[USART3_RX_BUF_SIZE];
volatile u16 usart3_rx_head = 0;  /* ISR 写入位置 */
volatile u16 usart3_rx_tail = 0;  /* 主循环读取位置 */

void usart3_init(u32 bound)
{
    GPIO_Config_T  GPIO_InitStructure;
    USART_Config_T USART_InitStructure;

    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_USART3);

    USART_Reset(USART3);

    /* TX: PB10 */
    GPIO_InitStructure.pin   = GPIO_PIN_10;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_AF_PP;
    GPIO_Config(GPIOB, &GPIO_InitStructure);

    /* RX: PB11 */
    GPIO_InitStructure.pin  = GPIO_PIN_11;
    GPIO_InitStructure.mode = GPIO_MODE_IN_FLOATING;
    GPIO_Config(GPIOB, &GPIO_InitStructure);

    USART_InitStructure.baudRate     = bound;
    USART_InitStructure.wordLength   = USART_WORD_LEN_8B;
    USART_InitStructure.stopBits     = USART_STOP_BIT_1;
    USART_InitStructure.parity       = USART_PARITY_NONE;
    USART_InitStructure.hardwareFlow = USART_HARDWARE_FLOW_NONE;
    USART_InitStructure.mode         = USART_MODE_TX_RX;

    USART_Config(USART3, &USART_InitStructure);

    /* 使能 RXBNE 中断 */
    USART_EnableInterrupt(USART3, USART_INT_RXBNE);

    /* NVIC: 使能 USART3 中断 (CMSIS 核心函数) */
    NVIC_SetPriority(USART3_IRQn, 0);
    NVIC_EnableIRQ(USART3_IRQn);

    USART_Enable(USART3);
}

void USART3_SendString(u8 *str)
{
    while (*str)
    {
        while (USART_ReadStatusFlag(USART3, USART_FLAG_TXBE) == RESET);
        USART_TxData(USART3, *str++);
    }
}

/* 返回环形缓冲区中可读字节数 */
u16 usart3_rx_available(void)
{
    if (usart3_rx_head >= usart3_rx_tail)
        return usart3_rx_head - usart3_rx_tail;
    else
        return USART3_RX_BUF_SIZE - usart3_rx_tail + usart3_rx_head;
}

/* 从环形缓冲区读取一个字节 (仅当 available() > 0 时调用) */
u8 usart3_rx_read(void)
{
    u8 byte = usart3_rx_buf[usart3_rx_tail];
    usart3_rx_tail = (usart3_rx_tail + 1) % USART3_RX_BUF_SIZE;
    return byte;
}

/* USART3 中断服务函数 — 接收字节存入环形缓冲区 */
void USART3_IRQHandler(void)
{
    if (USART_ReadIntFlag(USART3, USART_INT_RXBNE) != RESET)
    {
        u8 byte = (u8)USART_RxData(USART3);

        /* 检查溢出标志并清除 */
        if (USART_ReadStatusFlag(USART3, USART_FLAG_OVRE) != RESET)
        {
            (void)USART_RxData(USART3);
        }

        /* 缓冲区非满时存入 */
        u16 next_head = (usart3_rx_head + 1) % USART3_RX_BUF_SIZE;
        if (next_head != usart3_rx_tail)
        {
            usart3_rx_buf[usart3_rx_head] = byte;
            usart3_rx_head = next_head;
        }
        /* 缓冲区满则丢字节, 防止死锁 */
    }
}
