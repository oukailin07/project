#include "eeprom.h"
#include "delay.h"

/* Global config variables loaded from EEPROM */
u8   g_ee_language        = 1;    /* default English */
u16  g_ee_p_set           = 300;  /* default 30.0 kPa */
u16  g_ee_pressure_range  = 500;  /* default 50.0 kPa full-scale */
u8   g_ee_version_major   = 1;
u8   g_ee_version_minor   = 0;
u8   g_ee_max_vol         = 70;   /* default 7.0 mL */
u8   g_ee_compress_time   = 10;   /* default 10 min */

/* Software I2C pin definitions: PA8=SCL, PA9=SDA */
#define EE_SCL_H()  GPIO_SetBits(GPIOA, GPIO_Pin_8)
#define EE_SCL_L()  GPIO_ResetBits(GPIOA, GPIO_Pin_8)
#define EE_SDA_H()  GPIO_SetBits(GPIOA, GPIO_Pin_9)
#define EE_SDA_L()  GPIO_ResetBits(GPIOA, GPIO_Pin_9)
#define EE_SDA_IN()  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9)

static void EE_SDA_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void EE_SDA_In(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void EE_Delay(void)
{
    delay_us(5);
}

static void EE_Start(void)
{
    EE_SDA_Out();
    EE_SDA_H();
    EE_SCL_H();
    EE_Delay();
    EE_SDA_L();
    EE_Delay();
    EE_SCL_L();
}

static void EE_Stop(void)
{
    EE_SDA_Out();
    EE_SDA_L();
    EE_SCL_H();
    EE_Delay();
    EE_SDA_H();
    EE_Delay();
}

static u8 EE_WaitAck(void)
{
    u8 ack;
    EE_SDA_In();
    EE_SCL_H();
    EE_Delay();
    ack = EE_SDA_IN();
    EE_SCL_L();
    EE_SDA_Out();
    return ack;
}

static void EE_SendNack(void)
{
    EE_SDA_Out();
    EE_SDA_H();
    EE_SCL_H();
    EE_Delay();
    EE_SCL_L();
}

static void EE_SendByte(u8 data)
{
    u8 i;
    EE_SDA_Out();
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            EE_SDA_H();
        else
            EE_SDA_L();
        data <<= 1;
        EE_SCL_H();
        EE_Delay();
        EE_SCL_L();
        EE_Delay();
    }
}

static u8 EE_RecvByte(void)
{
    u8 i, data = 0;
    EE_SDA_In();
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        EE_SCL_H();
        EE_Delay();
        if (EE_SDA_IN())
            data |= 1;
        EE_SCL_L();
        EE_Delay();
    }
    EE_SDA_Out();
    return data;
}

void EEPROM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA8 = SCL, PA9 = SDA, both open-drain output */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Release bus */
    EE_SCL_H();
    EE_SDA_H();
    delay_ms(10);
}

void EEPROM_WriteByte(u8 addr, u8 data)
{
    EE_Start();
    EE_SendByte(0xA0);          /* device address + write */
    EE_WaitAck();
    EE_SendByte(addr);          /* memory address */
    EE_WaitAck();
    EE_SendByte(data);          /* data */
    EE_WaitAck();
    EE_Stop();
    delay_ms(10);               /* wait for write cycle */
}

u8 EEPROM_ReadByte(u8 addr)
{
    u8 data;

    /* Dummy write to set address */
    EE_Start();
    EE_SendByte(0xA0);
    EE_WaitAck();
    EE_SendByte(addr);
    EE_WaitAck();

    /* Repeated start for read */
    EE_Start();
    EE_SendByte(0xA1);          /* device address + read */
    EE_WaitAck();
    data = EE_RecvByte();
    EE_SendNack();
    EE_Stop();

    return data;
}

void EEPROM_WriteHalfWord(u8 addr, u16 data)
{
    EEPROM_WriteByte(addr, (u8)(data >> 8));
    EEPROM_WriteByte(addr + 1, (u8)(data & 0xFF));
}

u16 EEPROM_ReadHalfWord(u8 addr)
{
    u16 data;
    data  = (u16)EEPROM_ReadByte(addr) << 8;
    data |= (u16)EEPROM_ReadByte(addr + 1);
    return data;
}

void EEPROM_LoadDefaults(void)
{
    g_ee_language        = 1;
    g_ee_p_set           = 300;  /* 30.0 kPa */
    g_ee_pressure_range  = 500;  /* 50.0 kPa full-scale */
    g_ee_version_major   = 1;
    g_ee_version_minor   = 0;
    g_ee_max_vol         = 70;   /* 7.0 mL */
    g_ee_compress_time   = 10;   /* 10 min */
}

void EEPROM_SaveConfig(void)
{
    EEPROM_WriteByte(EE_ADDR_LANGUAGE, g_ee_language);
    EEPROM_WriteHalfWord(EE_ADDR_P_SET_H, g_ee_p_set);
    EEPROM_WriteByte(EE_ADDR_VERSION_MAJOR, g_ee_version_major);
    EEPROM_WriteByte(EE_ADDR_VERSION_MINOR, g_ee_version_minor);
    EEPROM_WriteByte(EE_ADDR_MAX_VOL, g_ee_max_vol);
    EEPROM_WriteByte(EE_ADDR_COMPRESS_TIME, g_ee_compress_time);
    EEPROM_WriteHalfWord(EE_ADDR_P_RANGE_H, g_ee_pressure_range);
}

void EEPROM_LoadConfig(void)
{
    u8 check;

    check = EEPROM_ReadByte(EE_ADDR_LANGUAGE);
    /* Validate: if EEPROM is fresh (0xFF), use defaults */
    if (check == 0xFF || check > 1)
    {
        EEPROM_LoadDefaults();
        EEPROM_SaveConfig();
        return;
    }

    g_ee_language        = check;
    g_ee_p_set           = EEPROM_ReadHalfWord(EE_ADDR_P_SET_H);
    g_ee_version_major   = EEPROM_ReadByte(EE_ADDR_VERSION_MAJOR);
    g_ee_version_minor   = EEPROM_ReadByte(EE_ADDR_VERSION_MINOR);
    g_ee_max_vol         = EEPROM_ReadByte(EE_ADDR_MAX_VOL);
    g_ee_compress_time   = EEPROM_ReadByte(EE_ADDR_COMPRESS_TIME);
    g_ee_pressure_range  = EEPROM_ReadHalfWord(EE_ADDR_P_RANGE_H);
    /* Validate pressure range: must be 10~999.9 kPa */
    if (g_ee_pressure_range < 10 || g_ee_pressure_range > 9999)
        g_ee_pressure_range = 500;
}

void EEPROM_SavePressurePoint(u8 index, u16 pressure_x10)
{
    if (index >= 120) return;
    EEPROM_WriteHalfWord(EE_ADDR_PRESSURE_HIST + index * 2, pressure_x10);
}

u16 EEPROM_ReadPressurePoint(u8 index)
{
    if (index >= 120) return 0;
    return EEPROM_ReadHalfWord(EE_ADDR_PRESSURE_HIST + index * 2);
}
