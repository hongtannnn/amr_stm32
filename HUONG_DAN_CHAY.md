# 🚀 Hướng Dẫn Vận Hành Robot AMR Trên Ubuntu

Tài liệu hướng dẫn từng bước bắt đầu từ khi mở Terminal trên Ubuntu để thực hiện 3 quy trình chính: **Lập bản đồ tự động**, **Lưu bản đồ**, và **Điều hướng tự hành P2P**.

---

## 🛠️ Bước 0: Chuẩn Bị Môi Trường Trên Laptop

Mở Terminal mới (`Ctrl + Alt + T`) và chạy lệnh sau để build và nạp môi trường:

```bash
cd ~/robot_amr
colcon build
source install/setup.bash
```

> [!TIP]
> Bạn có thể thêm dòng `source ~/robot_amr/install/setup.bash` vào cuối file `~/.bashrc` bằng lệnh:
> `echo "source ~/robot_amr/install/setup.bash" >> ~/.bashrc`
> để mỗi lần mở terminal mới không cần gõ lại lệnh `source`.

---

## 📡 Bước 0.1: Các Bước Khởi Động Pi & Kết Nối SSH Sau Khi Cắm Thẻ Nhớ

1. **Cắm thẻ nhớ và cấp nguồn Pi:**
   - Cắm thẻ nhớ vào khe cắm trên Raspberry Pi, sau đó cắm nguồn cho Pi.
   - Chờ khoảng 1 đến 2 phút để hệ thống Pi khởi động hoàn tất và tự động kết nối vào mạng Wi-Fi `Kien Bui_5Ghz`.

2. **Dò tìm địa chỉ IP của Pi từ laptop:**
   - Đảm bảo laptop cũng đang kết nối Wi-Fi `Kien Bui_5Ghz`.
   - Mở Terminal trên laptop và kiểm tra kết nối với Pi:
     ```bash
     ping raspberrypi.local
     ```
   - Nếu tìm thấy tín hiệu phản hồi, bạn sẽ thấy địa chỉ IP (ví dụ: `192.168.1.xxx`).
   - Trường hợp không ping được bằng tên, dùng lệnh quét mạng:
     ```bash
     arp -a
     ```

3. **Mở 2 Tab Terminal trên laptop để SSH vào Pi:**
   - Mở Tab 1:
     ```bash
     ssh ubuntu@<IP_Pi_tim_duoc>
     # (Hoặc: ssh quangthao2005@<IP_Pi_tim_duoc> tùy user tạo trên Pi)
     ```
   - Mở Tab 2 và gõ lệnh SSH tương tự để chuẩn bị chạy mắt quét LiDAR.

---

## 🗺️ Quy Trình 1: Lập Map Tự Động (Autonomous Exploration)

### Terminal 1: Khởi động phần cứng

- **Trên Laptop (Hiển thị mô hình 3D trên RViz):**
```bash
cd ~/robot_amr && source install/setup.bash
ros2 launch robot_description display.launch.py
```

- **Trên Raspberry Pi (Mở 2 tab riêng đã SSH vào Pi ở Bước 0.1):**
```bash
# Tab 1.1 (SSH Pi): Kết nối vi điều khiển STM32 đọc Odom
cd ~/robot_amr && source install/setup.bash
ros2 launch robot_odom odom.launch.py

# Tab 1.2 (SSH Pi): Mắt quét laser RPLidar
cd ~/robot_amr && source install/setup.bash
ros2 launch rp_lidar rp_lidar.launch.py
```

### Terminal 2 (Trên Laptop): Khởi động thuật toán SLAM vẽ bản đồ
Tắt RViz của SLAM để tránh mở trùng lặp cửa sổ hiển thị:
```bash
cd ~/robot_amr && source install/setup.bash
ros2 launch robot_mapping slam_toolbox.launch.py use_rviz:=false
```

### Terminal 3 (Trên Laptop): Kích hoạt hoa tiêu tự hành thám hiểm
Lệnh này sẽ khởi động Nav2 + node `explore_lite` + giao diện RViz điều khiển:
```bash
cd ~/robot_amr && source install/setup.bash
ros2 launch robot_navigation explore.launch.py
```
*(Robot sẽ tự động tìm các vùng biên giới sương mù và lăn bánh đi thám hiểm phủ kín phòng).*

---

## 💾 Quy Trình 2: Lưu Bản Đồ

Khi quan sát trên RViz thấy robot đã vẽ kín các bức tường và góc phòng, mở Terminal mới trên laptop để lưu map:

```bash
cd ~/robot_amr && source install/setup.bash
ros2 run nav2_map_server map_saver_cli -f ~/robot_amr/src/robot_navigation/maps/my_map
```

> [!NOTE]
> Lệnh này sẽ tạo ra 2 file: `my_map.yaml` (thông số) và `my_map.pgm` (hình ảnh bản đồ) trong thư mục `robot_navigation/maps/`.

---

## 🎯 Quy Trình 3: Điều Hướng P2P Dẫn Đường (Khi Đã Có Bản Đồ)

Sau khi lưu map, bạn bấm `Ctrl + C` ở các Terminal của Quy trình 1 để dừng chế độ thám hiểm.

### Bước 3.1: Khởi động hệ thống dẫn đường trọn gói
Chạy file bringup toàn bộ phần cứng và Nav2 với bản đồ vừa lưu:
```bash
cd ~/robot_amr && source install/setup.bash
ros2 launch robot_navigation bringup_all.launch.py map:=$HOME/robot_amr/src/robot_navigation/maps/my_map.yaml
```

### Bước 3.2: Thao tác chấm điểm đến trên màn hình RViz
1. **Khởi tạo vị trí ban đầu (2D Pose Estimate):**
   - Click chọn công cụ **2D Pose Estimate** trên thanh công cụ đỉnh màn hình RViz.
   - Click và giữ chuột trái kéo theo hướng xe đang đứng thực tế để AMCL định vị chuẩn xác.
2. **Ra lệnh điểm đến (Nav2 Goal):**
   - Click công cụ **Nav2 Goal** (hoặc **2D Goal Pose**).
   - Click và kéo chuột tại vị trí muốn xe đến.
   - Xe sẽ tự động tính toán lộ trình tối ưu và di chuyển tới đích.
