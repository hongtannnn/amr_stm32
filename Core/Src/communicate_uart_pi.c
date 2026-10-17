#include "communicate_uart_pi.h"
#include "usart.h"
#include "differential_drive_kinematics.h"
#include <string.h>

typedef enum {
    RX_STATE_WAIT_HEADER1 = 0,
    RX_STATE_WAIT_HEADER2,
    RX_STATE_WAIT_MSG_ID,
    RX_STATE_WAIT_LEN,
    RX_STATE_READ_PAYLOAD,
    RX_STATE_WAIT_CHECKSUM
} CommPi_RxState_t;

static uint8_t s_rx_byte = 0;
static CommPi_RxState_t s_rx_state = RX_STATE_WAIT_HEADER1;
static uint8_t s_payload_idx = 0;
static uint8_t s_expected_len = 0;
static uint8_t s_payload_buf[16];
static uint32_t s_last_rx_tick = 0;

static uint8_t ComputeChecksum(const uint8_t *data, uint16_t len)
{
    uint8_t sum = 0;
    for (uint16_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

void CommPi_Init(void)
{
    s_rx_state = RX_STATE_WAIT_HEADER1;
    s_last_rx_tick = HAL_GetTick();
    HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
}

void CommPi_SendTelemetry(int32_t ticks_left, int32_t ticks_right, float gyro_z, float battery_volt)
{
    CommPi_TelemetryPacket_t pkt;
    pkt.header1 = COMM_PI_HEADER1;
    pkt.header2 = COMM_PI_HEADER2;
    pkt.msg_id = COMM_PI_MSG_TELEMETRY;
    pkt.payload_len = sizeof(int32_t) * 2 + sizeof(float) * 2;
    pkt.ticks_left = ticks_left;
    pkt.ticks_right = ticks_right;
    pkt.gyro_z = gyro_z;
    pkt.battery_volt = battery_volt;

    // Checksum tính từ msg_id đến hết payload
    uint8_t *p_data = &pkt.msg_id;
    uint16_t len_to_sum = 1 + 1 + pkt.payload_len;
    pkt.checksum = ComputeChecksum(p_data, len_to_sum);

    HAL_UART_Transmit(&huart2, (uint8_t *)&pkt, sizeof(pkt), 20);
}

void CommPi_CheckTimeout(uint32_t timeout_ms)
{
    // Ngắt an toàn: dừng robot nếu mất kết nối lệnh từ Pi quá thời gian quy định
    if ((HAL_GetTick() - s_last_rx_tick) > timeout_ms) {
        Kinematics_ComputeTarget(0.0f, 0.0f);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        switch (s_rx_state) {
            case RX_STATE_WAIT_HEADER1:
                if (s_rx_byte == COMM_PI_HEADER1) {
                    s_rx_state = RX_STATE_WAIT_HEADER2;
                }
                break;

            case RX_STATE_WAIT_HEADER2:
                if (s_rx_byte == COMM_PI_HEADER2) {
                    s_rx_state = RX_STATE_WAIT_MSG_ID;
                } else if (s_rx_byte != COMM_PI_HEADER1) {
                    s_rx_state = RX_STATE_WAIT_HEADER1;
                }
                break;

            case RX_STATE_WAIT_MSG_ID:
                if (s_rx_byte == COMM_PI_MSG_COMMAND) {
                    s_rx_state = RX_STATE_WAIT_LEN;
                } else {
                    s_rx_state = RX_STATE_WAIT_HEADER1;
                }
                break;

            case RX_STATE_WAIT_LEN:
                s_expected_len = s_rx_byte;
                if (s_expected_len == sizeof(float) * 2) {
                    s_payload_idx = 0;
                    s_rx_state = RX_STATE_READ_PAYLOAD;
                } else {
                    s_rx_state = RX_STATE_WAIT_HEADER1;
                }
                break;

            case RX_STATE_READ_PAYLOAD:
                s_payload_buf[s_payload_idx++] = s_rx_byte;
                if (s_payload_idx >= s_expected_len) {
                    s_rx_state = RX_STATE_WAIT_CHECKSUM;
                }
                break;

            case RX_STATE_WAIT_CHECKSUM: {
                uint8_t header_and_len[2] = {COMM_PI_MSG_COMMAND, s_expected_len};
                uint8_t calc_sum = ComputeChecksum(header_and_len, 2) + ComputeChecksum(s_payload_buf, s_expected_len);
                if (calc_sum == s_rx_byte) {
                    float cmd_v = 0.0f;
                    float cmd_omega = 0.0f;
                    memcpy(&cmd_v, &s_payload_buf[0], sizeof(float));
                    memcpy(&cmd_omega, &s_payload_buf[4], sizeof(float));

                    Kinematics_ComputeTarget(cmd_v, cmd_omega);
                    s_last_rx_tick = HAL_GetTick();
                }
                s_rx_state = RX_STATE_WAIT_HEADER1;
                break;
            }

            default:
                s_rx_state = RX_STATE_WAIT_HEADER1;
                break;
        }

        HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        // Xóa cờ lỗi Overrun nếu có và kích hoạt lại ngắt nhận
        __HAL_UART_CLEAR_OREFLAG(huart);
        s_rx_state = RX_STATE_WAIT_HEADER1;
        HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
    }
}
