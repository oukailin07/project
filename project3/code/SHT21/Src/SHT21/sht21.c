#include "sht21.h"
#include "delay.h"

#define SHT21_ADDR  0x40

#define SCL_H()     GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define SCL_L()     GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define SDA_H()     GPIO_SetBits(GPIOB, GPIO_Pin_1)
#define SDA_L()     GPIO_ResetBits(GPIOB, GPIO_Pin_1)
#define SDA_READ()  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)

static void SDA_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static void SDA_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static void I2C_Delay(void)
{
    delay_us(5);
}

static void I2C_Start(void)
{
    SDA_OUT();
    SDA_H();
    SCL_H();
    I2C_Delay();
    SDA_L();
    I2C_Delay();
    SCL_L();
    I2C_Delay();
}

static void I2C_Stop(void)
{
    SDA_OUT();
    SDA_L();
    SCL_H();
    I2C_Delay();
    SDA_H();
    I2C_Delay();
}

/* Return 1 if ACK received, 0 if NACK */
static u8 I2C_WaitAck(void)
{
    u8 ack;
    SDA_IN();
    SCL_H();
    I2C_Delay();
    ack = SDA_READ() ? 0 : 1;  /* SDA low = ACK */
    SCL_L();
    I2C_Delay();
    SDA_OUT();
    return ack;
}

static void I2C_SendByte(u8 byte)
{
    u8 i;
    SDA_OUT();
    for (i = 0; i < 8; i++)
    {
        if (byte & 0x80)
            SDA_H();
        else
            SDA_L();
        byte <<= 1;
        SCL_H();
        I2C_Delay();
        SCL_L();
        I2C_Delay();
    }
}

static u8 I2C_ReadByte(void)
{
    u8 i, byte = 0;
    SDA_IN();
    for (i = 0; i < 8; i++)
    {
        byte <<= 1;
        SCL_H();
        I2C_Delay();
        if (SDA_READ())
            byte |= 1;
        SCL_L();
        I2C_Delay();
    }
    return byte;
}

static void I2C_SendAck(void)
{
    SDA_OUT();
    SDA_L();
    SCL_H();
    I2C_Delay();
    SCL_L();
    I2C_Delay();
}

static void I2C_SendNack(void)
{
    SDA_OUT();
    SDA_H();
    SCL_H();
    I2C_Delay();
    SCL_L();
    I2C_Delay();
}

void SHT21_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SCL_H();
    SDA_H();
    delay_ms(20);

    /* Soft reset */
    I2C_Start();
    I2C_SendByte(SHT21_ADDR << 1);
    I2C_WaitAck();
    I2C_SendByte(0xFE);
    I2C_WaitAck();
    I2C_Stop();
    delay_ms(20);
}

static u16 SHT21_ReadValue(u8 cmd)
{
    u16 value = 0;
    u16 timeout;

    /* Trigger measurement */
    I2C_Start();
    I2C_SendByte(SHT21_ADDR << 1);
    if (!I2C_WaitAck())
    {
        I2C_Stop();
        return 0;
    }
    I2C_SendByte(cmd);
    if (!I2C_WaitAck())
    {
        I2C_Stop();
        return 0;
    }
    I2C_Stop();

    /* Poll until sensor ready (max ~200ms) */
    timeout = 0;
    while (timeout < 400)
    {
        delay_us(500);
        timeout++;

        I2C_Start();
        I2C_SendByte((SHT21_ADDR << 1) | 1);
        if (I2C_WaitAck())
        {
            /* Sensor ready, read 2 data bytes */
            value = ((u16)I2C_ReadByte()) << 8;
            I2C_SendAck();
            value |= I2C_ReadByte();
            I2C_SendNack();
            I2C_Stop();
            value &= 0xFFFC;
            return value;
        }
        /* Not ready: send STOP and retry */
        I2C_Stop();
    }

    /* Timeout */
    return 0;
}

float SHT21_ReadTemperature(void)
{
    u16 raw = SHT21_ReadValue(0xF3);
    if (raw == 0) return 0.0f;  /* invalid / timeout */
    return -46.85f + 175.72f * ((float)raw / 65536.0f);
}

float SHT21_ReadHumidity(void)
{
    u16 raw = SHT21_ReadValue(0xF5);
    if (raw == 0) return 0.0f;  /* invalid / timeout */
    float rh = -6.0f + 125.0f * ((float)raw / 65536.0f);
    if (rh > 100.0f) rh = 100.0f;
    if (rh < 0.0f)   rh = 0.0f;
    return rh;
}
