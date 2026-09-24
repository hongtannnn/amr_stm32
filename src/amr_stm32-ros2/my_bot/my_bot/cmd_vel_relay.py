#!/usr/bin/env python3
"""Relay node: đảo dấu angular.z từ /cmd_vel_raw sang /cmd_vel."""
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist


class CmdVelRelay(Node):
    def __init__(self):
        super().__init__('cmd_vel_relay')
        self.pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.sub = self.create_subscription(Twist, '/cmd_vel_raw', self.callback, 10)

    def callback(self, msg: Twist):
        out = Twist()
        out.linear.x = msg.linear.x
        out.linear.y = msg.linear.y
        out.linear.z = msg.linear.z
        out.angular.x = msg.angular.x
        out.angular.y = msg.angular.y
        out.angular.z = msg.angular.z  # đảo chiều quay
        self.pub.publish(out)


def main():
    rclpy.init()
    node = CmdVelRelay()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
