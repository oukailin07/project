/**
 * 乒乓球 PID 高度悬浮控制系统
 *
 * 硬件平台: LCKFB-DKX-STM32F103C8T6 (立创·地阔星)
 *
 * 结构示意 (侧视图):
 *   ┌──────────────────────┐  ← 圆筒顶部
 *   │  HC-SR04 超声波       │  传感器朝下，测量到小球距离 D
 *   │  ═══════════════════ │
 *   │                      │
 *   │      ██ 乒乓球 ██     │  ← 悬浮在气流中
 *   │                      │  小球距筒底高度 H = 35 - D
 *   │  ═══════════════════ │
 *   │    ↑↑↑ 风扇 ↑↑↑      │  ← 圆筒底部，向上吹气
 *   └──────────────────────┘
 *   筒高 35cm, 小球高度范围 5~30cm
 *
 * 外设接线:
 *   OLED  : SCL-PB6, SDA-PB7
 *   HC-SR04: TRIG-PB4, ECHO-PB3 (顶部朝下)
 *   矩阵键盘: ROW-PA11/PA10/PA9/PA8, COL-PB15/PB14/PB13/PB12
 *   L298N : ENA-PA2, IN1-PB10, IN2-PB11 → OUT1/OUT2 → 风扇
 *
 * 操作说明:
 *   两位数输入目标高度(距筒底 cm):
 *     例: [1][0]→10cm  [1][5]→15cm  [2][0]→20cm
 *   [#]  急停 (风扇关)
 *   [*]  取消输入
 *   [A]  10cm  [B]  15cm  [C]  20cm  [D]  25cm
 */

#include "stm32f10x.h"
#include "delay.h"
#include "oled_disp.h"
#include "hc_sr04.h"
#include "keypad.h"
#include "l298n.h"
#include "pid.h"
#include "stdio.h"

/* ═══════════════════════════════════════════════════════
 *  系统参数
 * ═══════════════════════════════════════════════════════ */

#define TUBE_HEIGHT       35.0f  /* 筒高(cm)                             */
#define TARGET_MIN         5.0f  /* 最小高度(cm)                         */
#define TARGET_MAX        30.0f  /* 最大高度(cm)                         */
#define SONIC_MIN          2.0f  /* 超声波盲区(cm)                       */

/* ── PID 参数 (整定时修改) ────────────────────────── */
#define PID_KP             3.0f  /* 比例: 风扇% / cm误差                 */
#define PID_KI             0.05f /* 积分: 消除稳态误差                   */
#define PID_KD             1.5f  /* 微分: 抑制振荡                       */
#define PID_INTEGRAL_LIMIT 30.0f /* 积分限幅(防饱和)                     */

/* ── 滤波参数 ────────────────────────────────────── */
#define HEIGHT_FILTER_A    0.3f  /* 低通滤波系数 (0~1, 越小越平滑)       */

/* ── 时序 ────────────────────────────────────────── */
#define CONTROL_MS          20   /* PID 周期 → 50Hz                       */
#define DISPLAY_MS         250   /* OLED 刷新                             */
/* ═══════════════════════════════════════════════════════
 *  全局状态
 * ═══════════════════════════════════════════════════════ */

static PID_t  g_pid;
static float  g_target     = 0.0f;   /* 目标高度(cm)                      */
static float  g_height     = 0.0f;   /* 当前高度(cm), 滤波后              */
static float  g_height_raw = 0.0f;   /* 原始高度(cm)                      */
static s8     g_fan_pct    = 0;      /* 风扇转速 0~100                    */
static u8     g_running    = 0;      /* 1=运行中                          */
static u8     g_has_reading = 0;     /* 1=超声波已读到过有效值              */
static float  g_sonic_raw  = 0.0f;   /* 最近一次原始超声波距离(cm)          */
static u8     g_launch_boost = 0;    /* 1=起飞助推中, 风扇满功率             */
#define LAUNCH_CATCH_H    3.0f       /* 球离目标 < 3cm 时退出助推            */
#define LAUNCH_MIN_RISE   0.5f       /* 球至少上升 0.5cm 才允许退出助推       */


/* ═══════════════════════════════════════════════════════
 *  超声波 → 高度 转换
 * ═══════════════════════════════════════════════════════ */

static float sonic_to_height(float sonic_cm)
{
    /*
     * 有效范围: SONIC_MIN(2cm) < D < TUBE_HEIGHT + margin
     *
     * 球在管底时 D≈35cm, 需允许 (之前写 >=34 就拒, 是bug)
     * 球贴近传感器时 D≈2cm (盲区下限)
     * D=0 表示 HC-SR04 超时无回波, 应拒绝
     */
    if (sonic_cm <= SONIC_MIN || sonic_cm > (TUBE_HEIGHT + 5.0f))
        return -1.0f;
    return TUBE_HEIGHT - sonic_cm;
}

/* ═══════════════════════════════════════════════════════
 *  目标高度设置
 * ═══════════════════════════════════════════════════════ */

static void set_target(float t)
{
    if (t < TARGET_MIN)  t = TARGET_MIN;
    if (t > TARGET_MAX)  t = TARGET_MAX;

    g_target       = t;
    g_launch_boost = 1;          /* 启动助推: 满功率直到球接近目标 */
    PID_Setpoint(&g_pid, t);
    PID_Init(&g_pid, PID_KP, PID_KI, PID_KD,
             PID_INTEGRAL_LIMIT, 0.0f, 100.0f);
    g_running = 1;
}

/* ═══════════════════════════════════════════════════════
 *  急停
 * ═══════════════════════════════════════════════════════ */

static void estop(void)
{
    g_running     = 0;
    g_fan_pct     = 0;
    g_target      = 0.0f;
    g_has_reading  = 0;
    g_sonic_raw    = 0.0f;
    g_launch_boost = 0;
    L298N_Motor_Stop();
    PID_Init(&g_pid, PID_KP, PID_KI, PID_KD,
             PID_INTEGRAL_LIMIT, 0.0f, 100.0f);
}

/* ═══════════════════════════════════════════════════════
 *  主函数
 * ═══════════════════════════════════════════════════════ */

int main(void)
{
    char   line[32];
    char   key;
    float  sonic;
    float  h_raw, pid_out;
    u8     fan;

    u16    tick10ms  = 0;     /* 10ms 节拍计数                       */

    /* ── 初始化 ────────────────────────────────────── */
    delay_init();
    OLED_Init();
    OLED_Clear();
    HC_SR04_Init();
    KEYPAD_Init();
    L298N_Init();

    PID_Init(&g_pid, PID_KP, PID_KI, PID_KD,
             PID_INTEGRAL_LIMIT, 0.0f, 100.0f);

    /* ── 静态标签 ──────────────────────────────────── */
    OLED_ShowString(0, 0, (u8 *)"Levitation Ctrl", 16);
    OLED_ShowString(0, 2, (u8 *)"H: ---.-cm",     16);
    OLED_ShowString(0, 4, (u8 *)"T: --.-  F:---",  16);
    OLED_ShowString(0, 6, (u8 *)"Key: --          ", 16);

    /* ══════════════════════════════════════════════════
     *  主循环 (10ms 节拍)
     * ══════════════════════════════════════════════════ */
    while (1)
    {
        /* ── 1. 按键处理: 单键预设目标高度 ──────────── */
        key = KEYPAD_Scan();
        if (key != '\0')
        {
            switch (key)
            {
            /* 数字键 → 直接映射到目标高度 */
            case '0': set_target(10.0f); break;
            case '1': set_target(15.0f); break;
            case '2': set_target(16.0f); break;
            case '3': set_target(17.0f); break;
            case '4': set_target(18.0f); break;
            case '5': set_target(19.0f); break;
            case '6': set_target(20.0f); break;
            case '7': set_target(21.0f); break;
            case '8': set_target(22.0f); break;
            case '9': set_target(23.0f); break;

            /* 字母键 → 扩展预设 */
            case 'A': set_target(25.0f); break;
            case 'B': set_target(28.0f); break;
            case 'C': set_target(30.0f); break;
            case 'D': set_target(12.0f); break;

            /* * = 停止风扇, # = 急停 */
            case '*': g_running = 0; g_fan_pct = 0; L298N_Motor_Stop();
                      OLED_ShowString(0, 6, (u8 *)"STOP             ", 16);
                      break;
            case '#': estop();
                      OLED_ShowString(0, 6, (u8 *)"EMERGENCY STOP   ", 16);
                      break;
            default: break;
            }

            /* 如果设置了目标, 显示 */
            if (g_running && key != '*' && key != '#')
            {
                sprintf(line, "Target: %2.0fcm   ", (double)g_target);
                OLED_ShowString(0, 6, (u8 *)line, 16);
            }
        }

        /* ── 2. 每 CONTROL_MS 读一次超声波 + PID ────── */
        if ((tick10ms % (CONTROL_MS / 10)) == 0)
        {
            sonic = HC_SR04_GetDistance();

            /*
             * 高度测量: 始终运行, 不受 g_running 影响
             * 这样还没设目标时也能看到球的位置
             */
            if (sonic > 0.0f)
            {
                g_sonic_raw = sonic;
                h_raw = sonic_to_height(sonic);

                if (h_raw >= 0.0f)
                {
                    g_has_reading = 1;

                    /* 一阶低通滤波 */
                    if (g_height_raw <= 0.0f)
                        g_height_raw = h_raw;
                    else
                        g_height_raw = HEIGHT_FILTER_A * h_raw
                                     + (1.0f - HEIGHT_FILTER_A) * g_height_raw;

                    g_height = g_height_raw;
                }
            }
            /* sonic==0 (超时) → 保持上次高度, 不更新 */

            /* PID 控制: 仅在运行态执行 */
            if (g_running && g_has_reading)
            {
                /*
                 * 起飞助推: 满功率直到球接近目标, 无时间限制
                 */
                if (g_launch_boost)
                {
                    /* 球已到位 + 确实离开筒底 → 结束助推, 切 PID */
                    if (g_height >= g_target - LAUNCH_CATCH_H &&
                        g_height >= LAUNCH_MIN_RISE)
                    {
                        g_launch_boost = 0;
                        PID_Init(&g_pid, PID_KP, PID_KI, PID_KD,
                                 PID_INTEGRAL_LIMIT, 0.0f, 100.0f);
                        PID_Setpoint(&g_pid, g_target);
                    }
                }

                if (g_launch_boost)
                {
                    fan = 100;
                }
                else
                {
                    /* 正常 PID 控制 */
                    pid_out = PID_Compute(&g_pid, g_height,
                                          (float)CONTROL_MS / 1000.0f);
                    fan = (u8)(pid_out + 0.5f);
                    if (fan > 100) fan = 100;
                }

                g_fan_pct = (s8)fan;
                if (fan > 0)
                    L298N_Motor_Set((s8)fan);
                else
                    L298N_Motor_Stop();
            }
            else if (!g_running)
            {
                g_fan_pct = 0;
                L298N_Motor_Stop();
            }
        }

        /* ── 3. 每 DISPLAY_MS 刷新 OLED ─────────────── */
        if ((tick10ms % (DISPLAY_MS / 10)) == 0)
        {
            /* 高度有效时显示高度, 有原始读数但超范围时显示原始距离 */
            if (g_has_reading)
                sprintf(line, "H:%5.1fcm     ", (double)g_height);
            else if (g_sonic_raw > 0.0f)
                sprintf(line, "S:%5.1fcm OOR ", (double)g_sonic_raw);
            else
                sprintf(line, "H: ---.-cm     ");
            OLED_ShowString(0, 2, (u8 *)line, 16);

            /* 目标 + 风扇 */
            if (g_running)
                sprintf(line, "T:%5.1f F:%3d%%", (double)g_target, (int)g_fan_pct);
            else if (g_fan_pct == 0)
                sprintf(line, "T: --.- STOPPED");
            else
                sprintf(line, "T:%5.1f F:%3d%%", (double)g_target, (int)g_fan_pct);
            OLED_ShowString(0, 4, (u8 *)line, 16);
        }

        tick10ms++;
        delay_ms(10);
    }
}
