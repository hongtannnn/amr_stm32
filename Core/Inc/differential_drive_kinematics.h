#ifndef DIFFERENTIAL_DRIVE_KINEMATICS_H
#define DIFFERENTIAL_DRIVE_KINEMATICS_H

#include <stdint.h>

#define ROBOT_WHEEL_BASE      0.193f  // Khoảng cách hai bánh xe L = 193mm (m)
#define ROBOT_WHEEL_RADIUS    0.033f  // Bán kính bánh xe R = 33mm (m)
#define ROBOT_MAX_LINEAR_VEL     0.515f  // Vận tốc dài tối đa mặc định V_max = 515mm/s (m/s)
#define ROBOT_TICKS_PER_REV      1980.0f // Số xung encoder trên một vòng quay bánh xe
#define ROBOT_MAX_ACCEL_LINEAR   1.0f    // Gia tốc dài tối đa (m/s^2)
#define ROBOT_MAX_ACCEL_ANGULAR  3.0f    // Gia tốc góc tối đa (rad/s^2)

typedef struct {
    float wheel_base;          // L (m)
    float wheel_radius;        // R (m)
    float max_linear_vel;      // V_max (m/s)

    // Lệnh vận tốc đích (Command from ROS / User)
    float cmd_v;               // Lệnh vận tốc dài đích (m/s)
    float cmd_omega;           // Lệnh vận tốc góc đích (rad/s)

    // Vận tốc đặt sau bộ lọc gia tốc Ramp (Target applied to PID)
    float target_v;            // Vận tốc dài đặt sau ramp (m/s)
    float target_omega;        // Vận tốc góc đặt sau ramp (rad/s)
    float target_v_left;       // Vận tốc dài mong muốn bánh trái (m/s)
    float target_v_right;      // Vận tốc dài mong muốn bánh phải (m/s)
    float target_omega_left;   // Vận tốc góc mong muốn bánh trái (rad/s)
    float target_omega_right;  // Vận tốc góc mong muốn bánh phải (rad/s)
    int16_t target_pwm_left;   // Xung PWM bánh trái [-1000, 1000]
    int16_t target_pwm_right;  // Xung PWM bánh phải [-1000, 1000]

    // Vận tốc thực tế (Actual measured from encoders)
    float actual_v_left;       // Vận tốc dài thực bánh trái (m/s)
    float actual_v_right;      // Vận tốc dài thực bánh phải (m/s)
    float actual_omega_left;   // Vận tốc góc thực bánh trái (rad/s)
    float actual_omega_right;  // Vận tốc góc thực bánh phải (rad/s)
    float actual_v;            // Vận tốc dài thực của robot (m/s)
    float actual_omega;        // Vận tốc góc thực của robot (rad/s)
} DiffDrive_Kinematics_t;

extern DiffDrive_Kinematics_t g_kinematics;

void Kinematics_Init(float wheel_base, float wheel_radius, float max_linear_vel);
void Kinematics_ComputeTarget(float v, float omega);
void Kinematics_UpdateRamp(float dt_seconds);
void Kinematics_ComputeActual(int32_t total_left_ticks, int32_t total_right_ticks, float dt_seconds);
int16_t Kinematics_VelocityToPWM(float v_wheel);

#endif /* DIFFERENTIAL_DRIVE_KINEMATICS_H */
