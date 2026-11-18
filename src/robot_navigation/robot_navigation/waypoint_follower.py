#!/usr/bin/env python3
"""Tuần tra qua các điểm Waypoints định trước sử dụng Nav2 FollowWaypoints."""

import math
import sys
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from geometry_msgs.msg import PoseStamped
from nav2_msgs.action import FollowWaypoints
from action_msgs.msg import GoalStatus


def euler_to_quaternion(yaw: float):
    return (0.0, 0.0, math.sin(yaw / 2.0), math.cos(yaw / 2.0))


class WaypointFollowerClient(Node):
    def __init__(self):
        super().__init__('waypoint_follower_client')
        self._action_client = ActionClient(self, FollowWaypoints, 'follow_waypoints')

    def send_waypoints(self, waypoints):
        self.get_logger().info('Đang chờ FollowWaypoints action server...')
        if not self._action_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('Không thể kết nối đến action server "follow_waypoints"!')
            return False

        goal_msg = FollowWaypoints.Goal()
        for x, y, yaw in waypoints:
            pose = PoseStamped()
            pose.header.frame_id = 'map'
            pose.header.stamp = self.get_clock().now().to_msg()
            pose.pose.position.x = float(x)
            pose.pose.position.y = float(y)
            pose.pose.position.z = 0.0
            qx, qy, qz, qw = euler_to_quaternion(yaw)
            pose.pose.orientation.x = qx
            pose.pose.orientation.y = qy
            pose.pose.orientation.z = qz
            pose.pose.orientation.w = qw
            goal_msg.poses.append(pose)

        self.get_logger().info(f'Bắt đầu gửi lộ trình gồm {len(waypoints)} điểm...')
        send_goal_future = self._action_client.send_goal_async(
            goal_msg, feedback_callback=self._feedback_callback
        )
        send_goal_future.add_done_callback(self._goal_response_callback)
        return True

    def _goal_response_callback(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().warn('Lộ trình bị từ chối!')
            return

        self.get_logger().info('Lộ trình đã được chấp nhận, robot bắt đầu tuần tra...')
        get_result_future = goal_handle.get_result_async()
        get_result_future.add_done_callback(self._get_result_callback)

    def _feedback_callback(self, feedback_msg):
        current_wp = feedback_msg.feedback.current_waypoint
        self.get_logger().info(f'Đang hướng tới waypoint chỉ số: {current_wp}', throttle_duration_sec=3.0)

    def _get_result_callback(self, future):
        status = future.result().status
        if status == GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().info('ĐÃ HOÀN THÀNH TOÀN BỘ LỘ TRÌNH WAYPOINTS!')
        else:
            self.get_logger().warn(f'Tuần tra dừng lại với mã trạng thái: {status}')
        rclpy.shutdown()


def main(args=None):
    rclpy.init(args=args)
    client = WaypointFollowerClient()

    # Danh sách các điểm tuần tra mẫu: [(x, y, yaw_rad), ...]
    default_waypoints = [
        (1.0, 0.0, 0.0),
        (1.5, 1.0, 1.57),
        (0.0, 1.0, 3.14),
        (0.0, 0.0, 0.0),
    ]

    client.send_waypoints(default_waypoints)
    try:
        rclpy.spin(client)
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
