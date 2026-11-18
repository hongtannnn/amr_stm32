import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    desc_pkg = get_package_share_directory('robot_description')
    odom_pkg = get_package_share_directory('robot_odom')
    lidar_pkg = get_package_share_directory('rp_lidar')
    nav_pkg = get_package_share_directory('robot_navigation')

    default_map = os.path.join(nav_pkg, 'maps', 'map.yaml')

    map_arg = DeclareLaunchArgument(
        'map',
        default_value=default_map,
        description='Full path to map yaml file to load'
    )
    odom_port_arg = DeclareLaunchArgument(
        'odom_port',
        default_value='/dev/ttyUSB1',
        description='Serial port for STM32 odom bridge'
    )
    lidar_port_arg = DeclareLaunchArgument(
        'lidar_port',
        default_value='/dev/ttyUSB0',
        description='Serial port for RPLidar'
    )
    use_rviz_arg = DeclareLaunchArgument(
        'use_rviz',
        default_value='true',
        description='Whether to start RViz'
    )

    map_file = LaunchConfiguration('map')
    odom_port = LaunchConfiguration('odom_port')
    lidar_port = LaunchConfiguration('lidar_port')
    use_rviz = LaunchConfiguration('use_rviz')

    # 1. Robot Description (URDF + TF)
    desc_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(desc_pkg, 'launch', 'display.launch.py')
        )
    )

    # 2. Robot Odometry & Motor Driver bridge
    odom_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(odom_pkg, 'launch', 'odom.launch.py')
        ),
        launch_arguments={'port': odom_port}.items(),
    )

    # 3. RPLidar
    lidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(lidar_pkg, 'launch', 'rp_lidar.launch.py')
        ),
        launch_arguments={'serial_port': lidar_port}.items(),
    )

    # 4. Navigation Stack 2
    nav_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(nav_pkg, 'launch', 'navigation.launch.py')
        ),
        launch_arguments={
            'map': map_file,
            'use_rviz': use_rviz,
        }.items(),
    )

    return LaunchDescription([
        map_arg,
        odom_port_arg,
        lidar_port_arg,
        use_rviz_arg,
        desc_launch,
        odom_launch,
        lidar_launch,
        nav_launch,
    ])
