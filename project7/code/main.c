/**
 * main.c - 姿态检测告警系统主程序
 *
 * 功能:
 *   站直3秒 → 绿灯亮
 *   前倾25°±2° → 黄灯闪(0.5秒一次) + 蜂鸣器随灯亮同步响 + 震动
 *   前倾35°±2°持续0.5s → 红灯闪(0.5秒一次) + 蜂鸣器长鸣(一直响) + 震动
 *   晃动防误报: 低通滤波 + 滞回 + 持续计时
 *
 * 硬件: APM32F103CB (由 STM32F103C8 移植) + JY61P姿态传感器
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
#include "SYSTEM/usart2/usart2.h"

/* ================================================================
 * 闪烁/定时常量
 * ================================================================ */
#define FLASH_SLOW_MS   250     /* 黄灯闪烁: 亮250ms/灭250ms, 0.5秒闪一次 */
#define FLASH_FAST_MS   250     /* 红灯闪烁: 亮250ms/灭250ms, 0.5秒闪一次 */
float pitch = 0;
/* ================================================================
 * 主函数
 * ================================================================ */
int main(void)
{
    /* --- 系统初始化 --- */
    SystemClockConfig();            /* 系统时钟配置到 96MHz (HSE×12, system_apm32f10x.c) */
    delay_init();                   /* SysTick 延时 (SYSTEM/delay) */

    /* --- 外设模块初始化 --- */
    LED_Init();                     /* 三色 LED (PB7/PB5/PB3) */
    Buzzer_Init();                  /* 蜂鸣器 (PA1) */
    Vibrator_Init();                /* 震动马达 (PB12) */
    JY61P_Init();                   /* JY61P 传感器 (USART1: PA9/PA10, 9600bps) */
    usart2_init(USART2_BAUDRATE);   /* USART2 日志输出 (PA2/PA3, 921600bps) */
    Posture_Init();                 /* 姿态检测状态机 */

    /* --- 输出时序变量 --- */
    uint32_t flash_timer_ms  = 0;   /* 闪烁计时器 (累计到半周期时翻转) */
    uint32_t log_timer_ms    = 0;   /* USART2 日志发送计时器 */
    uint8_t  yellow_on = 0;         /* 黄灯当前亮灭状态 */
    uint8_t  red_on    = 0;         /* 红灯当前亮灭状态 */

    /* --- 主循环 (100Hz) --- */
    while (1)
    {
        /* 读取 PCB 板面前倾角 (基于加速度计重力矢量, 无万向节死锁)
         * JY61P_GetForwardLean() 直接从 Ax/Az 重力分量计算:
         *   atan2(-az, ax) → 只取 XZ 平面(前后方向), 忽略 Y 轴(左右侧倾)
         *   站直=0°, 前倾>0°, 后倾<0° */
        pitch = JY61P_GetForwardLean();

        /* 姿态检测状态机更新 (10ms 周期) */
        Posture_Update(pitch, MAIN_LOOP_PERIOD);

        /* 获取当前状态 */
        PostureState_t ps = Posture_GetState();

        /* 累计闪烁计时器 */
        flash_timer_ms  += MAIN_LOOP_PERIOD;
        log_timer_ms    += MAIN_LOOP_PERIOD;

        /* --- 根据姿态状态控制输出 --- */
        switch (ps)
        {
        case POSTURE_STRAIGHT:
            /* 站立中(未稳定) — 全灭 */
            LED_AllOff();
            Buzzer_Off();
            Vibrator_Off();
            flash_timer_ms = 0;
            yellow_on = 0;
            red_on = 0;
            break;

        case POSTURE_STANDING_STABLE:
            /* 直立稳定 >=3秒 — 绿灯常亮, 其余灭 */
            LED_Set(LED_RED,    LED_OFF);
            LED_Set(LED_YELLOW, LED_OFF);
            LED_Set(LED_GREEN,  LED_ON);
            Buzzer_Off();
            Vibrator_Off();
            flash_timer_ms = 0;
            yellow_on = 0;
            red_on = 0;
            break;

        case POSTURE_TILT_WARN:
            /* 前倾 >=25° — 黄灯闪(0.5秒一次) + 蜂鸣器随灯亮同步响 */
            LED_Set(LED_GREEN, LED_OFF);
            LED_Set(LED_RED,   LED_OFF);
            Vibrator_On();      /* 震动常开 */

            /* 黄灯闪烁 + 蜂鸣器同步: 每250ms翻转一次, 灯亮蜂鸣器响 */
            if (flash_timer_ms >= FLASH_SLOW_MS) {
                flash_timer_ms = 0;
                yellow_on = !yellow_on;
                LED_Set(LED_YELLOW, yellow_on ? LED_ON : LED_OFF);
                if (yellow_on)
                    Buzzer_On();    /* 灯亮 → 蜂鸣器响 */
                else
                    Buzzer_Off();   /* 灯灭 → 蜂鸣器停 */
            }
            break;

        case POSTURE_TILT_ALARM:
            /* 前倾 >=35°持续0.5秒 — 红灯闪(0.5秒一次) + 蜂鸣器长鸣(一直响) */
            LED_Set(LED_GREEN,  LED_OFF);
            LED_Set(LED_YELLOW, LED_OFF);
            Buzzer_On();        /* 红色告警: 蜂鸣器一直响 */
            Vibrator_On();      /* 震动 */

            /* 红灯闪烁: 每250ms翻转一次 */
            if (flash_timer_ms >= FLASH_FAST_MS) {
                flash_timer_ms = 0;
                red_on = !red_on;
                LED_Set(LED_RED, red_on ? LED_ON : LED_OFF);
            }
            break;
        }

        /* --- USART2 日志: 发送俯仰角 + 姿态状态到 lksscope --- */
        if (log_timer_ms >= USART2_LOG_PERIOD) {
            log_timer_ms = 0;
            /* 格式: pitch,state (state: 0=站立中 1=直立 2=25°告警 3=35°告警) */
            USART2_SendData(pitch, (uint8_t)ps);
        }

        /* 10ms 循环周期 */
        delay_ms(MAIN_LOOP_PERIOD);
    }

    return 0;   /* unreachable */
}
