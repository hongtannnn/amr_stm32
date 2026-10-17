#include "differential_drive_kinematics.h"

#define PI_CONST 3.14159265f

DiffDrive_Kinematics_t g_kinematics;

static int32_t s_prev_left_ticks = 0;
static int32_t s_prev_right_ticks = 0;

void Kinematics_Init(float wheel_base, float wheel_radius, float max_linear_vel) 
{
    g_kinematics.wheel_base = wheel_base;
    g_kinematics.wheel_radius = wheel_radius;
    g_kinematics.max_linear_vel = max_linear_vel;

    g_kinematics.cmd_v = 0.0f;
    g_kinematics.cmd_omega = 0.0f;
    g_kinematics.target_v = 0.0f;
    g_kinematics.target_omega = 0.0f;
    g_kinematics.target_v_left = 0.0f;
    g_kinematics.target_v_right = 0.0f;
    g_kinematics.target_omega_left = 0.0f;
    g_kinematics.target_omega_right = 0.0f;
    g_kinematics.target_pwm_left = 0;
    g_kinematics.target_pwm_right = 0;

    g_kinematics.actual_v_left = 0.0f;
    g_kinematics.actual_v_right = 0.0f;
    g_kinematics.actual_omega_left = 0.0f;
    g_kinematics.actual_omega_right = 0.0f;
    g_kinematics.actual_v = 0.0f;
    g_kinematics.actual_omega = 0.0f;

    s_prev_left_ticks = 0;
    s_prev_right_ticks = 0;
}

int16_t Kinematics_VelocityToPWM(float v_wheel) 
{
    if (g_kinematics.max_linear_vel <= 0.0001f) {
        return 0;
    }
    float pwm_float = (v_wheel / g_kinematics.max_linear_vel) * 1000.0f;
    if (pwm_float > 1000.0f) {
        pwm_float = 1000.0f;
    } else if (pwm_float < -1000.0f) {
        pwm_float = -1000.0f;
    }
    return (int16_t)pwm_float;
}

void Kinematics_ComputeTarget(float v, float omega) 
{
    g_kinematics.cmd_v = v;
    g_kinematics.cmd_omega = omega;
}

void Kinematics_UpdateRamp(float dt_seconds)
{
    if (dt_seconds <= 0.0f) {
        return;
    }

    // Giới hạn mức thay đổi vận tốc dài mỗi bước chu kỳ dt
    float max_delta_v = ROBOT_MAX_ACCEL_LINEAR * dt_seconds;
    float diff_v = g_kinematics.cmd_v - g_kinematics.target_v;
    if (diff_v > max_delta_v) {
        g_kinematics.target_v += max_delta_v;
    } else if (diff_v < -max_delta_v) {
        g_kinematics.target_v -= max_delta_v;
    } else {
        g_kinematics.target_v = g_kinematics.cmd_v;
    }

    // Giới hạn mức thay đổi vận tốc góc mỗi bước chu kỳ dt
    float max_delta_omega = ROBOT_MAX_ACCEL_ANGULAR * dt_seconds;
    float diff_omega = g_kinematics.cmd_omega - g_kinematics.target_omega;
    if (diff_omega > max_delta_omega) {
        g_kinematics.target_omega += max_delta_omega;
    } else if (diff_omega < -max_delta_omega) {
        g_kinematics.target_omega -= max_delta_omega;
    } else {
        g_kinematics.target_omega = g_kinematics.cmd_omega;
    }

    // Tính toán vận tốc đặt cho từng bánh dựa trên target_v và target_omega sau ramp
    g_kinematics.target_v_left = g_kinematics.target_v - (g_kinematics.target_omega * g_kinematics.wheel_base / 2.0f);
    g_kinematics.target_v_right = g_kinematics.target_v + (g_kinematics.target_omega * g_kinematics.wheel_base / 2.0f);

    // Vận tốc góc mong muốn của từng bánh xe (rad/s)
    if (g_kinematics.wheel_radius > 0.0001f) {
        g_kinematics.target_omega_left = g_kinematics.target_v_left / g_kinematics.wheel_radius;
        g_kinematics.target_omega_right = g_kinematics.target_v_right / g_kinematics.wheel_radius;
    }

    // Quy đổi ra xung PWM điều khiển động cơ [-1000, 1000]
    g_kinematics.target_pwm_left = Kinematics_VelocityToPWM(g_kinematics.target_v_left);
    g_kinematics.target_pwm_right = Kinematics_VelocityToPWM(g_kinematics.target_v_right);
}

void Kinematics_ComputeActual(int32_t total_left_ticks, int32_t total_right_ticks, float dt_seconds) 
{
    if (dt_seconds <= 0.0001f || g_kinematics.wheel_radius <= 0.0001f || g_kinematics.wheel_base <= 0.0001f) {
        return;
    }

    int32_t delta_ticks_left = total_left_ticks - s_prev_left_ticks;
    int32_t delta_ticks_right = total_right_ticks - s_prev_right_ticks;

    s_prev_left_ticks = total_left_ticks;
    s_prev_right_ticks = total_right_ticks;

    // Chu vi bánh xe C = 2 * PI * R (m)
    float wheel_circ = 2.0f * PI_CONST * g_kinematics.wheel_radius;

    // Quãng đường từng bánh di chuyển trong khoảng dt (m)
    float dist_left = ((float)delta_ticks_left / ROBOT_TICKS_PER_REV) * wheel_circ;
    float dist_right = ((float)delta_ticks_right / ROBOT_TICKS_PER_REV) * wheel_circ;

    // Vận tốc dài thực tế từng bánh (m/s)
    g_kinematics.actual_v_left = dist_left / dt_seconds;
    g_kinematics.actual_v_right = dist_right / dt_seconds;

    // Vận tốc góc thực tế từng bánh (rad/s)
    g_kinematics.actual_omega_left = g_kinematics.actual_v_left / g_kinematics.wheel_radius;
    g_kinematics.actual_omega_right = g_kinematics.actual_v_right / g_kinematics.wheel_radius;

    // Vận tốc dài và vận tốc góc thực tế của robot (m/s và rad/s)
    g_kinematics.actual_v = (g_kinematics.actual_v_left + g_kinematics.actual_v_right) / 2.0f;
    g_kinematics.actual_omega = (g_kinematics.actual_v_right - g_kinematics.actual_v_left) / g_kinematics.wheel_base;
}
