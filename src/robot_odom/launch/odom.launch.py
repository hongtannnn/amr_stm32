from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    port = LaunchConfiguration('port')
    baud = LaunchConfiguration('baud')
    broadcast_tf = LaunchConfiguration('broadcast_tf')

    return LaunchDescription([
        DeclareLaunchArgument(
            'port',
            default_value='/dev/ttyUSB1',
            description='Serial device connected to the wheel encoder controller',
        ),
        DeclareLaunchArgument(
            'baud',
            default_value='115200',
            description='Serial baud rate for the wheel encoder controller',
        ),
        DeclareLaunchArgument(
            'broadcast_tf',
            default_value='false',
            description='Whether to broadcast odom -> base_link TF (set false when using EKF)',
        ),
        Node(
            package='robot_odom',
            executable='odom_node',
            name='odom',
            output='screen',
            parameters=[{
                'port': port,
                'baud': baud,
                'broadcast_tf': broadcast_tf,
            }],
        ),
    ])
