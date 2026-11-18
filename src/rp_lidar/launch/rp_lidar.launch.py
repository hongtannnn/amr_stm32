from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    serial_port_arg = DeclareLaunchArgument(
        'serial_port', default_value='/dev/ttyUSB0',
        description='Serial device for RPLIDAR A1M8'
    )
    serial_baudrate_arg = DeclareLaunchArgument(
        'serial_baudrate', default_value='115200',
        description='Serial baudrate for RPLIDAR A1M8'
    )
    frame_id_arg = DeclareLaunchArgument(
        'frame_id', default_value='laser',
        description='TF frame ID for the laser scan'
    )
    inverted_arg = DeclareLaunchArgument(
        'inverted', default_value='false',
        description='Invert the scan direction'
    )
    angle_compensate_arg = DeclareLaunchArgument(
        'angle_compensate', default_value='true',
        description='Enable angle compensation'
    )

    serial_port = LaunchConfiguration('serial_port')
    serial_baudrate = LaunchConfiguration('serial_baudrate')
    frame_id = LaunchConfiguration('frame_id')
    inverted = LaunchConfiguration('inverted')
    angle_compensate = LaunchConfiguration('angle_compensate')

    return LaunchDescription([
        serial_port_arg,
        serial_baudrate_arg,
        frame_id_arg,
        inverted_arg,
        angle_compensate_arg,

        Node(
            package='rplidar_ros',
            executable='rplidar_composition',
            name='rplidar_composition',
            output='screen',
            parameters=[{
                'serial_port': serial_port,
                'serial_baudrate': serial_baudrate,
                'frame_id': frame_id,
                'inverted': inverted,
                'angle_compensate': angle_compensate,
            }]
        ),
    ])




