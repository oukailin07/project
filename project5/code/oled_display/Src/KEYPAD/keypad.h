/**
 * keypad.h - 4×4 矩阵按键模块驱动
 *
 * 硬件: STM32F103C8
 * 行线(ROW): PA11, PA10, PA9, PA8  (推挽输出)
 * 列线(COL): PB15, PB14, PB13, PB12 (上拉输入)
 *
 * 按键布局:
 *   [1]  [2]  [3]  [A]
 *   [4]  [5]  [6]  [B]
 *   [7]  [8]  [9]  [C]
 *   [*]  [0]  [#]  [D]
 *
 * 扫描方式: 逐行拉低，读取列电平，消抖处理
 */

#ifndef __KEYPAD_H
#define __KEYPAD_H

#include "sys.h"

/* ── 引脚定义 ───────────────────────────────────────── */
#define KP_ROW_PORT     GPIOA
#define KP_ROW1_PIN     GPIO_Pin_11
#define KP_ROW2_PIN     GPIO_Pin_10
#define KP_ROW3_PIN     GPIO_Pin_9
#define KP_ROW4_PIN     GPIO_Pin_8
#define KP_ROW_PINS     (KP_ROW1_PIN | KP_ROW2_PIN | KP_ROW3_PIN | KP_ROW4_PIN)

#define KP_COL_PORT     GPIOB
#define KP_COL1_PIN     GPIO_Pin_15
#define KP_COL2_PIN     GPIO_Pin_14
#define KP_COL3_PIN     GPIO_Pin_13
#define KP_COL4_PIN     GPIO_Pin_12
#define KP_COL_PINS     (KP_COL1_PIN | KP_COL2_PIN | KP_COL3_PIN | KP_COL4_PIN)

/* ── API 函数声明 ────────────────────────────────────── */

void KEYPAD_Init(void);
char KEYPAD_Scan(void);

#endif /* __KEYPAD_H */
