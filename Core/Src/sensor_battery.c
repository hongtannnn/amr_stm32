#include "sensor_battery.h"
#include "adc.h"

volatile float battery_voltage = 0.0f;
volatile uint8_t battery_percent = 0;

void Battery_Init(void) {
    battery_voltage = 0.0f;
    battery_percent = 0;
}

void Battery_Read(void) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);

        // Cầu phân áp R16 (9k) và R17 (1k) chia áp 10 lần, ADC 12-bit, Vref = 3.3V
        battery_voltage = ((float)adc_val / 4095.0f) * 33.0f;

        // Tính phần trăm dung lượng pin 3S (10.5V: 0%, 12.6V: 100%)
        if (battery_voltage >= 12.6f) {
            battery_percent = 100;
        } else if (battery_voltage <= 10.5f) {
            battery_percent = 0;
        } else {
            battery_percent = (uint16_t)(((battery_voltage - 10.5f) / 2.1f) * 100.0f);
        }
    }
    HAL_ADC_Stop(&hadc1);
}
