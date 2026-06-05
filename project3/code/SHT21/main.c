/**
 * SHT21 Temperature & Humidity Sensor Node
 * Hardware: STM32F103C8
 * SHT21: SCL-PB0, SDA-PB1 (software I2C)
 * UART : TX-PB10, RX-PB11 (USART3, 9600 baud)
 * Sends "T:xx.x H:yy.y\r\n" every 1 second
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "sht21.h"
#include "stdio.h"

int main(void)
{
    char buf[64];
    float temp, humi;

    delay_init();
    usart3_init(9600);
    SHT21_Init();

    while (1)
    {
        temp = SHT21_ReadTemperature();
        humi = SHT21_ReadHumidity();

        sprintf(buf, "T:%.1f H:%.1f\r\n", (double)temp, (double)humi);
        USART3_SendString((u8 *)buf);

        delay_ms(1000);
    }
}
