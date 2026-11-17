# Project Overview

Dự án điều khiển xe tự hành AMR (Autonomous Mobile Robot) tầng thấp sử dụng vi điều khiển STM32F405RGT6 kết hợp FreeRTOS, giao tiếp UART tầng trên với Raspberry Pi / ROS 2.

## Purpose
- Điều khiển động cơ, giải mã encoder, xử lý cảm biến IMU (BMI323), và điều hướng tầng thấp.
- Phối hợp với ROS 2 Nav2 và các script điều phối nhiệm vụ hàng đợi phòng.

## Tech Stack
- Firmware: C, STM32CubeMX, FreeRTOS, HAL Drivers.
- Hardware: STM32F405RGT6, BMI323 IMU, DC Motor Drivers, Optical Encoders.
- High-level: Raspberry Pi, ROS 2 (Humble), Python (Nav2 Commander API).

## Architecture
- Low-level (STM32): FreeRTOS tasks (Motor control PID, IMU processing, UART telemetry).
- High-level (Pi/PC): ROS 2 navigation stack, queue mission dispatcher, auto room segmenter.
