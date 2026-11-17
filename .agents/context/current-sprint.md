# Current Sprint

<!-- Active goals, in-progress features, known blockers -->

## Goals
- Clean up Dabble joystick module and maintain pure bidirectional UART2 communication between STM32 and Raspberry Pi.
- Deploy Intel RealSense D435 for Social-Aware Navigation (Asymmetric 2D Gaussian costmap).
- Implement Automatic Room Segmentation (1..N) and Multi-Room Queue Mission Dispatcher via Nav2.

## In Progress
- Completed: Pushed clean firmware to GitHub repository `amr_stm32` on branch `mcu_v3` with build artifacts ignored.
- Completed: Fixed syntax error in `usart.c` by adding missing closing brace `}` to `HAL_UART_MspInit()`.
- Completed: Upgraded straight-line synchronization (`s_pid_sync`) in `motion_control.c` from velocity difference to direct cumulative encoder tick locking (`sync_tick_error = left_ticks - right_ticks`), ensuring exact pulse matching between wheels.
- Completed: Isolated FreeRTOS Task1 (`freertos.c`) to run direct open-loop `Motor_Run(1000, 1000)` testing with `Motor_Init()` to inspect TB6612 hardware channels without closed-loop PID interference.
- Completed: Standardized `driver_tb6612fng.c` channel mappings by properly assigning `left_wheel` to Motor A (PB10, PC0, PC1) and `right_wheel` to Motor B (PB11, PC2, PC3).
- Completed: Optimized STM32 UART2 telemetry transmission to non-blocking `HAL_UART_Transmit_IT` with static packet buffer and state check, resolving RX overrun (ORE) issues during bidirectional full-duplex communication.
- Completed: Implemented bidirectional UART hardware bridge node on ROS 2 (`robot_odom/odom_node.py`) with 50Hz cmd_vel transmission, watchdog safety stop, and full extraction of gyro_z (`/imu/data_raw`) and battery (`/battery_state`).
- Completed: Removed `joystick_dabble` module from STM32 firmware and isolated bidirectional communication on UART2 (Telemetry: encoders, gyro_z, battery; Command: cmd_v, cmd_omega).
- Watchdog: Enabled `CommunicatePi_CheckTimeout(500)` in FreeRTOS Task2 for safe shutdown upon loss of Pi connection.
- High-level ROS 2 scripts ready: `auto_room_segmenter.py` and `queue_mission_dispatcher.py`.

## Blockers
- None. Firmware is clean and ready for hardware testing with Raspberry Pi.
