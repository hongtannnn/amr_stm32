#include "driver_tb6612fng.h"
#include "tim.h"
#include "gpio.h"

#define MOTOR_MAX_PWM 1000

void Motor_Init(void) 
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    Motor_Stop();
}

void Motor_Run(int16_t left_wheel, int16_t right_wheel)
{
    // 1.Giới hạn dải đầu vào [-1000, 1000]
    if (right_wheel > MOTOR_MAX_PWM) right_wheel = MOTOR_MAX_PWM;
    else if (right_wheel < -MOTOR_MAX_PWM) right_wheel = -MOTOR_MAX_PWM;

    if (left_wheel > MOTOR_MAX_PWM) left_wheel = MOTOR_MAX_PWM;
    else if (left_wheel < -MOTOR_MAX_PWM) left_wheel = -MOTOR_MAX_PWM;

    // 2.Điều khiển bánh trái (Motor A: PB10 - TIM2_CH3, AIN1 - PC0, AIN2 - PC1)
    if (right_wheel > 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)right_wheel);
    } else if (right_wheel < 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)(-right_wheel));
    } else {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
    }

    // 3.Điều khiển bánh phải (Motor B: PB11 - TIM2_CH4, BIN1 - PC2, BIN2 - PC3)
    if (left_wheel > 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, (uint32_t)left_wheel);
    } else if (left_wheel < 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, (uint32_t)(-left_wheel));
    } else {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 0);
    }
}

void Motor_Stop(void) 
{
    Motor_Run(0, 0);
}
