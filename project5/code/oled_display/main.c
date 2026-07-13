/**
 * OLED 显示节点 — 超声波测距 + 电机控制终端
 * 硬件: STM32F103C8
 *
 * 外设:
 *   OLED  : SCL-PB12, SDA-PB13   (SSD1306, I2C 软件模拟)
 *   HC-SR04: TRIG-PA0, ECHO-PA1  (超声波测距)
 *   矩阵键盘: ROW-PB4~PB7, COL-PB8,PB9,PB14,PB15 (4×4)
 *   L298N : ENA-PA2, IN1-PB0, IN2-PB1, ENB-PA3, IN3-PB10, IN4-PB11
 *
 * 功能: HC-SR04 实时测距 + 按键控制电机 + OLED 显示
 *   按键控制:
 *     [A] 电机A正转   [B] 电机A反转
 *     [C] 电机B正转   [D] 电机B反转
 *     [#] 全部停止
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
#define MOTOR_SPEED     50    /* 默认电机转速 50% */

int main(void)
{
    char    line_buf[32];
    float   distance = 0.0f;
    char    key;
    s8      motor_a = 0;
    s8      motor_b = 0;
    u8      tick = 0;

    /* ── 初始化所有外设 ──────────────────────────────── */
    delay_init();
    OLED_Init();
    OLED_Clear();
    HC_SR04_Init();
    KEYPAD_Init();
    L298N_Init();

    /* ── 显示静态标签 ────────────────────────────────── */
    OLED_ShowString(0, 0, (u8 *)"HC-SR04 Range", 16);
    OLED_ShowString(0, 2, (u8 *)"Dist: ---.-cm", 16);
    OLED_ShowString(0, 4, (u8 *)"Key: --", 16);
    OLED_ShowString(0, 6, (u8 *)"Motor: Stop   ", 16);

    while (1)
    {
        /* ── 按键扫描 + 电机控制 ──────────────────────── */
        key = KEYPAD_Scan();
        if (key != '\0')
        {
            sprintf(line_buf, "Key: %c  ", key);
            OLED_ShowString(0, 4, (u8 *)line_buf, 16);

            switch (key)
            {
            case 'A':
                motor_a =  MOTOR_SPEED;
                motor_b =  0;
                L298N_MotorA_Set(motor_a);
                L298N_MotorB_Stop();
                OLED_ShowString(0, 6, (u8 *)"Motor: A Fwd  ", 16);
                break;
            case 'B':
                motor_a = -MOTOR_SPEED;
                motor_b =  0;
                L298N_MotorA_Set(motor_a);
                L298N_MotorB_Stop();
                OLED_ShowString(0, 6, (u8 *)"Motor: A Rev  ", 16);
                break;
            case 'C':
                motor_b =  MOTOR_SPEED;
                motor_a =  0;
                L298N_MotorB_Set(motor_b);
                L298N_MotorA_Stop();
                OLED_ShowString(0, 6, (u8 *)"Motor: B Fwd  ", 16);
                break;
            case 'D':
                motor_b = -MOTOR_SPEED;
                motor_a =  0;
                L298N_MotorB_Set(motor_b);
                L298N_MotorA_Stop();
                OLED_ShowString(0, 6, (u8 *)"Motor: B Rev  ", 16);
                break;
            case '#':
                motor_a = 0;
                motor_b = 0;
                L298N_Stop();
                OLED_ShowString(0, 6, (u8 *)"Motor: Stop   ", 16);
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
