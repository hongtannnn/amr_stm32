#ifndef SENSOR_IMU_H
#define SENSOR_IMU_H

#include <stdint.h>

extern volatile float gz_dps;

// Các hàm chính
void IMU_Init(void);
void IMU_Read_Data(void);

#endif /* SENSOR_IMU_H */
