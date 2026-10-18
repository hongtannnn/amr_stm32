#ifndef MOTION_CONTROL_H
#define MOTION_CONTROL_H

#include <stdint.h>
#include "main.h"

/* Cac bien quan sat trang thai (Live Watch / Debug) */
extern volatile float yaw_correction;
extern volatile int16_t pwm_left;
extern volatile int16_t pwm_right;

/* Khoi tao phan cung va thong so dieu khien chuyen dong */
void Motion_Control_Init(void);

/* Vong lap tinh toan va dieu khien dong co theo chu ky dt (giay) */
void Motion_Control_Update(float dt);

#endif /* MOTION_CONTROL_H */
