import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    nav_pkg = get_package_share_directory('robot_navigation')
    mapping_pkg = get_package_share_directory('robot_mapping')
    nav2_bringup_pkg = get_package_share_directory('nav2_bringup')

    default_nav2_params = os.path.join(nav_pkg, 'config', 'nav2_params_explore.yaml')
    default_explore_params = os.path.join(nav_pkg, 'config', 'explore.yaml')
    default_rviz = os.path.join(nav_pkg, 'rviz', 'nav2_view.rviz')

    # Arguments
    params_file_arg = DeclareLaunchArgument(
        'params_file',
        default_value=default_nav2_params,
        description='Full path to the Nav2 parameters file to use for exploration'
    )
    explore_params_arg = DeclareLaunchArgument(
        'explore_params_file',
        default_value=default_explore_params,
        description='Full path to the explore_lite parameters file'
    )
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation (Gazebo) clock if true'
    )
    autostart_arg = DeclareLaunchArgument(
        'autostart',
        default_value='true',
        description='Automatically startup the nav2 stack'
    )
    use_rviz_arg = DeclareLaunchArgument(
        'use_rviz',
        default_value='true',
        description='Whether to start RViz'
    )
    launch_slam_arg = DeclareLaunchArgument(
        'launch_slam',
        default_value='false',
        description='Whether to launch SLAM Toolbox directly in this launch file'
    )

    params_file = LaunchConfiguration('params_file')
    explore_params_file = LaunchConfiguration('explore_params_file')
    use_sim_time = LaunchConfiguration('use_sim_time')
    autostart = LaunchConfiguration('autostart')
    use_rviz = LaunchConfiguration('use_rviz')
    launch_slam = LaunchConfiguration('launch_slam')

    # 1. Optional SLAM Toolbox (if launch_slam:=true)
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(mapping_pkg, 'launch', 'slam_toolbox.launch.py')
        ),
        condition=IfCondition(launch_slam),
        launch_arguments={'use_rviz': 'false'}.items(),
    )

    # 2. Pure Nav2 Navigation Stack (no AMCL, no map_server, subscribes directly to live /map from SLAM)
    navigation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(nav2_bringup_pkg, 'launch', 'navigation_launch.py')
        ),
        launch_arguments={
            'params_file': params_file,
            'use_sim_time': use_sim_time,
            'autostart': autostart,
        }.items(),
    )

    # 3. Explore Lite Node
    explore_node = Node(
        package='explore_lite',
        executable='explore',
        name='explore_node',
        output='screen',
        parameters=[explore_params_file, {'use_sim_time': use_sim_time}],
    )

    # 4. RViz2 View
    rviz_node = Node(
        condition=IfCondition(use_rviz),
        package='rviz2',
        executable='rviz2',
        name='rviz2_explore',
        arguments=['-d', default_rviz],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen'
    )

    return LaunchDescription([
        params_file_arg,
        explore_params_arg,
        use_sim_time_arg,
        autostart_arg,
        use_rviz_arg,
        launch_slam_arg,
        slam_launch,
        navigation_launch,
        explore_node,
        rviz_node,
    ])
