# Hướng Dẫn Chạy Nav2 Cho Robot AMR

Package này (`robot_navigation`) được thiết kế riêng cho robot AMR vi sai (differential drive) với phần cứng:
- **MCU**: STM32 gửi nhận dữ liệu qua Serial (baud 115200) qua package `robot_odom`
- **LiDAR**: RPLIDAR A1M8 (frame `laser`, topic `/scan`) qua package `rp_lidar`
- **Model Robot**: URDF trong `robot_description` (base_link, wheels, lidar)
- **Kinematics**: Bán kính bánh xe $R = 0.033\text{ m}$, khoảng cách hai bánh $L = 0.193\text{ m}$, bán kính thân robot $r \approx 0.16\text{ m}$.

---

## 1. Cài đặt các gói phụ thuộc (Dependencies)

Nếu trên máy / Raspberry Pi / Jetson chưa cài Nav2:

```bash
sudo apt update
# Dành cho ROS 2 Humble (nếu dùng bản khác, thay 'humble' bằng tên distro tương ứng):
sudo apt install ros-humble-navigation2 ros-humble-nav2-bringup
```

---

## 2. Build dự án

Đứng tại thư mục gốc của workspace (`robot_amr`):

```bash
cd ~/robot_amr  # hoặc đường dẫn workspace của bạn
colcon build --symlink-install --packages-select robot_navigation
source install/setup.bash
```

---

## 3. Các bước vận hành Nav2

### Bước 3.1: Quét và lưu bản đồ (SLAM) - Thực hiện 1 lần đầu
1. Khởi động phần cứng và thuật toán quét bản đồ (SLAM Toolbox):
   ```bash
   ros2 launch robot_description display.launch.py
   ros2 launch robot_odom odom.launch.py
   ros2 launch rp_lidar rp_lidar.launch.py
   ros2 launch robot_mapping slam_toolbox.launch.py
   ```
2. Điều khiển robot di chuyển quanh phòng để quét hết bản đồ.
3. Lưu bản đồ vào thư mục maps của `robot_navigation`:
   ```bash
   ros2 run nav2_map_server map_saver_cli -f src/robot_navigation/maps/my_map
   ```

---

### Bước 3.2: Chạy Navigation 2 (Điều hướng tự động)

#### Cách 1: Chạy toàn bộ hệ thống bằng 1 lệnh duy nhất (Khuyên dùng)
```bash
ros2 launch robot_navigation bringup_all.launch.py map:=$(ros2 pkg prefix robot_navigation)/share/robot_navigation/maps/map.yaml
```
*(Nếu bạn đã lưu bản đồ riêng, truyền đường dẫn tới file `.yaml` của bạn vào tham số `map:=...`)*

#### Cách 2: Khởi động từng module riêng biệt
- **Terminal 1 (Robot Description & TF)**:
  ```bash
  ros2 launch robot_description display.launch.py
  ```
- **Terminal 2 (Odometry & Motor Bridge)**:
  ```bash
  ros2 launch robot_odom odom.launch.py
  ```
- **Terminal 3 (LiDAR)**:
  ```bash
  ros2 launch rp_lidar rp_lidar.launch.py
  ```
- **Terminal 4 (Nav2 Stack + RViz2)**:
  ```bash
  ros2 launch robot_navigation navigation.launch.py map:=/path/to/your_map.yaml
  ```

---

## 4. Thao tác trên RViz2

1. **Khởi tạo vị trí ban đầu (Initial Pose)**:
   - Trên thanh công cụ RViz, chọn nút **`2D Pose Estimate`**.
   - Nhấp chuột vào vị trí thực tế của robot trên bản đồ, kéo chuột để xác định hướng nhìn của robot.
   - Quan sát cụm tia AMCL (màu đỏ) co lại đúng với đường biên của laser scan.

2. **Gửi điểm đến (Navigation Goal)**:
   - Chọn nút **`Nav2 Goal`** (hoặc **`2D Goal Pose`**) trên thanh công cụ RViz.
   - Nhấp và kéo mũi tên tới vị trí đích. Robot sẽ tự tính toán đường đi toàn cục (đường xanh lá), tạo đường tránh vật cản cục bộ (đường xanh dương) và tự động di chuyển đến đích.

---

## 5. Lập trình điều hướng tự động bằng code Python

### Gửi một điểm đến (Navigate To Pose)
```bash
ros2 run robot_navigation navigate_to_pose <X> <Y> <YAW_RAD>
# Ví dụ:
ros2 run robot_navigation navigate_to_pose 1.5 2.0 1.57
```

### Tuần tra qua danh sách nhiều điểm (Waypoint Follower)
```bash
ros2 run robot_navigation waypoint_follower
```
*(Bạn có thể mở file `robot_navigation/waypoint_follower.py` để chỉnh sửa danh sách tọa độ các điểm tuần tra theo ý muốn).*
