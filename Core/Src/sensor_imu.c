#include "sensor_imu.h"
#include "spi.h"   // Thư viện SPI chứa hspi1
#include "main.h"

// =====================================================================
// ĐỊNH NGHĨA THANH GHI BMI323 (THEO CHUẨN DATASHEET & CODE DRONE ĐÃ TEST)
// =====================================================================
#define REG_CHIP_ID     0x00u
#define REG_ERR         0x01u
#define REG_ACC_DATA_X  0x03u
#define REG_ACC_CONF    0x20u
#define REG_GYR_CONF    0x21u
#define REG_CMD         0x7Eu

#define CHIP_ID_VALUE   0x43u
#define CMD_SOFT_RESET  0xDEAFu

#define ACC_CONF_VALUE  0x702Cu  // high-perf, ±8 g, 1.6 kHz
#define GYR_CONF_VALUE  0x704Du  // high-perf, ±2000 °/s, 3.2 kHz

// Chân CS (Chip Select)
#define BMI323_CS_PORT  GPIOD
#define BMI323_CS_PIN   GPIO_PIN_2

extern SPI_HandleTypeDef hspi1;

// =====================================================================
// BIẾN TOÀN CỤC
// =====================================================================
uint8_t bmi323_chip_id = 0;

volatile int16_t acc_raw[3] = {0, 0, 0};
volatile int16_t gyr_raw[3] = {0, 0, 0};

volatile float ax_g = 0.0f, ay_g = 0.0f, az_g = 0.0f;
volatile float gx_dps = 0.0f, gy_dps = 0.0f, gz_dps = 0.0f;

volatile float yaw = 0.0f;
static float gz_bias = 0.0f;

// =====================================================================
// HÀM GIAO TIẾP SPI ĐỒNG THỜI (FULL-DUPLEX)
// =====================================================================
static inline void cs_low(void)  
{ 
    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_RESET); 
}

static inline void cs_high(void) 
{ 
    HAL_GPIO_WritePin(BMI323_CS_PORT, BMI323_CS_PIN, GPIO_PIN_SET); 
}

static int spi_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    cs_low();
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)tx, rx, len, 10);
    cs_high();
    return (st == HAL_OK) ? 0 : -1;
}

static int read_reg(uint8_t reg, uint16_t *value)
{
    uint8_t tx[4] = {(uint8_t)(0x80u | reg), 0, 0, 0};
    uint8_t rx[4] = {0};
    if (spi_xfer(tx, rx, 4) != 0) {
        return -1;
    }
    *value = (uint16_t)(rx[2] | (rx[3] << 8));
    return 0;
}

static int write_reg(uint8_t reg, uint16_t value)
{
    uint8_t tx[3] = {(uint8_t)(reg & 0x7Fu), (uint8_t)(value & 0xFFu), (uint8_t)(value >> 8)};
    uint8_t rx[3] = {0};
    return spi_xfer(tx, rx, 3);
}

// =====================================================================
// CÁC HÀM XỬ LÝ CHÍNH
// =====================================================================
void IMU_Init(void) 
{
    cs_high();
    HAL_Delay(50);

    uint16_t v = 0;

    // 1. Cảm biến bật lên ở chế độ I2C; 1 lần đọc giả tạo cạnh lên CS sẽ chuyển sang SPI
    read_reg(REG_CHIP_ID, &v);
    HAL_Delay(2);
    read_reg(REG_CHIP_ID, &v);

    bmi323_chip_id = (uint8_t)(v & 0xFFu);

    for (int retry = 0; retry < 5 && bmi323_chip_id != CHIP_ID_VALUE; retry++) {
        HAL_Delay(20);
        read_reg(REG_CHIP_ID, &v);
        bmi323_chip_id = (uint8_t)(v & 0xFFu);
    }

    if (bmi323_chip_id != CHIP_ID_VALUE) {
        return; // Không nhận diện được cảm biến
    }

    // 2. Soft reset cảm biến
    write_reg(REG_CMD, CMD_SOFT_RESET);
    HAL_Delay(5);

    // Sau soft reset cảm biến về lại I2C mode -> đọc dummy để sang lại SPI mode
    read_reg(REG_CHIP_ID, &v);
    HAL_Delay(2);
    read_reg(REG_CHIP_ID, &v);
    bmi323_chip_id = (uint8_t)(v & 0xFFu);

    // 3. Cấu hình Accel và Gyro theo thông số chuẩn
    write_reg(REG_ACC_CONF, ACC_CONF_VALUE);
    write_reg(REG_GYR_CONF, GYR_CONF_VALUE);

    HAL_Delay(50); // Chờ Gyro ổn định (~30ms)

    // 4. Lấy mẫu 100 lần khi tĩnh để khử bias gyro Z
    float gz_sum = 0.0f;
    uint8_t tx_burst[14] = {(uint8_t)(0x80u | REG_ACC_DATA_X), 0};
    uint8_t rx_burst[14] = {0};

    for (int i = 0; i < 100; i++) {
        if (spi_xfer(tx_burst, rx_burst, 14) == 0) {
            int16_t raw_gz = (int16_t)(rx_burst[12] | (rx_burst[13] << 8));
            gz_sum += (float)raw_gz / 16.384f;
        }
        HAL_Delay(5);
    }
    gz_bias = gz_sum / 100.0f;
}

void IMU_Read_Data(void) 
{
    if (bmi323_chip_id != CHIP_ID_VALUE) return;

    // Burst 14 bytes: Cmd(1) + Dummy(1) + ACC_XYZ(6) + GYR_XYZ(6)
    uint8_t tx[14] = {(uint8_t)(0x80u | REG_ACC_DATA_X), 0};
    uint8_t rx[14] = {0};

    if (spi_xfer(tx, rx, 14) != 0) return;

    // Đọc thô Gyro Z (tọa độ byte 12 và byte 13)
    gyr_raw[2] = (int16_t)(rx[12] | (rx[13] << 8));

    // Quy đổi dps (thang ±2000 dps -> 16.384 LSB/dps) và trừ trôi tĩnh
    gz_dps = ((float)gyr_raw[2] / 16.384f) - gz_bias;
}
