/**
 * OLED Display Node - Receives temp/humi via UART
 * Hardware: STM32F103C8
 * OLED : SCL-PB12, SDA-PB13 (SSD1306 I2C)
 * UART : TX-PB10, RX-PB11 (USART3, 9600 baud)
 * Parses "T:xx.x H:yy.y\r\n" and displays on OLED
 */

#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

int main(void)
{
    char rx_buf[64];
    u8   rx_idx = 0;
    char line_buf[32];
    float temp = 0.0f, humi = 0.0f;
    u8   has_data = 0;

    delay_init();
    OLED_Init();
    OLED_Clear();
    usart3_init(9600);

    OLED_ShowString(0, 0, (u8 *)"SHT21 Monitor", 16);
    OLED_ShowString(0, 2, (u8 *)"Temp: --.- C", 16);
    OLED_ShowString(0, 4, (u8 *)"Humi: --.- %", 16);

    while (1)
    {
        /* Read all bytes captured by UART interrupt */
        while (usart3_rx_available() > 0)
        {
            char c = (char)usart3_rx_read();
            if (c == '\r' || c == '\n')
            {
                if (rx_idx > 0)
                {
                    rx_buf[rx_idx] = '\0';
                    if (sscanf(rx_buf, "T:%f H:%f", &temp, &humi) == 2)
                    {
                        has_data = 1;
                    }
                    rx_idx = 0;
                }
            }
            else
            {
                if (rx_idx < sizeof(rx_buf) - 1)
                {
                    rx_buf[rx_idx++] = c;
                }
            }
        }

        if (has_data)
        {
            has_data = 0;
            sprintf(line_buf, "Temp:%04.1f C ", (double)temp);
            OLED_ShowString(0, 2, (u8 *)line_buf, 16);
            sprintf(line_buf, "Humi:%04.1f %% ", (double)humi);
            OLED_ShowString(0, 4, (u8 *)line_buf, 16);
        }

        delay_ms(10);
    }
}
