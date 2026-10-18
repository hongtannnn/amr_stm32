#ifndef JOYSTICK_DABBLE_H
#define JOYSTICK_DABBLE_H

#include <stdint.h>
#include "main.h"

/*
 * Dabble GamePad Joystick - giao thuc nhan qua HC-05/USART3:
 *   Frame: 0xFF | 0x01 | 0x02 | 0x01 | 0x01 | DATA | 0x00
 *   DATA[7:3] = angle / 15  (0-360 deg, buoc 15)
 *   DATA[2:0] = radius       (0-7)
 *   x = radius * cos(angle)
 *   y = radius * sin(angle)
 *
 * Anh xa sang lenh robot:
 *   y > 0  ->  cmd_v =  +0.3 m/s
 *   y < 0  ->  cmd_v =  -0.3 m/s
 *   x > 0  ->  cmd_omega = +1.0 rad/s  (cua phai)
 *   x < 0  ->  cmd_omega = -1.0 rad/s  (cua trai)
 */

void Joystick_Dabble_Init(void);
void Joystick_Dabble_ProcessByte(uint8_t byte);

#endif /* JOYSTICK_DABBLE_H */
