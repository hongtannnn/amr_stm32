#ifndef SENSOR_ENCODER_H
#define SENSOR_ENCODER_H

#include <stdint.h>

extern volatile int32_t left_encoder;
extern volatile int32_t right_encoder;

void Encoder_Init(void);
void Encoder_Update(void);
void Encoder_Reset(void);

#endif /* SENSOR_ENCODER_H */
