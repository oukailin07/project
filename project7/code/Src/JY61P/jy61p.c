/**
 * jy61p.c - JY61P 姿态传感器驱动实现
 *
 * 功能: USART1 中断接收 + 环形缓冲区 + JY61P 11字节数据包解析
 * 移植说明: 修改 main.h 中的 JY61P_* 引脚和波特率定义即可适配不同硬件
 *
 * 由 STM32 移植到 APM32F103: 库 API 采用 APM32 StdPeriph
 */

#include "jy61p.h"

/* APM32 外设库 (STM32 由 conf.h 自动包含, APM32 需显式包含) */
#include "apm32f10x_gpio.h"
#include "apm32f10x_rcm.h"
#include "apm32f10x_usart.h"

/* ================================================================
 * 环形接收缓冲区
 * ================================================================ */
static volatile uint8_t  rx_buf[JY61P_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;   /* ISR 写入位置 */
static volatile uint16_t rx_tail = 0;   /* 主循环读取位置 */

/* ================================================================
 * 解析状态机
 * ================================================================ */
static JY61P_Data_t g_jy61p_data;

/* 解析状态 */
typedef enum {
    PARSE_WAIT_HEAD = 0,    /* 等待 0x55 包头 */
    PARSE_WAIT_TYPE,        /* 等待类型字节 (0x51~0x53) */
    PARSE_DATA              /* 接收数据负载 (8字节) + 校验和(1字节) */
} ParseState_t;

static ParseState_t parse_state = PARSE_WAIT_HEAD;
static uint8_t  parse_buf[JY61P_PACKET_LEN];   /* 当前包缓冲区 */
static uint8_t  parse_idx  = 0;                 /* 当前包写入索引 */

/* ================================================================
 * USART1 初始化
 * ================================================================ */
void JY61P_Init(void)
{
    GPIO_Config_T  GPIO_InitStructure;
    USART_Config_T USART_InitStructure;

    /* 使能时钟: USART1 在 APB2 上, GPIOA 也在 APB2 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | RCM_APB2_PERIPH_USART1);

    USART_Reset(JY61P_USART);

    /* TX: PA9, 复用推挽输出 */
    GPIO_InitStructure.pin   = JY61P_TX_PIN;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_AF_PP;
    GPIO_Config(JY61P_TX_PORT, &GPIO_InitStructure);

    /* RX: PA10, 浮空输入 */
    GPIO_InitStructure.pin  = JY61P_RX_PIN;
    GPIO_InitStructure.mode = GPIO_MODE_IN_FLOATING;
    GPIO_Config(JY61P_RX_PORT, &GPIO_InitStructure);

    /* USART 配置: 波特率可配, 8N1 */
    USART_InitStructure.baudRate     = JY61P_BAUDRATE;
    USART_InitStructure.wordLength   = USART_WORD_LEN_8B;
    USART_InitStructure.stopBits     = USART_STOP_BIT_1;
    USART_InitStructure.parity       = USART_PARITY_NONE;
    USART_InitStructure.hardwareFlow = USART_HARDWARE_FLOW_NONE;
    USART_InitStructure.mode         = USART_MODE_TX_RX;
    USART_Config(JY61P_USART, &USART_InitStructure);

    /* 使能 RXBNE 中断 */
    USART_EnableInterrupt(JY61P_USART, USART_INT_RXBNE);

    /* NVIC 配置 */
    NVIC_SetPriority(JY61P_USART_IRQn, 1);
    NVIC_EnableIRQ(JY61P_USART_IRQn);

    /* 使能 USART */
    USART_Enable(JY61P_USART);
}

/* ================================================================
 * 环形缓冲区操作
 * ================================================================ */
uint16_t JY61P_RxAvailable(void)
{
    if (rx_head >= rx_tail)
        return rx_head - rx_tail;
    else
        return JY61P_RX_BUF_SIZE - rx_tail + rx_head;
}

static uint8_t rx_read(void)
{
    uint8_t byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % JY61P_RX_BUF_SIZE;
    return byte;
}

/* ================================================================
 * 数据包解析 (逐字节状态机, 在主循环中调用)
 * ================================================================ */
static int16_t parse_int16(uint8_t lo, uint8_t hi)
{
    return (int16_t)(((uint16_t)hi << 8) | lo);
}

static void parse_byte(uint8_t byte)
{
    switch (parse_state)
    {
    case PARSE_WAIT_HEAD:
        if (byte == JY61P_PACKET_HEAD) {
            parse_buf[0] = byte;
            parse_idx = 1;
            parse_state = PARSE_WAIT_TYPE;
        }
        break;

    case PARSE_WAIT_TYPE:
        if (byte == JY61P_TYPE_ANGLE) {
            /* 只解析角度包 (0x53), 其他包类型丢弃 */
            parse_buf[1] = byte;
            parse_idx = 2;
            parse_state = PARSE_DATA;
        } else {
            /* 非角度包, 回到等待包头状态 */
            parse_state = PARSE_WAIT_HEAD;
        }
        break;

    case PARSE_DATA:
        parse_buf[parse_idx++] = byte;

        if (parse_idx >= JY61P_PACKET_LEN) {
            /* 收满11字节, 校验和验证 */
            uint8_t sum = 0;
            uint8_t i;
            for (i = 0; i < JY61P_PACKET_LEN - 1; i++) {
                sum += parse_buf[i];
            }

            if (sum == parse_buf[JY61P_PACKET_LEN - 1]) {
                /* 校验通过, 解析角度值 */
                int16_t roll_raw  = parse_int16(parse_buf[2], parse_buf[3]);
                int16_t pitch_raw = parse_int16(parse_buf[4], parse_buf[5]);
                int16_t yaw_raw   = parse_int16(parse_buf[6], parse_buf[7]);

                g_jy61p_data.roll  = (float)roll_raw  / 32768.0f * 180.0f;
                g_jy61p_data.pitch = (float)pitch_raw / 32768.0f * 180.0f;
                g_jy61p_data.yaw   = (float)yaw_raw   / 32768.0f * 180.0f;
                g_jy61p_data.fresh = 1;
            }
            /* 校验失败则丢弃本包, 不影响之前的数据 */

            /* 回到等待下一个包头 */
            parse_state = PARSE_WAIT_HEAD;
            parse_idx = 0;
        }
        break;
    }
}

/* ================================================================
 * 数据处理 (主循环调用: 从环形缓冲区取字节并解析)
 * ================================================================ */
static void JY61P_Process(void)
{
    while (JY61P_RxAvailable() > 0) {
        uint8_t byte = rx_read();
        parse_byte(byte);
    }
}

/* ================================================================
 * 公开 API
 * ================================================================ */

float JY61P_GetPitch(void)
{
    JY61P_Process();    /* 先处理缓冲区的所有字节 */
    return g_jy61p_data.pitch;
}

float JY61P_GetRoll(void)
{
    JY61P_Process();
    return g_jy61p_data.roll;
}

float JY61P_GetYaw(void)
{
    JY61P_Process();
    return g_jy61p_data.yaw;
}

uint8_t JY61P_DataReady(void)
{
    return g_jy61p_data.fresh;
}

void JY61P_ClearFlag(void)
{
    g_jy61p_data.fresh = 0;
}

/* ================================================================
 * USART1 中断服务函数
 * ================================================================ */
void JY61P_USART_IRQHandler(void)
{
    if (USART_ReadIntFlag(JY61P_USART, USART_INT_RXBNE) != RESET)
    {
        uint8_t byte = (uint8_t)USART_RxData(JY61P_USART);

        /* 清除溢出标志 */
        if (USART_ReadStatusFlag(JY61P_USART, USART_FLAG_OVRE) != RESET)
        {
            (void)USART_RxData(JY61P_USART);
        }

        /* 存入环形缓冲区 (非满时) */
        uint16_t next_head = (rx_head + 1) % JY61P_RX_BUF_SIZE;
        if (next_head != rx_tail)
        {
            rx_buf[rx_head] = byte;
            rx_head = next_head;
        }
        /* 缓冲区满则丢字节, 防止死锁 */
    }
}
