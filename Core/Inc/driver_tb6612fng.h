#ifndef DRIVER_TB6612FNG_H
#define DRIVER_TB6612FNG_H

#include <stdint.h>

void Motor_Init(void);
void Motor_Run(int16_t left_wheel, int16_t right_wheel);
void Motor_Stop(void);

#endif /* DRIVER_TB6612FNG_H */
