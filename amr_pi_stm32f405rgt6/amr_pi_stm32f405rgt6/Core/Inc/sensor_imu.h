#ifndef SENSOR_IMU_H
#define SENSOR_IMU_H

#include <stdint.h>

// Biến lưu ID của chip (Chính là biến đang bị thiếu gây ra lỗi)
extern uint8_t bmi323_chip_id;

// Dữ liệu thô (Raw) chưa qua xử lý
// Thêm chữ volatile vào trước các biến
extern volatile int16_t acc_raw[3];
extern volatile int16_t gyr_raw[3];

extern volatile float ax_g, ay_g, az_g;
extern volatile float gx_dps, gy_dps, gz_dps;

// Các hàm chính
void IMU_Init(void);
void IMU_Read_Data(void);

#endif /* SENSOR_IMU_H */
