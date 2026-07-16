/**
 * pid.h - 增量式 PID 控制器
 */

#ifndef __PID_H
#define __PID_H

#include "sys.h"

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float setpoint;
    float integral;
    float prev_error;
    float integral_limit;
    float output_min;
    float output_max;
} PID_t;

void  PID_Init(PID_t *pid, float Kp, float Ki, float Kd,
               float integral_limit, float out_min, float out_max);
void  PID_Setpoint(PID_t *pid, float sp);
float PID_Compute(PID_t *pid, float measured, float dt);

#endif /* __PID_H */
