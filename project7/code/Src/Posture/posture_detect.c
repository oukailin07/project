/**
 * posture_detect.c - 姿态检测状态机实现
 *
 * 纯逻辑模块: 不访问任何硬件寄存器, 只做角度→状态判断。
 * 所有输出控制由 main.c 根据 Posture_GetState() 返回的状态来驱动。
 *
 * 状态机:
 *                 ┌──────────────────────────────────────┐
 *                 │                                      │
 *   STRAIGHT ──(│pitch│≤10° 持续3s)──→ STANDING_STABLE   │
 *       │              │                     │           │
 *       │  pitch≥25°   │     pitch≥25°       │           │
 *       │  持续200ms   │     持续200ms       │           │
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
 *   2. 25°需累计 200ms (TILT_25_HOLD_TIME) — 瞬时晃动不触发黄灯告警
 *   3. 35°需累计 500ms (TILT_35_HOLD_TIME) — 瞬时晃动不触发红灯告警
 *   4. 滞回退出 (3°) — 防止阈值边界反复切换
 *   5. 直立需累计 3s — 短暂直立不误清除告警
 */

#include "posture_detect.h"

/* ================================================================
 * 内部状态变量
 * ================================================================ */

static PostureState_t state;            /* 当前姿态状态 */
static float         filtered_pitch;    /* 低通滤波后的俯仰角 */
static uint8_t       filter_inited;     /* 滤波器是否已初始化 */

/* 持续计时器 */
static uint32_t stand_timer_ms;         /* 直立累计时间 (→ 绿灯) */
static uint32_t tilt25_timer_ms;        /* 25°前倾累计时间 (→ 黄灯) */
static uint32_t tilt35_timer_ms;        /* 35°前倾累计时间 (→ 红灯) */

/* ================================================================
 * 初始化
 * ================================================================ */
void Posture_Init(void)
{
    state           = POSTURE_STRAIGHT;
    filtered_pitch  = 0.0f;
    filter_inited   = 0;
    stand_timer_ms  = 0;
    tilt25_timer_ms = 0;
    tilt35_timer_ms = 0;
}

/* ================================================================
 * 判断辅助函数
 * ================================================================ */

/* 是否处于直立姿态 (|pitch| ≤ POSTURE_STAND_MAX, 前后对称) */
static inline int is_standing(float pitch)
{
    float abs_p = pitch > 0 ? pitch : -pitch;
    return (abs_p <= POSTURE_STAND_MAX);
}

/* 是否进入黄灯告警区 (|pitch| ≥ 25°, 前后对称) */
static inline int in_warn_zone(float pitch)
{
    float abs_p = pitch > 0 ? pitch : -pitch;
    return (abs_p >= TILT_WARN_ENTER);
}

/* 是否进入红灯告警区 (|pitch| ≥ 35°, 前后对称) */
static inline int in_alarm_zone(float pitch)
{
    float abs_p = pitch > 0 ? pitch : -pitch;
    return (abs_p >= TILT_ALARM_ENTER);
}

/* 是否退出黄灯区 (|pitch| ≤ 22°, 滞回) */
static inline int exit_warn_zone(float pitch)
{
    float abs_p = pitch > 0 ? pitch : -pitch;
    return (abs_p <= TILT_WARN_EXIT);
}

/* 是否退出红灯区到黄灯区 (22° < |pitch| ≤ 32°) */
static inline int exit_alarm_to_warn(float pitch)
{
    float abs_p = pitch > 0 ? pitch : -pitch;
    return (abs_p <= TILT_ALARM_EXIT && abs_p > TILT_WARN_EXIT);
}

/* 是否退出红灯区到直立区 (|pitch| ≤ 22°) */
static inline int exit_alarm_to_stand(float pitch)
{
    float abs_p = pitch > 0 ? pitch : -pitch;
    return (abs_p <= TILT_WARN_EXIT);
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
                stand_timer_ms  = 0;
                tilt25_timer_ms = 0;
                tilt35_timer_ms = 0;
                break;
            }
        } else {
            stand_timer_ms = 0;
        }

        /* 检查前倾: 35° 优先级高于 25° */
        if (in_alarm_zone(fp)) {
            /* ≥35°: 累计 500ms → 红灯 */
            tilt25_timer_ms = 0;
            tilt35_timer_ms += dt_ms;
            if (tilt35_timer_ms >= TILT_35_HOLD_TIME) {
                state = POSTURE_TILT_ALARM;
                tilt35_timer_ms = 0;
                stand_timer_ms  = 0;
                break;
            }
        } else if (in_warn_zone(fp)) {
            /* 25°~35°: 累计 200ms → 黄灯 (防晃动) */
            tilt35_timer_ms = 0;
            tilt25_timer_ms += dt_ms;
            if (tilt25_timer_ms >= TILT_25_HOLD_TIME) {
                state = POSTURE_TILT_WARN;
                tilt25_timer_ms = 0;
                stand_timer_ms  = 0;
                break;
            }
        } else {
            /* <25°: 未达告警, 清零所有前倾计时 */
            tilt25_timer_ms = 0;
            tilt35_timer_ms = 0;
        }
        break;

    case POSTURE_STANDING_STABLE:
        /* 保持直立 → 绿灯常亮, 无需处理 */
        if (!is_standing(fp)) {
            /* 偏离直立 */
            if (in_alarm_zone(fp)) {
                tilt25_timer_ms = 0;
                tilt35_timer_ms += dt_ms;
                if (tilt35_timer_ms >= TILT_35_HOLD_TIME) {
                    state = POSTURE_TILT_ALARM;
                    tilt35_timer_ms = 0;
                    break;
                }
            } else if (in_warn_zone(fp)) {
                tilt35_timer_ms = 0;
                tilt25_timer_ms += dt_ms;
                if (tilt25_timer_ms >= TILT_25_HOLD_TIME) {
                    state = POSTURE_TILT_WARN;
                    tilt25_timer_ms = 0;
                    break;
                }
            } else {
                /* 微偏但未达告警, 回到计时状态 */
                tilt25_timer_ms = 0;
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
                tilt25_timer_ms = 0;
                tilt35_timer_ms = 0;
                break;
            }
        } else {
            tilt35_timer_ms = 0;
        }

        /* 退出黄灯? (滞回) */
        if (exit_warn_zone(fp)) {
            state = POSTURE_STRAIGHT;
            stand_timer_ms  = 0;
            tilt25_timer_ms = 0;
            tilt35_timer_ms = 0;
            break;
        }
        break;

    case POSTURE_TILT_ALARM:
        /* 退出红灯? */
        if (exit_alarm_to_stand(fp)) {
            state = POSTURE_STRAIGHT;
            stand_timer_ms  = 0;
            tilt25_timer_ms = 0;
            tilt35_timer_ms = 0;
            break;
        } else if (exit_alarm_to_warn(fp)) {
            /* 降级到黄灯 (无需重新计时, 已在告警区) */
            state = POSTURE_TILT_WARN;
            tilt25_timer_ms = 0;
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
