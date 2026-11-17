#include "motion_control.h"
#include "driver_tb6612fng.h"
#include "sensor_encoder.h"
#include "differential_drive_kinematics.h"
#include "pid_controller.h"
#include "sensor_imu.h"

/* Bộ điều khiển PID nội bộ của module */
static PID_Controller_t s_pid_wheel_left;
static PID_Controller_t s_pid_wheel_right;
static PID_Controller_t s_pid_yaw;
static PID_Controller_t s_pid_sync;

/* Biến quan sát toàn cục */
volatile float yaw_correction = 0.0f;
volatile float sync_correction = 0.0f;
volatile int32_t sync_tick_error = 0;
volatile int16_t pwm_left = 0;
volatile int16_t pwm_right = 0;

void Motion_Control_Init(void)
{
    Motor_Init();
    Encoder_Init();
    Kinematics_Init(ROBOT_WHEEL_BASE, ROBOT_WHEEL_RADIUS, ROBOT_MAX_LINEAR_VEL);

    // Khởi tạo bộ điều khiển PID cho 2 bánh (Feedforward + PID bù sai số)
    PID_Init(&s_pid_wheel_left, 1500.0f, 2000.0f, 0.0f, -1000.0f, 1000.0f);
    PID_Init(&s_pid_wheel_right, 1500.0f, 2000.0f, 0.0f, -1000.0f, 1000.0f);

    // Khởi tạo PID Yaw giữ thẳng hướng (chỉ dùng Kp, Ki = 0 để chống trôi tích phân Gyro)
    PID_Init(&s_pid_yaw, 0.1f, 0.0f, 0.0f, -0.2f, 0.2f);

    // Khởi tạo PID đồng bộ khóa cứng số xung Encoder giữa 2 bánh khi đi thẳng
    // Kp bù độ lệch xung, Kd chống lắc, giới hạn can thiệp [-0.08, 0.08] m/s
    PID_Init(&s_pid_sync, 0.004f, 0.0f, 0.00000f, -0.08f, 0.08f);
}

void Motion_Control_Update(float dt)
{
    // 1. Cập nhật số đếm encoder và đọc cảm biến IMU (gz_dps)
    Encoder_Update();
    IMU_Read_Data();

    // 2. Cập nhật bộ lọc gia tốc Ramp và tính vận tốc thực tế từng bánh
    Kinematics_UpdateRamp(dt);
    Kinematics_ComputeActual(left_encoder, right_encoder, dt);

    // 3. Tính toán bù lệch góc bằng IMU và đồng bộ xung encoder 2 bánh khi đi thẳng
    float target_v_left_final = g_kinematics.target_v_left;
    float target_v_right_final = g_kinematics.target_v_right;

    static uint8_t s_is_going_straight = 0;
    static int32_t s_start_encoder_left = 0;
    static int32_t s_start_encoder_right = 0;

    if (g_kinematics.cmd_omega == 0.0f && g_kinematics.cmd_v != 0.0f && 
        (g_kinematics.target_v > 0.05f || g_kinematics.target_v < -0.05f)) {
        
        // Bắt đầu vào trạng thái đi thẳng: chốt mốc encoder làm chuẩn
        if (!s_is_going_straight) {
            s_is_going_straight = 1;
            s_start_encoder_left = left_encoder;
            s_start_encoder_right = right_encoder;
            PID_Reset(&s_pid_sync);
        }

        // Tính chênh lệch xung thực tế kể từ mốc đi thẳng
        int32_t left_ticks = left_encoder - s_start_encoder_left;
        int32_t right_ticks = right_encoder - s_start_encoder_right;
        sync_tick_error = left_ticks - right_ticks;

        // PID đồng bộ khóa cứng số xung: nếu bánh nào đi nhiều xung hơn sẽ bị hãm lại
        sync_correction = PID_Compute(&s_pid_sync, 0.0f, (float)sync_tick_error, dt);
        target_v_left_final += sync_correction;
        target_v_right_final -= sync_correction;

        // Bù hướng bằng Gyro Yaw (chỉ P, không tích dồn trôi góc)
        yaw_correction = PID_Compute(&s_pid_yaw, 0.0f, gz_dps, dt);
        float delta_v = yaw_correction * (g_kinematics.wheel_base / 2.0f);
        target_v_left_final -= delta_v;
        target_v_right_final += delta_v;
    } else {
        s_is_going_straight = 0;
        PID_Reset(&s_pid_sync);
        PID_Reset(&s_pid_yaw);
        sync_correction = 0.0f;
        sync_tick_error = 0;
        yaw_correction = 0.0f;
    }

    // 4. Tính toán ngõ ra: Feedforward (xung nền động học) + PID (bù sai số tinh)
    int16_t pid_out_left = (int16_t)PID_Compute(&s_pid_wheel_left, target_v_left_final, g_kinematics.actual_v_left, dt);
    int16_t pid_out_right = (int16_t)PID_Compute(&s_pid_wheel_right, target_v_right_final, g_kinematics.actual_v_right, dt);

    pwm_left = g_kinematics.target_pwm_left + pid_out_left;
    pwm_right = g_kinematics.target_pwm_right + pid_out_right;

    // Giới hạn dải đầu ra [-1000, 1000]
    if (pwm_left > 1000) pwm_left = 1000;
    else if (pwm_left < -1000) pwm_left = -1000;
    if (pwm_right > 1000) pwm_right = 1000;
    else if (pwm_right < -1000) pwm_right = -1000;

    // 5. Dừng hoàn toàn khi không có lệnh vận tốc và xe đã giảm tốc về 0
    if (g_kinematics.cmd_v == 0.0f && g_kinematics.cmd_omega == 0.0f &&
        g_kinematics.target_v == 0.0f && g_kinematics.target_omega == 0.0f) {
        pwm_left = 0;
        pwm_right = 0;
        PID_Reset(&s_pid_wheel_left);
        PID_Reset(&s_pid_wheel_right);
        PID_Reset(&s_pid_yaw);
        PID_Reset(&s_pid_sync);
        sync_correction = 0.0f;
        sync_tick_error = 0;
        s_is_going_straight = 0;
    }

    // 6. Cấp xung điều khiển động cơ
    Motor_Run(pwm_left, pwm_right);
}

void Motion_Control_TestOpenLoop(float dt)
{
    // 1. Cập nhật số đếm encoder và đọc cảm biến IMU để vẫn theo dõi vận tốc thực tế
    Encoder_Update();
    IMU_Read_Data();

    // 2. Cập nhật dốc gia tốc và tính toán vận tốc thực tế từ encoder
    Kinematics_UpdateRamp(dt);
    Kinematics_ComputeActual(left_encoder, right_encoder, dt);

    // 3. Vòng hở thuần túy: Xuất trực tiếp PWM tính từ công thức động học, bỏ qua PID
    pwm_left = g_kinematics.target_pwm_left;
    pwm_right = g_kinematics.target_pwm_right;

    // 4. Dừng hoàn toàn khi không có lệnh vận tốc và xe đã giảm tốc về 0
    if (g_kinematics.cmd_v == 0.0f && g_kinematics.cmd_omega == 0.0f &&
        g_kinematics.target_v == 0.0f && g_kinematics.target_omega == 0.0f) {
        pwm_left = 0;
        pwm_right = 0;
    }

    // 5. Cấp xung điều khiển động cơ
    Motor_Run(pwm_left, pwm_right);
}
