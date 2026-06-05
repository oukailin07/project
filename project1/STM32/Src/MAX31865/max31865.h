#ifndef __MAX31865_H
#define __MAX31865_H

#include "sys.h"
#include "delay.h"
#include "index.h"

/* Software SPI pin definitions — all on free GPIOs to avoid conflicts */
/* PB6 = SCK, PB7 = MISO, PB8 = MOSI, PB9 = RDY, PC13 = CS */
#define MAX31865_SCK_Port         GPIOB
#define MAX31865_SCK_Pin          GPIO_Pin_6
#define MAX31865_MISO_Port        GPIOB
#define MAX31865_MISO_Pin         GPIO_Pin_7
#define MAX31865_MOSI_Port        GPIOB
#define MAX31865_MOSI_Pin         GPIO_Pin_8
#define MAX31865_CS_Port          GPIOC
#define MAX31865_CS_Pin           GPIO_Pin_13
#define MAX31865_RDY_Port         GPIOB
#define MAX31865_RDY_Pin          GPIO_Pin_9

#define RREF                      (400)

#define RTD_TABLE_TEMP_MIN    -200
#define RTD_TABLE_TEMP_MAX    650

void MAX31865_GPIO_Init(void);
void MAX31865_SB_Write(uint8_t addr, uint8_t wdata);
uint8_t MAX31865_SB_Read(uint8_t addr);
void MAX31865_Init(void);
uint16_t MAX31865_Find_Index(double Rt);
double MAX31865_Conver_Temperature(double Rt);
float MAX31865_Get_Temperature(void);

#endif
