#ifndef SENSOR_BATTERY_H
#define SENSOR_BATTERY_H

#include <stdint.h>

extern volatile float battery_voltage;
extern volatile uint8_t battery_percent;

void Battery_Init(void);
void Battery_Read(void);

#endif /* SENSOR_BATTERY_H */
