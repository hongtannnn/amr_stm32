#include "motion_control.h"
#include "driver_tb6612fng.h"
#include "sensor_encoder.h"
#include "differential_drive_kinematics.h"
#include "pid_controller.h"
#include "sensor_imu.h"
#include "joystick_dabble.h"

/* Bộ điều khiển PID nội bộ của module */
static PID_Controller_t s_pid_wheel_left;
static PID_Controller_t s_pid_wheel_right;
static PID_Controller_t s_pid_yaw;

/* Biến quan sát toàn cục */
volatile float yaw_correction = 0.0f;
volatile int16_t pwm_left = 0;
volatile int16_t pwm_right = 0;

void Motion_Control_Init(void)
{
    Motor_Init();
    Encoder_Init();
    Kinematics_Init(ROBOT_WHEEL_BASE, ROBOT_WHEEL_RADIUS, ROBOT_MAX_LINEAR_VEL);

    // Khởi động nhận joystick Dabble qua USART3/HC-05
    Joystick_Dabble_Init();

    // Khởi tạo bộ điều khiển PID cho 2 bánh (out_min = -1000, out_max = 1000)
    PID_Init(&s_pid_wheel_left, 2000.0f, 2000.0f, 0.0f, -1000.0f, 1000.0f);
    PID_Init(&s_pid_wheel_right, 2000.0f, 2000.0f, 0.0f, -1000.0f, 1000.0f);

    // Khởi tạo PID Yaw giữ thẳng hướng (hệ số tạm để 0 để bạn tự chỉnh)
    PID_Init(&s_pid_yaw, 0.03f, 0.08f, 0.0f, -1.0f, 1.0f);
}

void Motion_Control_Update(float dt)
{
    // 1. Cập nhật số đếm encoder và đọc cảm biến IMU (gz_dps)
    Encoder_Update();
    IMU_Read_Data();

    // 2. Cập nhật bộ lọc gia tốc Ramp và tính vận tốc thực tế từng bánh
    Kinematics_UpdateRamp(dt);
    Kinematics_ComputeActual(left_encoder, right_encoder, dt);

    // 3. Tính toán bù lệch góc bằng IMU Yaw PID khi đi thẳng
    float target_v_left_final = g_kinematics.target_v_left;
    float target_v_right_final = g_kinematics.target_v_right;

    if (g_kinematics.cmd_omega == 0.0f && g_kinematics.cmd_v != 0.0f) {
        // Khi đi thẳng: bật PID Yaw giữ gz_dps = 0
        yaw_correction = PID_Compute(&s_pid_yaw, 0.0f, gz_dps, dt);
        float delta_v = yaw_correction * (g_kinematics.wheel_base / 2.0f);
        target_v_left_final -= delta_v;
        target_v_right_final += delta_v;
    } else {
        // Khi đi cua (cmd_omega != 0) hoặc đứng yên: không can thiệp
        PID_Reset(&s_pid_yaw);
        yaw_correction = 0.0f;
    }

    // 4. Tính toán ngõ ra PID từ sai số vận tốc đặt và thực tế
    pwm_left = (int16_t)PID_Compute(&s_pid_wheel_left, target_v_left_final, g_kinematics.actual_v_left, dt);
    pwm_right = (int16_t)PID_Compute(&s_pid_wheel_right, target_v_right_final, g_kinematics.actual_v_right, dt);

    // 5. Dừng hoàn toàn khi không có lệnh vận tốc và xe đã giảm tốc về 0
    if (g_kinematics.cmd_v == 0.0f && g_kinematics.cmd_omega == 0.0f &&
        g_kinematics.target_v == 0.0f && g_kinematics.target_omega == 0.0f) {
        pwm_left = 0;
        pwm_right = 0;
        PID_Reset(&s_pid_wheel_left);
        PID_Reset(&s_pid_wheel_right);
        PID_Reset(&s_pid_yaw);
    }

    // 6. Cấp xung điều khiển động cơ
    Motor_Run(pwm_left, pwm_right);
}
