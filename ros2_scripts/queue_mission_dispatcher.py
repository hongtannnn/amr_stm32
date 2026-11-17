#!/usr/bin/env python3
"""
queue_mission_dispatcher.py - Module điều phối robot tự động di chuyển theo hàng đợi (Queue)
Sử dụng Nav2 Simple Commander API trên ROS 2 (Raspberry Pi 4B).

Cách sử dụng:
    python3 queue_mission_dispatcher.py --queue 1 4 3 6 5 2 --dwell 5
Hoặc chạy lệnh ros2 run:
    ros2 run amr_navigation queue_dispatcher --queue 1 4 3 6 5 2
"""

import os
import sys
import time
import math
import argparse
import yaml

try:
    import rclpy
    from rclpy.node import Node
    from geometry_msgs.msg import PoseStamped
    from nav2_simple_commander.robot_navigator import BasicNavigator, TaskResult
except ImportError:
    # Nếu chạy thử trên máy chưa có ROS 2
    rclpy = None
    BasicNavigator = None
    TaskResult = None


def euler_to_quaternion(yaw_rad):
    """Quy đổi góc Yaw (radian) sang Quaternion (qx, qy, qz, qw)"""
    qw = math.cos(yaw_rad * 0.5)
    qz = math.sin(yaw_rad * 0.5)
    return 0.0, 0.0, qz, qw


class QueueMissionDispatcher:
    def __init__(self, config_path="rooms_config.yaml", dwell_time_sec=5.0):
        self.config_path = config_path
        self.dwell_time_sec = dwell_time_sec
        self.rooms = self.load_rooms()

    def load_rooms(self):
        if not os.path.exists(self.config_path):
            raise FileNotFoundError(f"[ERROR] Không tìm thấy file tọa độ phòng: {self.config_path}. Hãy chạy auto_room_segmenter.py trước.")
        
        with open(self.config_path, 'r', encoding='utf-8') as f:
            data = yaml.safe_load(f)
        
        room_map = {}
        for room_name, info in data.get('rooms', {}).items():
            num = info.get('room_number')
            room_map[num] = info
        return room_map

    def execute_queue(self, queue_list):
        if rclpy is None:
            print("[SIMULATION / TEST CHƯA CÓ ROS 2]")
            print(f"Hàng đợi cần đi: {queue_list}")
            for r_num in queue_list:
                if r_num not in self.rooms:
                    print(f"  [CẢNH BÁO] Không tìm thấy phòng {r_num} trong cấu hình!")
                    continue
                info = self.rooms[r_num]
                print(f"  -> Đang di chuyển tới Phòng {r_num} (Tọa độ x={info['x']}, y={info['y']})...")
                time.sleep(1.0)
                print(f"  -> [ĐÃ TỚI PHÒNG {r_num}] - Dừng chờ {self.dwell_time_sec}s để trả hàng...")
                time.sleep(1.0)
            print("[HOÀN THÀNH TOÀN BỘ HÀNG ĐỢI!]")
            return

        rclpy.init()
        navigator = BasicNavigator()

        # Chờ hệ thống Nav2 sẵn sàng
        print("[INFO] Đang chờ hệ thống Nav2 kích hoạt...")
        navigator.waitUntilNav2Active()

        print(f"[OK] Bắt đầu hành trình theo hàng đợi: {queue_list}")

        for step_idx, room_num in enumerate(queue_list, start=1):
            if room_num not in self.rooms:
                print(f"[CẢNH BÁO] Bỏ qua Phòng {room_num} do không tồn tại trong bản đồ!")
                continue

            target_info = self.rooms[room_num]
            target_pose = PoseStamped()
            target_pose.header.frame_id = 'map'
            target_pose.header.stamp = navigator.get_clock().now().to_msg()
            target_pose.pose.position.x = float(target_info['x'])
            target_pose.pose.position.y = float(target_info['y'])
            target_pose.pose.position.z = 0.0

            yaw_rad = math.radians(float(target_info.get('yaw_deg', 0.0)))
            qx, qy, qz, qw = euler_to_quaternion(yaw_rad)
            target_pose.pose.orientation.x = qx
            target_pose.pose.orientation.y = qy
            target_pose.pose.orientation.z = qz
            target_pose.pose.orientation.w = qw

            print(f"\n[TIẾN ĐỘ {step_idx}/{len(queue_list)}] Đang điều hướng đến Phòng {room_num} (x={target_info['x']}, y={target_info['y']})...")
            navigator.goToPose(target_pose)

            while not navigator.isTaskComplete():
                feedback = navigator.getFeedback()
                # Có thể in tiến độ khoảng cách còn lại nếu muốn
                time.sleep(0.5)

            result = navigator.getResult()
            if result == TaskResult.SUCCEEDED:
                print(f"[THÀNH CÔNG] Robot đã đến đích Phòng {room_num} an toàn!")
                print(f"[CHỜ] Dừng {self.dwell_time_sec} giây trước khi chuyển phòng tiếp theo...")
                time.sleep(self.dwell_time_sec)
            else:
                print(f"[THẤT BẠI] Không thể tiếp cận Phòng {room_num}! Mã kết quả: {result}")
                break

        print("\n[HOÀN TẤT] Robot đã thực hiện xong toàn bộ danh sách phòng trong hàng đợi!")
        rclpy.shutdown()


def main():
    parser = argparse.ArgumentParser(description="AMR Multi-Room Queue Mission Dispatcher")
    parser.add_argument('--queue', nargs='+', type=int, default=[1, 4, 3, 6, 5, 2],
                        help="Danh sách số phòng cần đi theo thứ tự, ví dụ: --queue 1 4 3 6 5 2")
    parser.add_argument('--config', type=str, default="rooms_config.yaml",
                        help="Đường dẫn file cấu hình phòng")
    parser.add_argument('--dwell', type=float, default=5.0,
                        help="Thời gian dừng chờ tại mỗi phòng (giây)")

    args = parser.parse_args()
    dispatcher = QueueMissionDispatcher(config_path=args.config, dwell_time_sec=args.dwell)
    dispatcher.execute_queue(args.queue)


if __name__ == '__main__':
    main()
