/**
 * main.h - 姿态检测告警系统配置
 *
 * 所有硬件引脚映射、阈值参数、时序常量集中在此文件定义。
 * 移植到不同硬件平台时，只需修改此文件中的引脚映射即可。
 */

#ifndef __MAIN_H
#define __MAIN_H

#include "sys.h"
#include "delay.h"

/* ================================================================
 * 硬件引脚映射 (移植时只需修改此处)
 * ================================================================ */

/* --- 三色 LED (高电平点亮) --- */
#define LED_R_PIN       PBout(7)    /* 红灯: PB7 */
#define LED_Y_PIN       PBout(5)    /* 黄灯: PB5 */
#define LED_G_PIN       PBout(3)    /* 绿灯: PB3 (需禁用JTAG) */

/* --- 蜂鸣器 (高电平触发) --- */
#define BUZZER_PIN      PAout(1)    /* 蜂鸣器: PA1 */

/* --- 震动马达 (高电平震动) --- */
#define VIBRATOR_PIN    PBout(12)   /* 震动马达: PB12 */

/* --- JY61P 姿态传感器 UART --- */
#define JY61P_USART         USART1
#define JY61P_USART_IRQn    USART1_IRQn
#define JY61P_USART_IRQHandler  USART1_IRQHandler
#define JY61P_TX_PORT       GPIOA
#define JY61P_TX_PIN        GPIO_Pin_9     /* PA9  = USART1_TX */
#define JY61P_RX_PORT       GPIOA
#define JY61P_RX_PIN        GPIO_Pin_10    /* PA10 = USART1_RX */
#define JY61P_BAUDRATE      9600           /* JY61P 默认波特率 */
#define JY61P_RX_BUF_SIZE   128            /* 环形接收缓冲区大小 */

/* ================================================================
 * 姿态检测阈值 (移植时可根据实际需求调整)
 * ================================================================ */

#define POSTURE_STAND_MAX       2.0f    /* 直立判定最大角度(°) */
#define POSTURE_TILT_25         25.0f   /* 前倾25°告警阈值 */
#define POSTURE_TILT_35         35.0f   /* 前倾35°告警阈值 */
#define POSTURE_HYSTERESIS      2.0f    /* 滞回(°): 离开告警区需回退2° */
#define POSTURE_TOLERANCE       3.0f    /* 允许误差范围(°) — 兼容25°±2°和35°±2° */

/* ================================================================
 * 时序常量 (单位: ms)
 * ================================================================ */

#define STAND_STABLE_TIME       3000    /* 直立稳定3秒 → 绿灯亮 */
#define TILT_35_HOLD_TIME       500     /* 35°持续0.5秒 → 红灯告警 */
#define MAIN_LOOP_PERIOD        10      /* 主循环周期 10ms (100Hz) */

/* --- LED 闪烁周期 --- */
#define LED_FLASH_SLOW_MS       500     /* 黄灯慢闪周期 (500ms亮/500ms灭) */
#define LED_FLASH_FAST_MS       250     /* 红灯快闪周期 (250ms亮/250ms灭) */

/* --- 蜂鸣器模式参数 --- */
#define BUZZER_BEEP_DURATION    200     /* 间歇蜂鸣: 单次响/停各200ms */
#define BUZZER_BEEP_INTERVAL    200     /* 间歇蜂鸣: 响停间隔 */

/* ================================================================
 * 低通滤波参数
 * ================================================================ */
#define FILTER_ALPHA            0.3f    /* 一阶低通滤波系数 (0~1, 越小越平滑) */

#endif /* __MAIN_H */
