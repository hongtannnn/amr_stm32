#!/usr/bin/env python3
"""Gửi tọa độ mục tiêu (x, y, yaw) đến Nav2 stack."""

import math
import sys
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from geometry_msgs.msg import PoseStamped
from nav2_msgs.action import NavigateToPose
from action_msgs.msg import GoalStatus


def euler_to_quaternion(yaw: float):
    return (0.0, 0.0, math.sin(yaw / 2.0), math.cos(yaw / 2.0))


class NavToPoseClient(Node):
    def __init__(self):
        super().__init__('nav_to_pose_client')
        self._action_client = ActionClient(self, NavigateToPose, 'navigate_to_pose')

    def send_goal(self, x: float, y: float, yaw: float = 0.0):
        self.get_logger().info('Đang chờ Navigation action server...')
        if not self._action_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('Không thể kết nối đến action server "navigate_to_pose"!')
            return False

        goal_msg = NavigateToPose.Goal()
        goal_msg.pose.header.frame_id = 'map'
        goal_msg.pose.header.stamp = self.get_clock().now().to_msg()
        goal_msg.pose.pose.position.x = float(x)
        goal_msg.pose.pose.position.y = float(y)
        goal_msg.pose.pose.position.z = 0.0

        qx, qy, qz, qw = euler_to_quaternion(yaw)
        goal_msg.pose.pose.orientation.x = qx
        goal_msg.pose.pose.orientation.y = qy
        goal_msg.pose.pose.orientation.z = qz
        goal_msg.pose.pose.orientation.w = qw

        self.get_logger().info(f'Đang gửi mục tiêu tới Nav2: X={x:.2f}, Y={y:.2f}, Yaw={yaw:.2f} rad')
        
        send_goal_future = self._action_client.send_goal_async(
            goal_msg, feedback_callback=self._feedback_callback
        )
        send_goal_future.add_done_callback(self._goal_response_callback)
        return True

    def _goal_response_callback(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().warn('Mục tiêu bị từ chối!')
            return

        self.get_logger().info('Mục tiêu đã được chấp nhận, robot bắt đầu di chuyển...')
        get_result_future = goal_handle.get_result_async()
        get_result_future.add_done_callback(self._get_result_callback)

    def _feedback_callback(self, feedback_msg):
        feedback = feedback_msg.feedback
        self.get_logger().info(
            f'Khoảng cách còn lại: {feedback.distance_remaining:.2f} m',
            throttle_duration_sec=2.0
        )

    def _get_result_callback(self, future):
        status = future.result().status
        if status == GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().info('ĐÃ ĐẾN MỤC TIÊU THÀNH CÔNG!')
        else:
            self.get_logger().warn(f'Di chuyển thất bại với mã trạng thái: {status}')
        rclpy.shutdown()


def main(args=None):
    rclpy.init(args=args)
    client = NavToPoseClient()

    # Nhận x, y, yaw từ tham số dòng lệnh nếu có (ví dụ: ros2 run robot_navigation navigate_to_pose 1.0 2.0 0.0)
    target_x = 1.0
    target_y = 1.0
    target_yaw = 0.0

    if len(sys.argv) >= 3:
        target_x = float(sys.argv[1])
        target_y = float(sys.argv[2])
    if len(sys.argv) >= 4:
        target_yaw = float(sys.argv[3])

    client.send_goal(target_x, target_y, target_yaw)
    try:
        rclpy.spin(client)
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
