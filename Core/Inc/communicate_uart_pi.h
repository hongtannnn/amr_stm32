#ifndef COMMUNICATE_UART_PI_H
#define COMMUNICATE_UART_PI_H

#include <stdint.h>
#include "main.h"

#define COMM_PI_HEADER1       0xAA
#define COMM_PI_HEADER2       0x55
#define COMM_PI_MSG_TELEMETRY 0x01
#define COMM_PI_MSG_COMMAND   0x02

#pragma pack(push, 1)
typedef struct {
    uint8_t  header1;
    uint8_t  header2;
    uint8_t  msg_id;
    uint8_t  payload_len;
    int32_t  ticks_left;
    int32_t  ticks_right;
    float    gyro_z;       // dps (hoặc rad/s)
    float    battery_volt; // V
    uint8_t  checksum;
} CommunicatePi_TelemetryPacket_t;

typedef struct {
    uint8_t header1;
    uint8_t header2;
    uint8_t msg_id;
    uint8_t payload_len;
    float   cmd_v;        // m/s
    float   cmd_omega;    // rad/s
    uint8_t checksum;
} CommunicatePi_CommandPacket_t;
#pragma pack(pop)

void CommunicatePi_Init(void);
void CommunicatePi_SendTelemetry(int32_t ticks_left, int32_t ticks_right, float gyro_z, float battery_volt);
void CommunicatePi_CheckTimeout(uint32_t timeout_ms);

#endif /* COMMUNICATE_UART_PI_H */
