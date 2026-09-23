import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription, SetEnvironmentVariable, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import xacro


def generate_launch_description():

    pkg_path = get_package_share_directory('my_bot')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')

    # Cho Gazebo Harmonic tìm được mesh
    gz_resource_path = SetEnvironmentVariable(
        name='GZ_SIM_RESOURCE_PATH',
        value=os.path.join(pkg_path, '..'),
    )

    # Process xacro → URDF
    xacro_file = os.path.join(pkg_path, 'description', 'urdf', 'robot_assembly.xacro')
    robot_description_config = xacro.process_file(xacro_file).toxml()

    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    world_file = os.path.join(pkg_path, 'worlds', 'empty.world')

    # 1. Gazebo Harmonic
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': world_file}.items(),
    )

    # 2. robot_state_publisher
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description_config,
            'use_sim_time': use_sim_time,
        }]
    )

    joint_state_publisher = Node(
        package='joint_state_publisher',
        executable='joint_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description_config,
            'use_sim_time': use_sim_time,
        }]
    )

    # 3. Spawn robot
    spawn_robot = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=[
            '-name', 'my_bot',
            '-topic', 'robot_description',
            '-x', '0.0',
            '-y', '0.0',
            '-z', '0.07',
        ],
        output='screen',
    )

    # 4. Bridge: /cmd_vel (ROS2→Gz) và /odom + /tf (Gz→ROS2)
    bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
            '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
            '/tf@tf2_msgs/msg/TFMessage[gz.msgs.Pose_V',
            '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
        ],
        output='screen',
    )

    # 5. Gửi lệnh dừng ngay sau khi bridge sẵn sàng (tránh lệnh cũ còn sót)
    stop_robot = TimerAction(
        period=2.0,
        actions=[
            ExecuteProcess(
                cmd=['ros2', 'topic', 'pub', '--once', '/cmd_vel',
                     'geometry_msgs/msg/Twist',
                     '{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}'],
                output='screen',
            )
        ]
    )

    # 5b. Reset pose robot về tọa độ gốc (0, 0, 0.07)
    reset_pose = TimerAction(
        period=2.5,
        actions=[
            ExecuteProcess(
                cmd=['gz', 'service', '-s', '/world/default/set_pose',
                     '--reqtype', 'gz.msgs.Pose',
                     '--reptype', 'gz.msgs.Boolean',
                     '--timeout', '1000',
                     '--req',
                     'name: "my_bot", position: {x: 0.0, y: 0.0, z: 0.07}, '
                     'orientation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0}'],
                output='screen',
            )
        ]
    )

    # 6a. Relay node: đảo angular.z từ /cmd_vel_raw → /cmd_vel
    cmd_vel_relay = Node(
        package='my_bot',
        executable='cmd_vel_relay.py',
        name='cmd_vel_relay',
        output='screen',
    )

    # 6b. Teleop twist keyboard (mở terminal mới, delay 3s chờ Gazebo khởi động)
    teleop = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='teleop_twist_keyboard',
                executable='teleop_twist_keyboard',
                name='teleop_twist_keyboard',
                output='screen',
                prefix='xterm -e',
                remappings=[('/cmd_vel', '/cmd_vel_raw')],
            )
        ]
    )

    return LaunchDescription([
        gz_resource_path,
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use sim time if true',
        ),
        gz_sim,
        robot_state_publisher,
        joint_state_publisher,
        spawn_robot,
        bridge,
        stop_robot,
        reset_pose,
        cmd_vel_relay,
        teleop,
    ])
