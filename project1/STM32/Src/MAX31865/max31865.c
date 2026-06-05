#include "max31865.h"

static void MAX31865_SPI_Delay(void)
{
    delay_ms(1);
}

static uint8_t MAX31865_SPI_Transfer(uint8_t tx_byte)
{
    uint8_t rx_byte = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        /* Set MOSI, then SCK low (falling edge = data change) */
        if (tx_byte & 0x80)
            GPIO_SetBits(MAX31865_MOSI_Port, MAX31865_MOSI_Pin);
        else
            GPIO_ResetBits(MAX31865_MOSI_Port, MAX31865_MOSI_Pin);
        tx_byte <<= 1;

        MAX31865_SPI_Delay();

        GPIO_ResetBits(MAX31865_SCK_Port, MAX31865_SCK_Pin);  /* SCK = 0 */
        MAX31865_SPI_Delay();

        GPIO_SetBits(MAX31865_SCK_Port, MAX31865_SCK_Pin);    /* SCK = 1 */
        MAX31865_SPI_Delay();

        /* Sample MISO on rising edge */
        rx_byte <<= 1;
        if (GPIO_ReadInputDataBit(MAX31865_MISO_Port, MAX31865_MISO_Pin))
            rx_byte |= 1;
    }

    return rx_byte;
}

void MAX31865_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    /* SCK (PB6), MOSI (PB8) — push-pull outputs */
    GPIO_InitStructure.GPIO_Pin = MAX31865_SCK_Pin | MAX31865_MOSI_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(MAX31865_SCK_Port, MAX31865_SCK_Pin);     /* SCK idle high (CPOL=1) */
    GPIO_ResetBits(MAX31865_MOSI_Port, MAX31865_MOSI_Pin);

    /* MISO (PB7) — input pull-up */
    GPIO_InitStructure.GPIO_Pin = MAX31865_MISO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* CS (PC13) — push-pull output, default high */
    GPIO_InitStructure.GPIO_Pin = MAX31865_CS_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(MAX31865_CS_Port, &GPIO_InitStructure);
    GPIO_SetBits(MAX31865_CS_Port, MAX31865_CS_Pin);

    /* RDY (PB9) — input pull-up */
    GPIO_InitStructure.GPIO_Pin = MAX31865_RDY_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(MAX31865_RDY_Port, &GPIO_InitStructure);
}

void MAX31865_SB_Write(uint8_t addr, uint8_t wdata)
{
    GPIO_ResetBits(MAX31865_CS_Port, MAX31865_CS_Pin);
    MAX31865_SPI_Delay();
    MAX31865_SPI_Transfer(addr | 0x80);
    MAX31865_SPI_Transfer(wdata);
    MAX31865_SPI_Delay();
    GPIO_SetBits(MAX31865_CS_Port, MAX31865_CS_Pin);
    MAX31865_SPI_Delay();
}

uint8_t MAX31865_SB_Read(uint8_t addr)
{
    uint8_t data;

    GPIO_ResetBits(MAX31865_CS_Port, MAX31865_CS_Pin);
    MAX31865_SPI_Delay();
    MAX31865_SPI_Transfer(addr & 0x7F);
    data = MAX31865_SPI_Transfer(0xFF);
    MAX31865_SPI_Delay();
    GPIO_SetBits(MAX31865_CS_Port, MAX31865_CS_Pin);
    MAX31865_SPI_Delay();

    return data;
}

void MAX31865_Init(void)
{
    MAX31865_GPIO_Init();

    MAX31865_SB_Write(0x80, 0xC1);
    delay_ms(100);
}

uint16_t MAX31865_Find_Index(double Rt)
{
    uint16_t i;

    for (i = 0; i < RTD_TABLE_SIZE; i++)
    {
        if (RTD_Table[i] > Rt)
            return (i - 1);
    }
    return RTD_TABLE_SIZE;
}

double MAX31865_Conver_Temperature(double Rt)
{
    unsigned short Index;
    double temperature;

    if (Rt < RTD_Table[0])
        return 0;

    if (Rt > RTD_Table[RTD_TABLE_SIZE - 1])
        return 0;

    Index = MAX31865_Find_Index(Rt);
    temperature = (Rt - RTD_Table[Index]) / (RTD_Table[Index + 1] - RTD_Table[Index])
                + Index + RTD_TABLE_TEMP_MIN;
    return temperature;
}

float MAX31865_Get_Temperature(void)
{
    unsigned int Data;
    float Rt;
    float temperature;

    Data  = MAX31865_SB_Read(0x01) << 8;
    Data |= MAX31865_SB_Read(0x02);
    Data >>= 1;

    Rt = (float)Data / 32768.0f * RREF;
    temperature = MAX31865_Conver_Temperature(Rt);

    return temperature;
}
