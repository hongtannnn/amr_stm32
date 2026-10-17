#include "sensor_encoder.h"
#include "tim.h"

volatile int32_t left_encoder = 0;
volatile int32_t right_encoder = 0;

static uint16_t last_cnt_left = 0;
static uint16_t last_cnt_right = 0;

void Encoder_Init(void) 
{
    HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL); // Bánh trái: TIM5 (PA0, PA1)
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); // Bánh phải: TIM3 (PA6, PA7)
    Encoder_Reset();
}

void Encoder_Update(void) 
{
    uint16_t cur_cnt_right = (uint16_t)__HAL_TIM_GET_COUNTER(&htim5);
    uint16_t cur_cnt_left = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);

    // Ép kiểu int16_t tự động xử lý hiện tượng tràn/quay vòng số đếm 16-bit
    int16_t delta_left = (int16_t)(cur_cnt_left - last_cnt_left);
    int16_t delta_right = (int16_t)(cur_cnt_right - last_cnt_right);

    last_cnt_left = cur_cnt_left;
    last_cnt_right = cur_cnt_right;

    left_encoder -= delta_left;
    right_encoder += delta_right;
}

void Encoder_Reset(void) 
{
    __HAL_TIM_SET_COUNTER(&htim5, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    last_cnt_left = 0;
    last_cnt_right = 0;
    left_encoder = 0;
    right_encoder = 0;
}
