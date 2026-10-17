#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float out_min;
    float out_max;
    float output;
    float error;
} PID_Controller_t;

void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd, float out_min, float out_max);
void PID_Reset(PID_Controller_t *pid);
float PID_Compute(PID_Controller_t *pid, float setpoint, float actual, float dt);

#endif /* PID_CONTROLLER_H */
