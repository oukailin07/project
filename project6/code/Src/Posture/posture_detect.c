/**
 * posture_detect.c - 姿态检测状态机实现
 *
 * 纯逻辑模块: 不访问任何硬件寄存器, 只做角度→状态判断。
 * 所有输出控制由 main.c 根据 Posture_GetState() 返回的状态来驱动。
 *
 * 状态机:
 *                 ┌──────────────────────────────────────┐
 *                 │                                      │
 *   STRAIGHT ──(│pitch│≤2° 持续3s)──→ STANDING_STABLE    │
 *       │              │                     │           │
 *       │  pitch≥25°   │     pitch≥25°       │           │
 *       ▼              │                     │           │
 *   TILT_WARN ◄────────┘                     │           │
 *       │                                     │          │
 *       │  pitch≥35° & 持续500ms              │           │
 *       ▼                                     │           │
 *   TILT_ALARM                                │           │
 *       │                                     │           │
 *       │  pitch≤32° → TILT_WARN              │           │
 *       │  pitch≤22° → STRAIGHT               │           │
 *       └─────────────────────────────────────┘           │
 *
 * 防误报措施:
 *   1. 一阶低通滤波 (α = FILTER_ALPHA) — 滤除晃动尖峰
 *   2. 滞回退出 (3°) — 防止阈值边界反复切换
 *   3. 35°需累计 500ms — 瞬时晃动不触发红色告警
 *   4. 直立需累计 3s — 短暂直立不误清除告警
 */

#include "posture_detect.h"

/* ================================================================
 * 内部状态变量
 * ================================================================ */

static PostureState_t state;            /* 当前姿态状态 */
static float         filtered_pitch;    /* 低通滤波后的俯仰角 */
static uint8_t       filter_inited;     /* 滤波器是否已初始化 */

/* 计时器 */
static uint32_t stand_timer_ms;         /* 站立累计时间 */
static uint32_t tilt35_timer_ms;        /* 35°前倾累计时间 */

/* ================================================================
 * 初始化
 * ================================================================ */
void Posture_Init(void)
{
    state          = POSTURE_STRAIGHT;
    filtered_pitch = 0.0f;
    filter_inited  = 0;
    stand_timer_ms = 0;
    tilt35_timer_ms = 0;
}

/* ================================================================
 * 判断辅助函数
 * ================================================================ */

/* 是否处于直立姿态 */
static inline int is_standing(float pitch)
{
    return (pitch >= -POSTURE_STAND_MAX && pitch <= POSTURE_STAND_MAX);
}

/* 是否处于黄灯告警区 (带滞回) */
static inline int in_warn_zone(float pitch)
{
    return (pitch >= TILT_WARN_ENTER);
}

/* 是否处于红灯告警区 (带滞回) */
static inline int in_alarm_zone(float pitch)
{
    return (pitch >= TILT_ALARM_ENTER);
}

/* 是否退出黄灯区 (滞回: 低于退出阈值) */
static inline int exit_warn_zone(float pitch)
{
    return (pitch <= TILT_WARN_EXIT);
}

/* 是否退出红灯区到黄灯区 (低于红灯退出阈值, 但高于黄灯退出阈值) */
static inline int exit_alarm_to_warn(float pitch)
{
    return (pitch <= TILT_ALARM_EXIT && pitch > TILT_WARN_EXIT);
}

/* 是否退出红灯区到直立区 */
static inline int exit_alarm_to_stand(float pitch)
{
    return (pitch <= TILT_WARN_EXIT);
}

/* ================================================================
 * 主状态机
 * ================================================================ */

void Posture_Update(float pitch, uint16_t dt_ms)
{
    /* --- 一阶低通滤波 --- */
    if (!filter_inited) {
        filtered_pitch = pitch;
        filter_inited = 1;
    } else {
        filtered_pitch = FILTER_ALPHA * pitch
                       + (1.0f - FILTER_ALPHA) * filtered_pitch;
    }

    float fp = filtered_pitch;

    /* --- 状态机 --- */
    switch (state)
    {
    case POSTURE_STRAIGHT:
        /* 计时: 直立 */
        if (is_standing(fp)) {
            stand_timer_ms += dt_ms;
            if (stand_timer_ms >= STAND_STABLE_TIME) {
                state = POSTURE_STANDING_STABLE;
                stand_timer_ms = 0;
                break;
            }
        } else {
            stand_timer_ms = 0;
        }

        /* 检查前倾告警 */
        if (in_alarm_zone(fp)) {
            tilt35_timer_ms += dt_ms;
            if (tilt35_timer_ms >= TILT_35_HOLD_TIME) {
                state = POSTURE_TILT_ALARM;
                tilt35_timer_ms = 0;
                stand_timer_ms = 0;
                break;
            }
        } else if (in_warn_zone(fp)) {
            tilt35_timer_ms = 0;
            state = POSTURE_TILT_WARN;
            stand_timer_ms = 0;
            break;
        } else {
            tilt35_timer_ms = 0;
        }
        break;

    case POSTURE_STANDING_STABLE:
        /* 保持直立 → 绿灯常亮, 无需处理 */
        if (!is_standing(fp)) {
            /* 偏离直立 */
            if (in_alarm_zone(fp)) {
                tilt35_timer_ms += dt_ms;
                if (tilt35_timer_ms >= TILT_35_HOLD_TIME) {
                    state = POSTURE_TILT_ALARM;
                    tilt35_timer_ms = 0;
                    break;
                }
            } else if (in_warn_zone(fp)) {
                tilt35_timer_ms = 0;
                state = POSTURE_TILT_WARN;
                break;
            } else {
                /* 微偏但未达告警, 回到计时状态 */
                tilt35_timer_ms = 0;
                state = POSTURE_STRAIGHT;
                stand_timer_ms = 0;
                break;
            }
        }
        break;

    case POSTURE_TILT_WARN:
        /* 升级到红灯? */
        if (in_alarm_zone(fp)) {
            tilt35_timer_ms += dt_ms;
            if (tilt35_timer_ms >= TILT_35_HOLD_TIME) {
                state = POSTURE_TILT_ALARM;
                tilt35_timer_ms = 0;
                break;
            }
        } else {
            tilt35_timer_ms = 0;
        }

        /* 退出黄灯? (滞回) */
        if (exit_warn_zone(fp)) {
            state = POSTURE_STRAIGHT;
            stand_timer_ms = 0;
            tilt35_timer_ms = 0;
            break;
        }
        break;

    case POSTURE_TILT_ALARM:
        /* 退出红灯? */
        if (exit_alarm_to_stand(fp)) {
            /* 直接回到直立 */
            state = POSTURE_STRAIGHT;
            stand_timer_ms = 0;
            tilt35_timer_ms = 0;
            break;
        } else if (exit_alarm_to_warn(fp)) {
            /* 降级到黄灯 */
            state = POSTURE_TILT_WARN;
            tilt35_timer_ms = 0;
            break;
        }
        /* 保持红灯 */
        break;
    }
}

/* ================================================================
 * 状态查询
 * ================================================================ */
PostureState_t Posture_GetState(void)
{
    return state;
}
