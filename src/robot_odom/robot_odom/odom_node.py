#!/usr/bin/env python3
import math
import struct
import time

from geometry_msgs.msg import Quaternion, TransformStamped, Twist
from nav_msgs.msg import Odometry
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import BatteryState, Imu
import serial
from tf2_ros import TransformBroadcaster

# --- Telemetry Packet (STM32 -> Pi: 21 bytes) ---
HEADER = b'\xAA\x55'
TELEMETRY_MSG_ID = 0x01
TELEMETRY_PAYLOAD_LEN = 16
TELEMETRY_PACKET_LEN = 4 + TELEMETRY_PAYLOAD_LEN + 1
TELEMETRY_PAYLOAD = struct.Struct('<iiff')

# --- Command Packet (Pi -> STM32: 13 bytes) --- 
COMMAND_MSG_ID = 0x02
COMMAND_PAYLOAD_LEN = 8
COMMAND_PACKET_LEN = 4 + COMMAND_PAYLOAD_LEN + 1
COMMAND_PAYLOAD = struct.Struct('<ff')


def quaternion_from_yaw(yaw: float) -> Quaternion:
    return Quaternion(
        x=0.0,
        y=0.0,
        z=math.sin(yaw * 0.5),
        w=math.cos(yaw * 0.5),
    )


class BridgeOdom(Node):
    def __init__(self):
        super().__init__('odom')

        self.declare_parameter('port', '/dev/ttyUSB1')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('wheel_radius', 0.033)
        self.declare_parameter('wheel_separation', 0.193)
        self.declare_parameter('ticks_per_rev', 1980.0)
        self.declare_parameter('frame_id', 'odom')
        self.declare_parameter('child_frame_id', 'base_link')
        self.declare_parameter('imu_frame_id', 'imu_link')
        self.declare_parameter('publish_hz', 50.0)
        self.declare_parameter('broadcast_tf', False)
        self.declare_parameter('cmd_vel_timeout', 0.5)

        self.port = self.get_parameter('port').value
        baud = int(self.get_parameter('baud').value)
        self.radius = float(self.get_parameter('wheel_radius').value)
        self.separation = float(self.get_parameter('wheel_separation').value)
        self.ticks_per_rev = float(self.get_parameter('ticks_per_rev').value)
        self.frame_id = self.get_parameter('frame_id').value
        self.child_frame_id = self.get_parameter('child_frame_id').value
        self.imu_frame_id = self.get_parameter('imu_frame_id').value
        self.broadcast_tf = bool(self.get_parameter('broadcast_tf').value)
        self.cmd_vel_timeout = float(self.get_parameter('cmd_vel_timeout').value)
        publish_hz = float(self.get_parameter('publish_hz').value)

        if (
            self.radius <= 0.0
            or self.separation <= 0.0
            or self.ticks_per_rev <= 0.0
            or publish_hz <= 0.0
        ):
            raise ValueError(
                'wheel_radius, wheel_separation, ticks_per_rev, and publish_hz '
                'must all be greater than zero'
            )

        self.period = 1.0 / publish_hz
        try:
            self.ser = serial.Serial(self.port, baud, timeout=0)
        except serial.SerialException as exc:
            self.get_logger().fatal(
                f'Could not open STM32 serial port {self.port}: {exc}'
            )
            raise

        time.sleep(2.0)
        self.get_logger().info(
            f'Serial opened: {self.port} @ {baud}; bidirectional STM32 link ready'
        )

        self.rx_buffer = bytearray()
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.prev_left = None
        self.prev_right = None
        self.prev_time = self.get_clock().now()

        # Command & Watchdog
        self.target_linear_x = 0.0
        self.target_angular_z = 0.0
        self.last_cmd_time = self.get_clock().now()

        # Publishers
        self.odom_pub = self.create_publisher(Odometry, 'odom', 10)
        self.imu_pub = self.create_publisher(Imu, 'imu/data_raw', 10)
        self.battery_pub = self.create_publisher(BatteryState, 'battery_state', 10)
        self.tf_broadcaster = TransformBroadcaster(self)

        # Subscriber
        self.cmd_vel_sub = self.create_subscription(
            Twist, 'cmd_vel', self.cmd_vel_callback, 10
        )

        self.timer = self.create_timer(self.period, self.loop)
        self.get_logger().info('Odom & Hardware Bridge is ready')

    def cmd_vel_callback(self, msg: Twist):
        self.target_linear_x = float(msg.linear.x)
        self.target_angular_z = float(msg.angular.z)
        self.last_cmd_time = self.get_clock().now()

    def _pack_command(self, cmd_v: float, cmd_omega: float) -> bytes:
        payload = COMMAND_PAYLOAD.pack(cmd_v, cmd_omega)
        checksum = (COMMAND_MSG_ID + COMMAND_PAYLOAD_LEN + sum(payload)) & 0xFF
        return HEADER + bytes([COMMAND_MSG_ID, COMMAND_PAYLOAD_LEN]) + payload + bytes([checksum])

    def _send_command(self):
        # Watchdog: dừng robot nếu không nhận được cmd_vel quá thời gian quy định
        now = self.get_clock().now()
        dt_cmd = (now - self.last_cmd_time).nanoseconds * 1e-9
        if dt_cmd > self.cmd_vel_timeout:
            self.target_linear_x = 0.0
            self.target_angular_z = 0.0

        pkt = self._pack_command(self.target_linear_x, self.target_angular_z)
        try:
            self.ser.write(pkt)
        except serial.SerialException as exc:
            self.get_logger().warning(f'Serial write error: {exc}')

    def _read_latest_telemetry(self):
        waiting = self.ser.in_waiting
        if waiting:
            self.rx_buffer.extend(self.ser.read(waiting))

        latest = None
        while True:
            header_index = self.rx_buffer.find(HEADER)
            if header_index < 0:
                keep = 1 if self.rx_buffer.endswith(HEADER[:1]) else 0
                if keep:
                    self.rx_buffer[:] = self.rx_buffer[-1:]
                else:
                    self.rx_buffer.clear()
                break

            if header_index:
                del self.rx_buffer[:header_index]

            if len(self.rx_buffer) < 4:
                break

            msg_id = self.rx_buffer[2]
            payload_len = self.rx_buffer[3]
            if msg_id != TELEMETRY_MSG_ID or payload_len != TELEMETRY_PAYLOAD_LEN:
                del self.rx_buffer[0]
                continue

            if len(self.rx_buffer) < TELEMETRY_PACKET_LEN:
                break

            checksum_index = 4 + payload_len
            expected_checksum = sum(self.rx_buffer[2:checksum_index]) & 0xFF
            if self.rx_buffer[checksum_index] != expected_checksum:
                del self.rx_buffer[0]
                continue

            left_ticks, right_ticks, gyro_z, battery_voltage = (
                TELEMETRY_PAYLOAD.unpack_from(self.rx_buffer, 4)
            )
            latest = (left_ticks, right_ticks, gyro_z, battery_voltage)
            del self.rx_buffer[:TELEMETRY_PACKET_LEN]

        return latest

    def loop(self):
        # 1. Gửi lệnh điều khiển định kỳ xuống STM32 (kèm cơ chế an toàn watchdog)
        self._send_command()

        # 2. Đọc và giải mã dữ liệu cảm biến từ STM32
        try:
            latest = self._read_latest_telemetry()
        except serial.SerialException as exc:
            self.get_logger().warning(f'Serial read error: {exc}')
            return

        if latest is None:
            return

        left_now, right_now, gyro_z, battery_voltage = latest
        now = self.get_clock().now()

        # 3. Xuất bản IMU (đổi gyro_z từ dps sang rad/s)
        imu_msg = Imu()
        imu_msg.header.stamp = now.to_msg()
        imu_msg.header.frame_id = self.imu_frame_id
        imu_msg.angular_velocity.z = math.radians(gyro_z)
        # Covariance cho góc quay quanh trục Z (chỉ số 8) để EKF dung hợp
        imu_msg.angular_velocity_covariance[8] = 0.02
        self.imu_pub.publish(imu_msg)

        # 4. Xuất bản thông tin pin (quy đổi % cho pin 3S: 10.5V -> 12.4V)
        bat_msg = BatteryState()
        bat_msg.header.stamp = now.to_msg()
        bat_msg.voltage = float(battery_voltage)
        pct = (battery_voltage - 10.5) / 2.1
        bat_msg.percentage = float(max(0.0, min(1.0, pct)))
        self.battery_pub.publish(bat_msg)

        # 5. Tính toán Odometry
        if self.prev_left is None:
            self.prev_left = left_now
            self.prev_right = right_now
            self.prev_time = now
            return

        dt = (now - self.prev_time).nanoseconds * 1e-9
        if dt <= 0.0:
            dt = self.period

        delta_left = left_now - self.prev_left
        delta_right = right_now - self.prev_right
        self.prev_left = left_now
        self.prev_right = right_now
        self.prev_time = now

        meters_per_tick = (2.0 * math.pi * self.radius) / self.ticks_per_rev
        distance_left = delta_left * meters_per_tick
        distance_right = delta_right * meters_per_tick
        distance = 0.5 * (distance_left + distance_right)
        delta_theta = (distance_right - distance_left) / self.separation

        self.x += distance * math.cos(self.theta + 0.5 * delta_theta)
        self.y += distance * math.sin(self.theta + 0.5 * delta_theta)
        self.theta = (self.theta + delta_theta + math.pi) % (2.0 * math.pi) - math.pi

        odom = Odometry()
        odom.header.stamp = now.to_msg()
        odom.header.frame_id = self.frame_id
        odom.child_frame_id = self.child_frame_id
        odom.pose.pose.position.x = self.x
        odom.pose.pose.position.y = self.y
        odom.pose.pose.position.z = 0.0
        odom.pose.pose.orientation = quaternion_from_yaw(self.theta)
        odom.pose.covariance[0] = 0.01
        odom.pose.covariance[7] = 0.01
        odom.pose.covariance[35] = 0.05
        odom.twist.twist.linear.x = distance / dt
        odom.twist.twist.linear.y = 0.0
        odom.twist.twist.angular.z = delta_theta / dt
        odom.twist.covariance[0] = 0.02
        odom.twist.covariance[35] = 0.05
        self.odom_pub.publish(odom)

        if self.broadcast_tf:
            transform = TransformStamped()
            transform.header.stamp = now.to_msg()
            transform.header.frame_id = self.frame_id
            transform.child_frame_id = self.child_frame_id
            transform.transform.translation.x = self.x
            transform.transform.translation.y = self.y
            transform.transform.translation.z = 0.0
            transform.transform.rotation = quaternion_from_yaw(self.theta)
            self.tf_broadcaster.sendTransform(transform)

    def destroy_node(self):
        if hasattr(self, 'ser') and self.ser.is_open:
            self.ser.close()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        node = BridgeOdom()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
