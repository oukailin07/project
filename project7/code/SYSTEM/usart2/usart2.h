/**
 * usart2.h - USART2 日志输出接口 (PA2=TX, PA3=RX)
 *
 * 用于将俯仰角 + 姿态状态通过串口2发送到上位机(lksscope)进行绘图。
 * 数据格式: 逗号分隔, 每帧以 \r\n 结尾
 *   pitch,state\r\n
 *   state: 0=站立中 1=直立稳定 2=25°告警 3=35°告警
 */

#ifndef __USART2_H
#define __USART2_H

#include "sys.h"

/* USART2 配置 */
#define USART2_BAUDRATE     115200      /* 波特率 */
#define USART2_LOG_PERIOD   10          /* 发送周期 (ms), 与主循环同步=100Hz */

/* API */
void usart2_init(u32 bound);
void USART2_SendString(char *str);
void USART2_SendData(float pitch, uint8_t state);

#endif /* __USART2_H */
