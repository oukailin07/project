/**
 * OLED 显示节点 — 超声波测距 + 风扇控制终端
 *
 * 硬件平台: LCKFB-DKX-STM32F103C8T6 (立创·地阔星 开发板)
 *   - 🔴 板载红色LED: PA1 (高电平点亮, 串联1kΩ)
 *   - 🟢 板载绿色LED: PA2 (高电平点亮, 串联1kΩ, 与L298N ENA共用PWM)
 *   - 板载USB-C: D-=PA11, D+=PA12
 *   - SWD调试: PA13(SWDIO), PA14(SWCLK)
 *   - BOOT0/BOOT1 独立跳线帽, 不在主排针上
 *
 * 外设接线 (参考排针丝印):
 *   OLED  : SCL-PB6, SDA-PB7           (SSD1306, 软件模拟I2C)
 *   HC-SR04: TRIG-PB4, ECHO-PB3         (超声波测距, PB3需禁用JTAG)
 *   矩阵键盘: ROW-PA11/PA10/PA9/PA8, COL-PB15/PB14/PB13/PB12 (4×4)
 *   L298N : ENA-PA2, IN1-PB10, IN2-PB11 (单风扇, PB10/PB11为5V耐受)
 *
 * 功能: HC-SR04 实时测距 + 按键控制风扇 + OLED 显示
 *   按键控制:
 *     [A] 风扇正转   [B] 风扇反转
 *     [#] 风扇停止
 *
 * 注意: PA2同时用作L298N PWM输出和板载绿色LED →
 *        风扇转动时绿色LED亮度随PWM变化, 属正常现象
 */

#include "stm32f10x.h"
#include "delay.h"
#include "oled_disp.h"
#include "hc_sr04.h"
#include "keypad.h"
#include "l298n.h"
#include "stdio.h"

/* ── 刷新周期（每 10ms 循环一次 → 50 次 = 500ms）── */
#define REFRESH_TICKS   50
#define MOTOR_SPEED     50    /* 默认风扇转速 50% */

int main(void)
{
    char    line_buf[32];
    float   distance = 0.0f;
    char    key;
    s8      motor = 0;
    u8      tick = 0;

    /* ── 初始化所有外设 ──────────────────────────────── */
    delay_init();
    OLED_Init();
    OLED_Clear();
    HC_SR04_Init();     /* 内部处理 PB3 JTAG 重映射 */
    KEYPAD_Init();
    L298N_Init();

    /* ── 显示静态标签 ────────────────────────────────── */
    OLED_ShowString(0, 0, (u8 *)"HC-SR04 Range", 16);
    OLED_ShowString(0, 2, (u8 *)"Dist: ---.-cm", 16);
    OLED_ShowString(0, 4, (u8 *)"Key: --", 16);
    OLED_ShowString(0, 6, (u8 *)"Fan: Stop     ", 16);

    while (1)
    {
        /* ── 按键扫描 + 风扇控制 ──────────────────────── */
        key = KEYPAD_Scan();
        if (key != '\0')
        {
            sprintf(line_buf, "Key: %c  ", key);
            OLED_ShowString(0, 4, (u8 *)line_buf, 16);

            switch (key)
            {
            case 'A':
                motor = MOTOR_SPEED;
                L298N_Motor_Set(motor);
                OLED_ShowString(0, 6, (u8 *)"Fan: Fwd      ", 16);
                break;
            case 'B':
                motor = -MOTOR_SPEED;
                L298N_Motor_Set(motor);
                OLED_ShowString(0, 6, (u8 *)"Fan: Rev      ", 16);
                break;
            case '#':
                motor = 0;
                L298N_Motor_Stop();
                OLED_ShowString(0, 6, (u8 *)"Fan: Stop     ", 16);
                break;
            default:
                break;
            }
        }

        /* ── 定时刷新测距（每 REFRESH_TICKS × 10ms）── */
        if (++tick >= REFRESH_TICKS)
        {
            tick = 0;

            distance = HC_SR04_GetDistance();

            if (distance > 0.0f && distance < 450.0f)
            {
                sprintf(line_buf, "Dist:%05.1fcm ", (double)distance);
            }
            else
            {
                sprintf(line_buf, "Dist: ---.-cm ");
            }
            OLED_ShowString(0, 2, (u8 *)line_buf, 16);
        }

        delay_ms(10);
    }
}
