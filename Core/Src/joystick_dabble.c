#include "joystick_dabble.h"
#include "usart.h"
#include "differential_drive_kinematics.h"
#include <math.h>

/* -----------------------------------------------------------
 * Giao thuc Dabble GamePad Analog (Joystick mode)
 * Frame:
 *   [0] 0xFF  START_OF_FRAME
 *   [1] 0x01  MODULE_ID = GAMEPAD
 *   [2] 0x02  FUNCTION_ID = GAMEPAD_ANALOG
 *   [3] 0x01  so argument = 1
 *   [4] 0x01  do dai argument = 1 byte
 *   [5] DATA  bit[7:3]=angle/15, bit[2:0]=radius
 *   [6] 0x00  END_OF_FRAME
 * ----------------------------------------------------------- */

#define DABBLE_START   0xFF
#define DABBLE_END     0x00
#define GAMEPAD_ID     0x01
#define GAMEPAD_ANALOG 0x02

/* Nguong dead-zone: radius < 2 xem nhu joystick o trung tam */
#define DEAD_ZONE_RADIUS 2

/* Van toc dich chuyen khi nhan tin hieu */
#define CMD_V_MAX     0.3f    /* m/s   */
#define CMD_OMEGA_MAX 1.0f    /* rad/s */

/* Bien toan cuc chua byte nhan tu USART3, dung chung voi callback
 * duoc dinh nghia trong communicate_uart_pi.c                      */
uint8_t joystick_rx_byte = 0;

/* May trang thai parser */
typedef enum {
    WAIT_START = 0,
    WAIT_MODULE,
    WAIT_FUNCTION,
    WAIT_ARGNO,
    WAIT_ARGLEN,
    WAIT_DATA,
    WAIT_END
} DabbleState_t;

static DabbleState_t s_state = WAIT_START;
static uint8_t s_function_id = 0;
static uint8_t s_arg_len = 0;
static uint8_t s_data_idx = 0;
static uint8_t s_data_buf[8] = {0};

/* -----------------------------------------------------------
 * Xu ly 1 byte nhan duoc tu USART3
 * Goi trong HAL_UART_RxCpltCallback (communicate_uart_pi.c)
 * ----------------------------------------------------------- */
void Joystick_Dabble_ProcessByte(uint8_t byte)
{
    switch (s_state) {
        case WAIT_START:
            if (byte == DABBLE_START) s_state = WAIT_MODULE;
            break;

        case WAIT_MODULE:
            if (byte == GAMEPAD_ID) s_state = WAIT_FUNCTION;
            else                    s_state = WAIT_START;
            break;

        case WAIT_FUNCTION:
            s_function_id = byte;
            if (byte == GAMEPAD_ANALOG) {
                s_state = WAIT_ARGNO;
            } else {
                s_state = WAIT_START;
            }
            break;

        case WAIT_ARGNO:
            /* So argument = 1, chuyen sang doc do dai argument */
            s_state = WAIT_ARGLEN;
            break;

        case WAIT_ARGLEN:
            s_arg_len = byte;
            s_data_idx = 0;
            if (s_arg_len > 0 && s_arg_len <= sizeof(s_data_buf)) {
                s_state = WAIT_DATA;
            } else {
                s_state = WAIT_START;
            }
            break;

        case WAIT_DATA:
            s_data_buf[s_data_idx++] = byte;
            if (s_data_idx >= s_arg_len) {
                s_state = WAIT_END;
            }
            break;

        case WAIT_END:
            if (byte == DABBLE_END) {
                if (s_function_id == GAMEPAD_ANALOG) {
                    /* Byte 0: Buttons (value0), Byte 1: Joystick (value: bit[7:3]=angle/15, bit[2:0]=radius) */
                    uint8_t joy_byte = (s_arg_len >= 2) ? s_data_buf[1] : s_data_buf[0];
                    uint8_t angle_idx = (joy_byte >> 3);         /* 0-24  */
                    uint8_t radius    = (joy_byte & 0x07);        /* 0-7   */
                    float   angle_rad = (float)angle_idx * 15.0f * (3.14159f / 180.0f);

                    float cmd_v     = 0.0f;
                    float cmd_omega = 0.0f;

                    if (radius >= DEAD_ZONE_RADIUS) {
                        /* x = radius*cos(angle), y = radius*sin(angle) */
                        float x = radius * cosf(angle_rad);
                        float y = radius * sinf(angle_rad);

                        /* Truc Y -> van toc thang */
                        if      (y >  0.5f) cmd_v = +CMD_V_MAX;
                        else if (y < -0.5f) cmd_v = -CMD_V_MAX;

                        /* Truc X -> van toc goc
                         * Quy uoc: x duong (cua phai) -> omega am
                         *           x am   (cua trai)  -> omega duong */
                        if      (x >  0.5f) cmd_omega = -CMD_OMEGA_MAX;
                        else if (x < -0.5f) cmd_omega = +CMD_OMEGA_MAX;
                    }

                    Kinematics_ComputeTarget(cmd_v, cmd_omega);
                }
            }
            s_state = WAIT_START;
            break;

        default:
            s_state = WAIT_START;
            break;
    }
}

/* -----------------------------------------------------------
 * Khoi dong: bat ngat nhan USART3 tung byte mot
 * ----------------------------------------------------------- */
void Joystick_Dabble_Init(void)
{
    s_state = WAIT_START;
    HAL_UART_Receive_IT(&huart3, &joystick_rx_byte, 1);
}
