/**
 * main.c - 姿态检测告警系统主程序
 *
 * 功能:
 *   站直3秒 → 绿灯亮
 *   前倾25°±2° → 黄灯闪 + 间歇蜂鸣(200ms) + 震动
 *   前倾35°±2°持续0.5s → 红灯闪(250ms) + 长鸣 + 震动
 *   晃动防误报: 低通滤波 + 滞回 + 持续计时
 *
 * 硬件: STM32F103C8 + JY61P姿态传感器
 *
 * 架构: 解耦分层
 *   main.c (输出调度) → posture_detect (纯逻辑) → jy61p (数据获取)
 *   main.c (输出调度) → LED/Buzzer/Vibrator (硬件驱动)
 */

#include "main.h"
#include "Src/JY61P/jy61p.h"
#include "Src/LED/indicator_led.h"
#include "Src/Buzzer/buzzer.h"
#include "Src/Motor/vibrator.h"
#include "Src/Posture/posture_detect.h"

/* ================================================================
 * 闪烁/定时常量
 * ================================================================ */
#define FLASH_SLOW_MS   500     /* 黄灯慢闪周期 (亮500ms/灭500ms) */
#define FLASH_FAST_MS   250     /* 红灯快闪周期 (亮250ms/灭250ms) */
#define BUZZER_INTV_MS  200     /* 间歇蜂鸣周期 (响200ms/停200ms) */

/* ================================================================
 * 主函数
 * ================================================================ */
int main(void)
{
    /* --- 系统初始化 --- */
    delay_init();                   /* SysTick 延时 (SYSTEM/delay) */

    /* --- 外设模块初始化 --- */
    LED_Init();                     /* 三色 LED (PB7/PB5/PB3) */
    Buzzer_Init();                  /* 蜂鸣器 (PA1) */
    Vibrator_Init();                /* 震动马达 (PB12) */
    JY61P_Init();                   /* JY61P 传感器 (USART1: PA9/PA10, 9600bps) */
    Posture_Init();                 /* 姿态检测状态机 */

    /* --- 输出时序变量 --- */
    uint32_t flash_timer_ms  = 0;   /* 闪烁计时器 (累计到半周期时翻转) */
    uint32_t buzzer_timer_ms = 0;   /* 间歇蜂鸣计时器 */
    uint8_t  yellow_on = 0;         /* 黄灯当前亮灭状态 */
    uint8_t  red_on    = 0;         /* 红灯当前亮灭状态 */
    uint8_t  buzzer_on  = 0;        /* 蜂鸣器当前状态 */

    /* --- 主循环 (100Hz) --- */
    while (1)
    {
        /* 读取俯仰角 (JY61P 内部自动处理缓冲和解析) */
        float pitch = JY61P_GetPitch();

        /* 姿态检测状态机更新 (10ms 周期) */
        Posture_Update(pitch, MAIN_LOOP_PERIOD);

        /* 获取当前状态 */
        PostureState_t ps = Posture_GetState();

        /* 累计闪烁计时器 */
        flash_timer_ms  += MAIN_LOOP_PERIOD;
        buzzer_timer_ms += MAIN_LOOP_PERIOD;

        /* --- 根据姿态状态控制输出 --- */
        switch (ps)
        {
        case POSTURE_STRAIGHT:
            /* 站立中(未稳定) — 全灭 */
            LED_AllOff();
            Buzzer_Off();
            Vibrator_Off();
            flash_timer_ms = 0;
            buzzer_timer_ms = 0;
            yellow_on = 0;
            red_on = 0;
            buzzer_on = 0;
            break;

        case POSTURE_STANDING_STABLE:
            /* 直立稳定 >=3秒 — 绿灯常亮, 其余灭 */
            LED_Set(LED_RED,    LED_OFF);
            LED_Set(LED_YELLOW, LED_OFF);
            LED_Set(LED_GREEN,  LED_ON);
            Buzzer_Off();
            Vibrator_Off();
            flash_timer_ms = 0;
            buzzer_timer_ms = 0;
            yellow_on = 0;
            red_on = 0;
            buzzer_on = 0;
            break;

        case POSTURE_TILT_WARN:
            /* 前倾 >=25° — 黄灯慢闪 (500ms周期) */
            LED_Set(LED_GREEN, LED_OFF);
            LED_Set(LED_RED,   LED_OFF);
            Vibrator_On();      /* 震动常开 */

            /* 黄灯闪烁: 每500ms翻转一次 */
            if (flash_timer_ms >= FLASH_SLOW_MS) {
                flash_timer_ms = 0;
                yellow_on = !yellow_on;
                LED_Set(LED_YELLOW, yellow_on ? LED_ON : LED_OFF);
            }

            /* 间歇蜂鸣: 每200ms翻转一次 */
            if (buzzer_timer_ms >= BUZZER_INTV_MS) {
                buzzer_timer_ms = 0;
                buzzer_on = !buzzer_on;
                if (buzzer_on)
                    Buzzer_On();
                else
                    Buzzer_Off();
            }
            break;

        case POSTURE_TILT_ALARM:
            /* 前倾 >=35°持续0.5秒 — 红灯快闪 (250ms周期) + 长鸣 + 震动 */
            LED_Set(LED_GREEN,  LED_OFF);
            LED_Set(LED_YELLOW, LED_OFF);
            Buzzer_On();        /* 长鸣 (持续响) */
            Vibrator_On();      /* 震动 */

            /* 红灯快速闪烁: 每250ms翻转一次 */
            if (flash_timer_ms >= FLASH_FAST_MS) {
                flash_timer_ms = 0;
                red_on = !red_on;
                LED_Set(LED_RED, red_on ? LED_ON : LED_OFF);
            }
            break;
        }

        /* 10ms 循环周期 */
        delay_ms(MAIN_LOOP_PERIOD);
    }

    return 0;   /* unreachable */
}
