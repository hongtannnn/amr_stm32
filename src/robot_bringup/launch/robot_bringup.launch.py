import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    rp_lidar_dir = get_package_share_directory('rp_lidar')
    robot_desc_dir = get_package_share_directory('robot_description')
    robot_odom_dir = get_package_share_directory('robot_odom')
    robot_bringup_dir = get_package_share_directory('robot_bringup')
    ekf_config_path = os.path.join(robot_bringup_dir, 'config', 'ekf.yaml')

    return LaunchDescription([
        # 1. RPLidar
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(rp_lidar_dir, 'launch', 'rp_lidar.launch.py')
            ),
            launch_arguments={'serial_port': '/dev/ttyUSB0'}.items()
        ),
        # 2. Robot Description (URDF & State Publisher)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(robot_desc_dir, 'launch', 'display.launch.py')
            )
        ),
        # 3. STM32 Odom & Hardware Bridge (Tắt phát TF odom -> base_link để EKF phát)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(robot_odom_dir, 'launch', 'odom.launch.py')
            ),
            launch_arguments={
                'port': '/dev/ttyAMA0',
                'baud': '115200',
                'broadcast_tf': 'false'
            }.items()
        ),
        # 4. Robot Localization (EKF: dung hợp Wheel Odom + IMU và phát TF odom -> base_link)
        Node(
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node',
            output='screen',
            parameters=[ekf_config_path],
        ),
    ])
