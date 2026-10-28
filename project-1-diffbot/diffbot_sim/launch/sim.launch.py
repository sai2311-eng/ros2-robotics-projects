import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    pkg_share = get_package_share_directory('diffbot_sim')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')

    xacro_file = os.path.join(pkg_share, 'urdf', 'diffbot.urdf.xacro')
    world_file = os.path.join(pkg_share, 'worlds', 'simple_world.sdf')
    bridge_config = os.path.join(pkg_share, 'config', 'bridge.yaml')
    rviz_config = os.path.join(pkg_share, 'config', 'diffbot.rviz')

    robot_description = ParameterValue(
        Command(['xacro ', xacro_file]), value_type=str
    )

    # 1. Gazebo (server + GUI) with our world, running immediately
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': f'-r {world_file}'}.items(),
    )

    # 2. Publishes /robot_description and the TF for fixed joints and wheels
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description,
                     'use_sim_time': True}],
    )

    # 3. Spawn the robot into the running world (delayed so Gazebo is ready)
    spawn = TimerAction(
        period=5.0,
        actions=[Node(
            package='ros_gz_sim',
            executable='create',
            arguments=['-world', 'simple_world',
                       '-name', 'diffbot',
                       '-topic', 'robot_description',
                       '-z', '0.05'],
            output='screen',
        )],
    )

    # 4. Bridge Gazebo topics <-> ROS 2 topics
    bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        parameters=[{'config_file': bridge_config,
                     'use_sim_time': True}],
        output='screen',
    )

    # 5. RViz2
    rviz = Node(
        package='rviz2',
        executable='rviz2',
        arguments=['-d', rviz_config],
        parameters=[{'use_sim_time': True}],
    )

    return LaunchDescription([
        gz_sim,
        robot_state_publisher,
        spawn,
        bridge,
        rviz,
    ])
