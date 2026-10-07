#include "sensor_imu.h"
#include "spi.h"   // Thư viện SPI do CubeMX sinh ra (chứa hspi1)
#include "main.h"  // Chứa định nghĩa các chân GPIO

// =====================================================================
// ĐỊNH NGHĨA SPI & THANH GHI BMI323
// =====================================================================
#define BMI323_REG_CHIP_ID  0x00
#define BMI323_REG_ACC_CONF 0x20
#define BMI323_REG_GYR_CONF 0x21
#define BMI323_REG_ACC_X    0x03

// Chân CS (Chip Select)
#define BMI323_CS_PORT      GPIOD
#define BMI323_CS_PIN       GPIO_PIN_2

extern SPI_HandleTypeDef hspi1;

// =====================================================================
// BIẾN TOÀN CỤC
// =====================================================================
uint8_t bmi323_chip_id = 0;

volatile int16_t acc_raw[3] = {0, 0, 0};
volatile int16_t gyr_raw[3] = {0, 0, 0};

volatile float ax_g = 0.0f, ay_g = 0.0f, az_g = 0.0f;
volatile float gx_dps = 0.0f, gy_dps = 0.0f, gz_dps = 0.0f;

// =====================================================================
// HÀM TRỢ GIÚP GIAO TIẾP SPI CHO BMI323
// =====================================================================
static void BMI323_SPI_Write(uint8_t reg, uint8_t *data, uint16_t len) {
    uint8_t tx_reg = reg & 0x7F; // Bit 7 = 0 để Ghi

    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &tx_reg, 1, 10);
    HAL_SPI_Transmit(&hspi1, data, len, 10);
    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_SET);
}

static void BMI323_SPI_Read(uint8_t reg, uint8_t *data, uint16_t len) {
    uint8_t tx_reg = reg | 0x80; // Bit 7 = 1 để Đọc
    uint8_t dummy = 0x00;

    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &tx_reg, 1, 10);

    // Đọc Dummy Byte (Bắt buộc với cảm biến Bosch trên SPI)
    HAL_SPI_Receive(&hspi1, &dummy, 1, 10);

    HAL_SPI_Receive(&hspi1, data, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_SET);
}

// =====================================================================
// CÁC HÀM XỬ LÝ CHÍNH
// =====================================================================
void IMU_Init(void) {
    uint8_t id_data[2] = {0};

    // 1. Chờ cảm biến lên nguồn
    HAL_Delay(100);

    // 2. Ép cảm biến chuyển từ I2C sang SPI mode (Gửi 1 xung giả vào chân CS)
    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    // 3. Đọc ID để kiểm tra kết nối
    for(int retry = 0; retry < 5; retry++) {
        BMI323_SPI_Read(BMI323_REG_CHIP_ID, id_data, 2);
        bmi323_chip_id = id_data[0];

        if (bmi323_chip_id == 0x43) break;
        HAL_Delay(50);
    }

    // 4. Cấu hình cảm biến nếu nhận diện đúng Chip ID
    if (bmi323_chip_id == 0x43) {
        // Bật Accel (Gửi LSB, MSB)
        uint8_t acc_cfg[2] = {0x08, 0x70};
        BMI323_SPI_Write(BMI323_REG_ACC_CONF, acc_cfg, 2);

        // Bật Gyro (Gửi LSB, MSB)
        uint8_t gyr_cfg[2] = {0x08, 0x70};
        BMI323_SPI_Write(BMI323_REG_GYR_CONF, gyr_cfg, 2);

        HAL_Delay(50); // Chờ cấu hình áp dụng thành công
    }
}

void IMU_Read_Data(void) {
    if (bmi323_chip_id != 0x43) return; // Nếu cảm biến lỗi thì không đọc

    uint8_t raw_data[12];

    // Đọc một mạch 12 byte từ thanh ghi ACC_X (0x03)
    BMI323_SPI_Read(BMI323_REG_ACC_X, raw_data, 12);

    // 1. Ghép 2 byte lại để lấy giá trị Raw (int16_t)
    acc_raw[0] = (int16_t)((raw_data[1] << 8)  | raw_data[0]);
    acc_raw[1] = (int16_t)((raw_data[3] << 8)  | raw_data[2]);
    acc_raw[2] = (int16_t)((raw_data[5] << 8)  | raw_data[4]);

    gyr_raw[0] = (int16_t)((raw_data[7] << 8)  | raw_data[6]);
    gyr_raw[1] = (int16_t)((raw_data[9] << 8)  | raw_data[8]);
    gyr_raw[2] = (int16_t)((raw_data[11] << 8) | raw_data[10]);

    // 2. Chuyển đổi sang đơn vị thực tế (Dựa vào dải đo mặc định của BMI323)
    // Gia tốc chia 16384 (dải +-2g) | Gyro chia 262.4 (dải +-125 dps)
    ax_g = acc_raw[0] / 16384.0f;
    ay_g = acc_raw[1] / 16384.0f;
    az_g = acc_raw[2] / 16384.0f;

    gx_dps = gyr_raw[0] / 262.4f;
    gy_dps = gyr_raw[1] / 262.4f;
    gz_dps = gyr_raw[2] / 262.4f;
}
