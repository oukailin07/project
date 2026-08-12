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
#define JY61P_TYPE_ANGLE    0x53    /* 角度包类型 (Euler角: Roll/Pitch/Yaw) */
#define JY61P_TYPE_ACCEL    0x51    /* 加速度包类型 (Ax/Ay/Az, 含重力) */
#define JY61P_PACKET_LEN    11      /* 每包11字节 */

/* 加速度计满量程: ±2g → 32768 = 2g, 1g = 16384 LSB */
#define JY61P_ACCEL_1G      16384.0f

/* 角度数据结构 */
typedef struct {
    float roll;     /* 横滚角 (°) */
    float pitch;    /* 俯仰角 (°) */
    float yaw;      /* 偏航角 (°) */
    uint8_t fresh;  /* 有新数据更新标志 (1=有新数据) */

    /* 加速度分量 (g), 用于 gimbal-lock-free 前倾角计算 */
    float ax;       /* X轴加速度 (g) — 站立时沿脊柱方向 */
    float ay;       /* Y轴加速度 (g) — 站立时左右方向 */
    float az;       /* Z轴加速度 (g) — 站立时前后方向 (垂直PCB板面) */
    uint8_t accel_fresh;    /* 加速度数据更新标志 */
} JY61P_Data_t;

/* API */
void JY61P_Init(void);
float JY61P_GetPitch(void);
float JY61P_GetRoll(void);
float JY61P_GetYaw(void);
uint8_t JY61P_DataReady(void);
void JY61P_ClearFlag(void);

/**
 * JY61P_GetForwardLean - 从重力矢量计算 PCB 板面前倾角 (gimbal-lock-free)
 *
 * 核心思路: 不依赖 Euler 角的 pitch (在 90° 附近有万向节死锁)，
 * 而是直接从加速度计读取重力矢量在传感器 XZ 平面上的投影，
 * 计算 PCB 板面法线偏离竖直方向的角度。
 *
 * 只取 XZ 平面 (前后方向) → 天然忽略 Y 轴 (左右/侧边倾斜)。
 *
 * 返回值:
 *   0°  = 站直 (PCB 竖直, 传感器 X+ 轴指向下方)
 *   >0° = 前倾 (正值越大前倾越多)
 *   <0° = 后倾
 */
float JY61P_GetForwardLean(void);

/* 环形缓冲区访问 (调试用) */
uint16_t JY61P_RxAvailable(void);

#endif /* __JY61P_H */
