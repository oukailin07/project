/**
 * pid.c - 增量式 PID 控制器（位置式输出 + 积分分离 + 输出限幅）
 *
 * u(t) = Kp*e(t) + Ki*∫e(t)dt + Kd*de(t)/dt
 */

#include "pid.h"

/**
 * @brief 初始化 PID 控制器
 * @param integral_limit  积分项限幅（防积分饱和）
 * @param out_min / out_max  输出限幅
 */
void PID_Init(PID_t *pid, float Kp, float Ki, float Kd,
              float integral_limit, float out_min, float out_max)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->setpoint      = 0.0f;
    pid->integral      = 0.0f;
    pid->prev_error    = 0.0f;
    pid->integral_limit = integral_limit;
    pid->output_min    = out_min;
    pid->output_max    = out_max;
}

/**
 * @brief 更新目标值，同时清零积分项避免突变冲击
 */
void PID_Setpoint(PID_t *pid, float sp)
{
    pid->setpoint   = sp;
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
}

/**
 * @brief 计算 PID 输出
 * @param measured  当前测量值
 * @param dt        距上次计算的时间间隔（秒）
 * @return          控制量输出 [output_min, output_max]
 *
 * 采用位置式 PID + 变速积分（误差大时削弱积分，误差小时加强积分）
 */
float PID_Compute(PID_t *pid, float measured, float dt)
{
    float error, derivative, output;

    if (dt <= 0.0f) dt = 0.02f;  /* 默认 20ms */

    error = pid->setpoint - measured;

    /* ── 变速积分：误差大时减弱积分作用 ──────────────── */
    if (error < 20.0f && error > -20.0f) {
        pid->integral += error * dt;

        /* 积分限幅 */
        if (pid->integral >  pid->integral_limit) pid->integral =  pid->integral_limit;
        if (pid->integral < -pid->integral_limit) pid->integral = -pid->integral_limit;
    }

    /* ── 微分项（测量值微分，避免设定值突变冲击）───── */
    derivative = -(measured - pid->prev_error) / dt;  /* d(-measure)/dt = -de/dt ≈ -d(measured)/dt */

    /* ── 位置式输出 ─────────────────────────────────── */
    output = pid->Kp * error
           + pid->Ki * pid->integral
           + pid->Kd * derivative;

    /* ── 输出限幅 ───────────────────────────────────── */
    if (output > pid->output_max) output = pid->output_max;
    if (output < pid->output_min) output = pid->output_min;

    pid->prev_error = measured;

    return output;
}
