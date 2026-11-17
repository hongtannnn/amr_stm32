# PROJECT PROGRESS - AMR STM32F405RGT6 FIRMWARE

Dự án điều khiển xe tự hành AMR (Autonomous Mobile Robot) tầng thấp sử dụng STM32F405RGT6 kết hợp FreeRTOS và giao tiếp tầng trên (Raspberry Pi / ROS 2).

---

## 📌 Lịch sử cập nhật và thay đổi thực thi

### [2026-10-19 / 2026-10-20] - Tối ưu hóa IMU & Dọn dẹp Code
- **Dọn dẹp biến IMU (BMI323):**
  - Xóa bỏ các khai báo biến dư thừa không sử dụng trong `sensor_imu.h` và `sensor_imu.c`: `acc_raw`, `gyr_raw`, `ax_g`, `ay_g`, `az_g`, `gx_dps`, `gy_dps`, `yaw`.
  - Chuyển `bmi323_chip_id` thành biến `static` nội bộ nhằm bảo vệ dữ liệu và giới hạn phạm vi trong file.
  - Tối ưu hóa hàm `IMU_Read_Data`: chuyển việc đọc byte Gyro Z sang biến cục bộ `raw_gz` trên stack thay vì mảng toàn cục `gyr_raw`.
  - Giữ lại duy nhất biến toàn cục `gz_dps` phục vụ bù lệch hướng thẳng và gửi lên ROS 2.
- **Chuẩn hóa truyền thông UART Raspberry Pi (`communicate_uart_pi`):**
  - Khảo sát và hoàn thiện cấu trúc gói tin `CommunicatePi_TelemetryPacket_t` với `#pragma pack(push, 1)` (loại bỏ padding byte để tối ưu độ dài 21 bytes truyền qua UART2).
  - Thu thập tín hiệu tay điều khiển Dabble GamePad thông qua ngắt `HAL_UART_RxCpltCallback` của USART3.

### [2026-10-21] - Tính năng Tự động Phân đoạn Phòng & Điều phối Hàng đợi (Queue)
- **Thiết kế kế hoạch kỹ thuật (`plan_auto_room_segmentation_queue.md`):**
  - Xây dựng giải pháp tự động nhận diện phòng từ bản đồ SLAM OccupancyGrid thay vì hardcode tọa độ thủ công.
- **Module tự động phân đoạn phòng (`ros2_scripts/auto_room_segmenter.py`):**
  - Áp dụng xử lý hình thái học (Morphological Erosion) và Connected Components để tách cửa phòng và trích xuất tâm phòng `(x, y)`.
  - Tự động gán nhãn phòng từ 1 đến N theo chuẩn không gian và xuất file cấu hình `rooms_config.yaml`.
- **Module điều phối hàng đợi (`ros2_scripts/queue_mission_dispatcher.py`):**
  - Sử dụng Nav2 Simple Commander API tiếp nhận chuỗi hàng đợi (ví dụ: `1 4 3 6 5 2`), điều hướng robot tuần tự qua từng phòng kèm thời gian dừng trả hàng (`dwell_time`).

### [2026-10-22] - Cài đặt Kiro-Antigravity Bridge
- **Tích hợp kênh giao tiếp song song (`kiro-antigravity-bridge`):**
  - Thiết lập thư mục `.agents/` và `.kiro/` tại gốc dự án để chia sẻ ngữ cảnh và điều phối nhiệm vụ 2 chiều giữa Kiro và Antigravity IDE.
  - Sẵn sàng cơ chế hook tự động tải nhiệm vụ tồn đọng thông qua `hooks.json` và `pre_invocation.js`.

### [2026-11-10] - Tạm thời Cô lập Task1 để Kiểm thử Động cơ Thuần
- **Cấu hình chạy thử nghiệm trực tiếp `Motor_Run(1000, 1000)` (`freertos.c`):**
  - Khởi tạo riêng `Motor_Init()` trong `StartTask1`.
  - Tạm thời vô hiệu hóa bộ điều khiển vòng kín `Motion_Control_Update` và biến thời gian `dt` để loại bỏ toàn bộ các điều kiện ngắt an toàn và PID khi đo đạc phần cứng.
  - Cung cấp xung cực đại 100% đến cả 2 kênh động cơ nhằm thẩm định tình trạng hoạt động vật lý của module driver TB6612.

### [2026-11-10] - Chuẩn hóa Điều khiển Driver Động cơ TB6612FNG
- **Sửa logic điều khiển kênh Motor A và B (`driver_tb6612fng.c`):**
  - Khắc phục lỗi đọc chéo biến: gán đúng `left_wheel` cho Motor A (PB10, PC0, PC1) và `right_wheel` cho Motor B (PB11, PC2, PC3).
  - Đảm bảo tính nhất quán tuyệt đối giữa lệnh vận tốc đặt và chiều quay thực tế của bánh xe.

### [2026-11-10] - Tối ưu Truyền thông Song công Toàn phần UART2 STM32
- **Chuyển đổi truyền Telemetry sang Non-blocking IT (`communicate_uart_pi.c`):**
  - Thay thế hàm gửi chặn `HAL_UART_Transmit` bằng hàm gửi ngắt `HAL_UART_Transmit_IT`.
  - Chuyển `pkt` thành biến tĩnh `static CommunicatePi_TelemetryPacket_t s_tx_pkt` để bảo toàn bộ nhớ trong suốt quá trình truyền ngắt.
  - Bổ sung kiểm tra trạng thái sẵn sàng `huart2.gState == HAL_UART_STATE_READY` trước khi phát gói mới.
  - Khắc phục triệt để lỗi nghẽn Overrun (ORE) trên ngắt nhận `HAL_UART_Receive_IT`, đảm bảo tiếp nhận 100% gói tin điều khiển `cmd_vel` từ Raspberry Pi.

### [2026-10-29] - Hoàn thiện Cầu nối Giao tiếp Hai Chiều UART ROS 2 (`robot_odom`)
- **Đồng bộ hóa giao thức truyền nhận trên Raspberry Pi (`odom_node.py`):**
  - Tích hợp subscriber `/cmd_vel` (kiểu `geometry_msgs/Twist`), đóng gói khung truyền 13 bytes (`0xAA 0x55 0x02 0x08 <cmd_v> <cmd_omega> <checksum>`) gửi định kỳ 50Hz xuống STM32.
  - Bổ sung cơ chế Watchdog ngắt an toàn trên Pi: tự động đưa vận tốc về 0.0 nếu mất lệnh `/cmd_vel` quá 500ms.
  - Trích xuất toàn bộ gói tin Telemetry 21 bytes từ STM32: vừa tính toán Odometry từ encoder, vừa trích xuất và chuyển đổi `gyro_z` (dps -> rad/s) xuất bản ra chủ đề `/imu/data_raw` (`sensor_msgs/Imu`), vừa xuất bản mức điện áp pin ra chủ đề `/battery_state` (`sensor_msgs/BatteryState`).
  - Thêm phụ thuộc `sensor_msgs` vào `package.xml` của gói `robot_odom`.

### [2026-11-17] - Phát hành Nhánh Firmware mcu_v3
- **Tạo nhánh phát triển mới (`mcu_v3`):**
  - Đồng bộ toàn bộ mã nguồn điều khiển mới nhất: tích hợp chuẩn hóa kênh PWM TB6612, truyền thông non-blocking IT UART2, loại bỏ module Dabble joystick thừa.
  - Thiết lập `.gitignore` loại bỏ thư mục build `Debug/` và các tệp sinh tự động để tối ưu dung lượng kho lưu trữ.
  - Đẩy mã nguồn hoàn chỉnh lên GitHub repository `amr_stm32` nhánh `mcu_v3`.

### [2026-11-14] - Khắc phục Lỗi Đấu Chéo Kênh Đọc Encoder & Bão Hòa PID
- **Sửa hoán đổi kênh đọc Timer Encoder (`sensor_encoder.c`):**
  - Khắc phục lỗi gán nhầm kênh: gán đúng `cur_cnt_left` đọc từ `htim5` (bánh trái) và `cur_cnt_right` đọc từ `htim3` (bánh phải).
  - Chuẩn hóa dấu tính toán tích lũy: `left_encoder += delta_left` và `right_encoder -= delta_right` để đảm bảo cả hai encoder cùng tăng giá trị dương khi robot di chuyển tiến về phía trước.
  - Xóa bỏ hiện tượng phản hồi chéo làm bộ điều khiển PID bánh trái bị tích phân bão hòa kịch trần xung PWM 1000 (CCR3).
- **Đơn giản hóa định thời chu kỳ Task1 (`freertos.c`):**
  - Loại bỏ biến `previous_wake_time` và thay hàm `osDelayUntil` bằng `osDelay(5)` trong `StartTask1` theo yêu cầu kiểm thử.
- **Cấu hình Vận tốc Khởi động & Đảo Chiều Góc Lái (`differential_drive_kinematics.c`):**
  - Khởi tạo mặc định `g_kinematics.cmd_v = 0.5f` để phục vụ chạy kiểm thử trực tiếp ngay khi cấp nguồn.
  - Đảo dấu vận tốc góc `g_kinematics.cmd_omega = -omega` để chuẩn hóa đúng chiều quay góc trong ROS (phím `j` rẽ trái, `l` rẽ phải).
- **Bổ sung Chế độ Chạy Kiểm thử Vòng Hở Thuần Túy (`motion_control.c` & `freertos.c`):**
  - Tạo hàm `Motion_Control_TestOpenLoop(dt)` xuất trực tiếp xung PWM từ công thức động học vi sai (`target_pwm_left`, `target_pwm_right`), bỏ qua hoàn toàn các khâu PID.
  - Vẫn duy trì cập nhật encoder và IMU để đo đạc và quan sát vận tốc thực tế của từng bánh trên bảng Live Expressions.
  - Kích hoạt chế độ kiểm thử này trong `StartTask1` của FreeRTOS.
- **Tạm thời Vô Hiệu Hóa Bộ Bù Góc IMU Yaw PID (`motion_control.c` & `freertos.c`):**
  - Comment dòng khởi tạo `s_pid_yaw` cũ và đặt `PID_Init(&s_pid_yaw, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f)` để triệt tiêu việc IMU can thiệp bẻ lái làm xe xoay đầu lúc khởi hành.
  - Chuyển `StartTask1` trong `freertos.c` trở lại chạy hàm vòng kín `Motion_Control_Update(dt)` để kiểm tra chuyển động thẳng.
- **Áp dụng Điều Khiển Kết Hợp Feedforward + PID (`motion_control.c`):**
  - Cập nhật ngõ ra xung `pwm = target_pwm + PID_Compute(...)` để cấp ngay xung cơ sở đồng đều cho hai bánh từ công thức động học vi sai, loại bỏ độ trễ ma sát tĩnh ban đầu.
  - Hạ hệ số PID hai bánh xuống `Kp = 100.0f`, `Ki = 50.0f` với biên độ bù giới hạn `[-500, 500]` để PID chỉ tập trung tinh chỉnh sai số nhỏ, loại bỏ hoàn toàn hiện tượng lệch góc khi đề-pa.
- **Kích Hoạt Lại Bộ Giữ Thẳng Hướng IMU Yaw PID An Toàn (`motion_control.c`):**
  - Cấu hình lại `s_pid_yaw` với hệ số P êm ái: `Kp = 0.02f`, `Ki = 0.0f` để chống trôi tích phân.
  - Thêm điều kiện kích hoạt an toàn: chỉ bù lệch hướng khi xe đã có đà chạy ổn định (`|target_v| > 0.05 m/s`), tránh bị giật góc bởi rung lắc lúc vừa xuất phát.
- **Bổ Sung Thuật Toán Đồng Bộ Khóa Cứng Xung Encoder Khi Đi Thẳng (`motion_control.c`):**
  - Chuyển đổi bộ điều khiển `s_pid_sync` từ việc so sánh vận tốc `v_diff` sang khóa cứng sai lệch xung lũy kế `sync_tick_error = (left_ticks - right_ticks)`.
  - Cấu hình hệ số: `Kp = 0.0003`, `Ki = 0.0`, `Kd = 0.00005`, giới hạn bù vận tốc `[-0.08, 0.08]` m/s; bổ sung cơ chế chốt mốc xung ban đầu khi bắt đầu đi thẳng.
  - Thêm biến quan sát `sync_tick_error` lên Live Expressions giúp giám sát độ chênh lệch xung thực tế giữa hai bánh.
- **Sửa Lỗi Cú Pháp Thiếu Dấu Ngoặc Nhọn (`usart.c`):**
  - Thêm dấu đóng ngoặc nhọn `}` kết thúc hàm `HAL_UART_MspInit()`, khắc phục lỗi biên dịch cú pháp khiến hàm `HAL_UART_MspDeInit()` bị hiểu nhầm là khai báo lồng.














