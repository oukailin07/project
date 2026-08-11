/**
 * jy61p.h - JY61P 姿态传感器驱动接口
 *
 * 职责: USART1 中断接收 + JY61P 数据包解析，仅提供角度数据。
 * 不包含任何业务逻辑（姿态判断、告警控制等）。
 *
 * 移植: 修改 main.h 中的 JY61P_* 宏即可。
 */

#ifndef __JY61P_H
#define __JY61P_H

#include "main.h"

/* JY61P 数据包常量 */
#define JY61P_PACKET_HEAD   0x55    /* 数据包头 */
#define JY61P_TYPE_ANGLE    0x53    /* 角度包类型 */
#define JY61P_PACKET_LEN    11      /* 每包11字节 */

/* 角度数据结构 */
typedef struct {
    float roll;     /* 横滚角 (°) */
    float pitch;    /* 俯仰角 (°) */
    float yaw;      /* 偏航角 (°) */
    uint8_t fresh;  /* 有新数据更新标志 (1=有新数据) */
} JY61P_Data_t;

/* API */
void JY61P_Init(void);
float JY61P_GetPitch(void);
float JY61P_GetRoll(void);
float JY61P_GetYaw(void);
uint8_t JY61P_DataReady(void);
void JY61P_ClearFlag(void);

/* 环形缓冲区访问 (调试用) */
uint16_t JY61P_RxAvailable(void);

#endif /* __JY61P_H */
