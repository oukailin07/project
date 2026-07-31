/**
 * posture_detect.h - 姿态检测状态机接口
 *
 * 职责: 纯逻辑模块。接收俯仰角数据 → 状态机判断 → 返回当前状态。
 * 不访问任何硬件寄存器，不控制任何输出。输出控制由 main.c 根据状态完成。
 *
 * 移植: 无需修改本模块。更换硬件只需修改本文件中的阈值宏。
 */

#ifndef __POSTURE_DETECT_H
#define __POSTURE_DETECT_H

#include "main.h"

/* ================================================================
 * 姿态状态枚举
 * ================================================================ */
typedef enum {
    POSTURE_STRAIGHT        = 0,    /* 站立中 (计时未达3秒, 无输出) */
    POSTURE_STANDING_STABLE = 1,    /* 直立稳定 ≥3秒 → 绿灯亮 */
    POSTURE_TILT_WARN       = 2,    /* 前倾 ≥25° → 黄灯闪 + 间歇蜂鸣 + 震动 */
    POSTURE_TILT_ALARM      = 3     /* 前倾 ≥35°持续0.5秒 → 红灯闪 + 长鸣 + 震动 */
} PostureState_t;

/* ================================================================
 * 阈值配置 (可在此文件修改, 覆盖 main.h 默认值)
 * ================================================================ */

/* 进入告警的俯仰角阈值 (°) */
#ifndef TILT_WARN_ENTER
#define TILT_WARN_ENTER     25.0f
#endif

#ifndef TILT_ALARM_ENTER
#define TILT_ALARM_ENTER    35.0f
#endif

/* 退出告警阈值 (含滞回3°, 防止边界振荡) */
#ifndef TILT_WARN_EXIT
#define TILT_WARN_EXIT      22.0f
#endif

#ifndef TILT_ALARM_EXIT
#define TILT_ALARM_EXIT     32.0f
#endif

/* 直立判定: |pitch| ≤ 此值视为站直 */
#ifndef POSTURE_STAND_MAX
#define POSTURE_STAND_MAX   2.0f
#endif

/* ================================================================
 * API
 * ================================================================ */
void Posture_Init(void);

/**
 * Posture_Update - 姿态检测主更新 (每主循环周期调用)
 * @pitch: 当前俯仰角 (°), 正值=前倾
 * @dt_ms: 距上次调用时间间隔 (ms)
 */
void Posture_Update(float pitch, uint16_t dt_ms);

PostureState_t Posture_GetState(void);

#endif /* __POSTURE_DETECT_H */
