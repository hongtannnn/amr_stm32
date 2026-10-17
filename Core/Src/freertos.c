/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sensor_battery.h"
#include "driver_tb6612fng.h"
#include "sensor_encoder.h"
#include "differential_drive_kinematics.h"
#include "pid_controller.h"
#include "sensor_imu.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
PID_Controller_t pid_wheel_left;
PID_Controller_t pid_wheel_right;
PID_Controller_t pid_yaw;
volatile float yaw_correction = 0.0f;
volatile int16_t pwm_left = 0;
volatile int16_t pwm_right = 0;
/* USER CODE END Variables */
osThreadId Task1Handle;
osThreadId Task2Handle;
osThreadId Task3Handle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartTask1(void const * argument);
void StartTask2(void const * argument);
void StartTask3(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Task1 */
  osThreadDef(Task1, StartTask1, osPriorityHigh, 0, 128);
  Task1Handle = osThreadCreate(osThread(Task1), NULL);

  /* definition and creation of Task2 */
  osThreadDef(Task2, StartTask2, osPriorityNormal, 0, 128);
  Task2Handle = osThreadCreate(osThread(Task2), NULL);

  /* definition and creation of Task3 */
  osThreadDef(Task3, StartTask3, osPriorityLow, 0, 128);
  Task3Handle = osThreadCreate(osThread(Task3), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartTask1 */
/**
  * @brief  Function implementing the Task1 thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTask1 */
void StartTask1(void const * argument)
{
  /* USER CODE BEGIN StartTask1 */
  Motor_Init();
  Encoder_Init();
  Kinematics_Init(ROBOT_WHEEL_BASE, ROBOT_WHEEL_RADIUS, ROBOT_MAX_LINEAR_VEL);

  // Khởi tạo bộ điều khiển PID cho 2 bánh (out_min = -1000, out_max = 1000)
  PID_Init(&pid_wheel_left, 2000.0f, 2000.0f, 0.0f, -1000.0f, 1000.0f);
  PID_Init(&pid_wheel_right, 2000.0f, 2000.0f, 0.0f, -1000.0f, 1000.0f);

  // Khởi tạo PID Yaw giữ thẳng hướng (hệ số tạm để 0 để bạn tự chỉnh)
  PID_Init(&pid_yaw, 0.03f, 0.08f, 0.0f, -1.0f, 1.0f);

  // Gọi hàm này khi muốn đặt vận tốc test (Ví dụ: v = 0.2 m/s, omega = 0.0 rad/s)
  Kinematics_ComputeTarget(0.3f, 0.0f);

  const float dt = 0.005f; // Chu kỳ lấy mẫu 5ms

  /* Infinite loop */
  for(;;)
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
      yaw_correction = PID_Compute(&pid_yaw, 0.0f, gz_dps, dt);
      float delta_v = yaw_correction * (g_kinematics.wheel_base / 2.0f);
      target_v_left_final -= delta_v;
      target_v_right_final += delta_v;
    } else {
      // Khi đi cua (cmd_omega != 0) hoặc đứng yên: không can thiệp
      PID_Reset(&pid_yaw);
      yaw_correction = 0.0f;
    }

    // 4. Tính toán ngõ ra PID từ sai số vận tốc đặt và thực tế
    pwm_left = (int16_t)PID_Compute(&pid_wheel_left, target_v_left_final, g_kinematics.actual_v_left, dt);
    pwm_right = (int16_t)PID_Compute(&pid_wheel_right, target_v_right_final, g_kinematics.actual_v_right, dt);

    // 5. Dừng hoàn toàn khi không có lệnh vận tốc và xe đã giảm tốc về 0
    if (g_kinematics.cmd_v == 0.0f && g_kinematics.cmd_omega == 0.0f &&
        g_kinematics.target_v == 0.0f && g_kinematics.target_omega == 0.0f) {
      pwm_left = 0;
      pwm_right = 0;
      PID_Reset(&pid_wheel_left);
      PID_Reset(&pid_wheel_right);
      PID_Reset(&pid_yaw);
    }

    // 6. Cấp xung điều khiển động cơ
    Motor_Run(pwm_left, pwm_right);

    osDelay(5);
  }
  /* USER CODE END StartTask1 */
}

/* USER CODE BEGIN Header_StartTask2 */
/**
* @brief Function implementing the Task2 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask2 */
void StartTask2(void const * argument)
{
  /* USER CODE BEGIN StartTask2 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTask2 */
}

/* USER CODE BEGIN Header_StartTask3 */
/**
* @brief Function implementing the Task3 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask3 */
void StartTask3(void const * argument)
{
  /* USER CODE BEGIN StartTask3 */
  Battery_Init();
  /* Infinite loop */
  for(;;)
  {
    Battery_Read();
    osDelay(20000);
  }
  /* USER CODE END StartTask3 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
