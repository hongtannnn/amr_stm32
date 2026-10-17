#include "pid_controller.h"

void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd, float out_min, float out_max) 
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->out_min = out_min;
    pid->out_max = out_max;
    pid->output = 0.0f;
    pid->error = 0.0f;
}

void PID_Reset(PID_Controller_t *pid) 
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output = 0.0f;
    pid->error = 0.0f;
}

float PID_Compute(PID_Controller_t *pid, float setpoint, float actual, float dt) 
{
    if (dt <= 0.0001f) {
        return 0.0f;
    }

    float error = setpoint - actual;
    pid->error = error;

    // Khâu tỷ lệ (P)
    float p_out = pid->kp * error;

    // Khâu tích phân (I)
    pid->integral += error * dt;

    // Chống bão hòa tích phân (Anti-windup)
    float i_out = pid->ki * pid->integral;
    if (i_out > pid->out_max) {
        i_out = pid->out_max;
        if (pid->ki > 0.0001f) {
            pid->integral = pid->out_max / pid->ki;
        }
    } else if (i_out < pid->out_min) {
        i_out = pid->out_min;
        if (pid->ki > 0.0001f) {
            pid->integral = pid->out_min / pid->ki;
        }
    }

    // Khâu vi phân (D)
    float derivative = (error - pid->prev_error) / dt;
    float d_out = pid->kd * derivative;
    pid->prev_error = error;

    // Tổng ngõ ra
    float output = p_out + i_out + d_out;

    // Giới hạn ngõ ra
    if (output > pid->out_max) {
        output = pid->out_max;
    } else if (output < pid->out_min) {
        output = pid->out_min;
    }

    pid->output = output;
    return output;
}
