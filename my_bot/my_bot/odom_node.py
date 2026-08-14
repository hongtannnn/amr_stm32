#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32MultiArray
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TransformStamped
from tf2_ros import TransformBroadcaster
import math

# Robot parameters
WHEEL_DIAMETER = 0.066   # m
WHEEL_BASE     = 0.192   # m
ENCODER_PPR    = 1960    # xung/vòng (full quadrature)
WHEEL_CIRCUM   = math.pi * WHEEL_DIAMETER


class OdomNode(Node):
    def __init__(self):
        super().__init__('odom_node')

        self.x     = 0.0
        self.y     = 0.0
        self.theta = 0.0

        self.prev_enc1 = None
        self.prev_enc2 = None

        self.sub = self.create_subscription(
            Int32MultiArray,
            '/wheel_ticks',
            self.wheel_ticks_callback,
            10
        )

        self.odom_pub = self.create_publisher(Odometry, '/odom', 10)
        self.tf_broadcaster = TransformBroadcaster(self)

        self.get_logger().info('odom_node started.')

    def wheel_ticks_callback(self, msg):
        enc1 = msg.data[0]
        enc2 = msg.data[1]

        if self.prev_enc1 is None:
            self.prev_enc1 = enc1
            self.prev_enc2 = enc2
            return

        d_enc1 = enc1 - self.prev_enc1
        d_enc2 = enc2 - self.prev_enc2
        self.prev_enc1 = enc1
        self.prev_enc2 = enc2

        # Quãng đường mỗi bánh (m)
        d_left  = (d_enc1 / ENCODER_PPR) * WHEEL_CIRCUM
        d_right = (d_enc2 / ENCODER_PPR) * WHEEL_CIRCUM

        # Tính odometry
        d_center = (d_left + d_right) / 2.0
        d_theta  = (d_right - d_left) / WHEEL_BASE

        self.x     += d_center * math.cos(self.theta + d_theta / 2.0)
        self.y     += d_center * math.sin(self.theta + d_theta / 2.0)
        self.theta += d_theta

        now = self.get_clock().now().to_msg()

        qz = math.sin(self.theta / 2.0)
        qw = math.cos(self.theta / 2.0)

        # Publish /odom
        odom = Odometry()
        odom.header.stamp            = now
        odom.header.frame_id         = 'odom'
        odom.child_frame_id          = 'base_link'
        odom.pose.pose.position.x    = self.x
        odom.pose.pose.position.y    = self.y
        odom.pose.pose.orientation.z = qz
        odom.pose.pose.orientation.w = qw
        self.odom_pub.publish(odom)

        # Broadcast TF odom → base_link
        tf = TransformStamped()
        tf.header.stamp            = now
        tf.header.frame_id         = 'odom'
        tf.child_frame_id          = 'base_link'
        tf.transform.translation.x = self.x
        tf.transform.translation.y = self.y
        tf.transform.rotation.z    = qz
        tf.transform.rotation.w    = qw
        self.tf_broadcaster.sendTransform(tf)


def main(args=None):
    rclpy.init(args=args)
    node = OdomNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
