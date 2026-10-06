/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "spi.h"
#include "gpio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Định nghĩa thanh ghi cơ bản của BMI323
#define BMI323_CHIP_ID_REG      0x00
#define BMI323_ACCEL_X_L_REG    0x03

// Macro điều khiển chân CS (Chân PD2)
#define BMI323_CS_LOW()         HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET)
#define BMI323_CS_HIGH()        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief Hàm đọc nhiều thanh ghi từ BMI323 qua SPI
  */
void BMI323_ReadRegisters(uint8_t regAddr, uint8_t *pData, uint16_t size)
{
    uint8_t txBuf[2];
    // Giao thức SPI của Bosch: Bit MSB của địa chỉ phải là 1 để báo hiệu lệnh Đọc (Read = 0x80)
    txBuf[0] = regAddr | 0x80;
    // Giao thức SPI của Bosch yêu cầu 1 byte dummy trống sau byte địa chỉ
    txBuf[1] = 0x00;

    BMI323_CS_LOW();

    // Gửi byte địa chỉ và byte dummy
    HAL_SPI_Transmit(&hspi1, txBuf, 2, HAL_MAX_DELAY);

    // Nhận dữ liệu thực tế
    HAL_SPI_Receive(&hspi1, pData, size, HAL_MAX_DELAY);

    BMI323_CS_HIGH();
}

// Biến toàn cục lưu trữ dữ liệu
int16_t accel_x, accel_y, accel_z;
int16_t gyro_x, gyro_y, gyro_z;
uint8_t rawData[12];
uint8_t chipID;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
    // Kéo chân CS lên mức cao để mặc định không chọn chip
    BMI323_CS_HIGH();
    HAL_Delay(100);

    // Kiểm tra kết nối (Chip ID của BMI323 thường trả về 0x43)
    BMI323_ReadRegisters(BMI323_CHIP_ID_REG, &chipID, 1);

    // Khai báo biến lưu mốc thời gian và thứ tự đèn
    uint32_t last_blink_time = HAL_GetTick();
    uint8_t led_step = 0;
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
      // Đọc liên tục 12 bytes bắt đầu từ thanh ghi 0x03 (chứa cả Accel 6 bytes và Gyro 6 bytes)
      BMI323_ReadRegisters(BMI323_ACCEL_X_L_REG, rawData, 12);

      // Nối 2 byte 8-bit lại thành dữ liệu 16-bit có dấu
      accel_x = (int16_t)((rawData[1] << 8) | rawData[0]);
      accel_y = (int16_t)((rawData[3] << 8) | rawData[2]);
      accel_z = (int16_t)((rawData[5] << 8) | rawData[4]);

      gyro_x  = (int16_t)((rawData[7] << 8) | rawData[6]);
      gyro_y  = (int16_t)((rawData[9] << 8) | rawData[8]);
      gyro_z  = (int16_t)((rawData[11] << 8) | rawData[10]);

      // Kiểm tra nếu đã trôi qua 300ms thì chuyển sang đèn tiếp theo
      if (HAL_GetTick() - last_blink_time >= 300)
      {
          // BƯỚC 1: Tắt tất cả 3 đèn
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

          // BƯỚC 2: Bật 1 đèn duy nhất theo thứ tự của led_step
          switch (led_step) {
              case 0:
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
                  break;
              case 1:
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
                  break;
              case 2:
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
                  break;
          }

          // BƯỚC 3: Tăng thứ tự đèn lên 1. Nếu vượt quá đèn số 3 (index 2) thì quay lại từ đầu
          led_step++;
          if (led_step > 2) {
              led_step = 0;
          }

          // Cập nhật lại mốc thời gian
          last_blink_time = HAL_GetTick();
      }

      HAL_Delay(10); // Tốc độ lấy mẫu tương đối 100Hz của cảm biến
      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
