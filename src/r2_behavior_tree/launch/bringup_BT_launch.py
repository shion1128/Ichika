import os

from ament_index_python.packages import get_package_share_directory
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.actions import DeclareLaunchArgument, GroupAction, SetEnvironmentVariable
from launch_ros.actions import Node
from launch import LaunchDescription


def generate_launch_description():
    
    loads_nodes =  GroupAction(
        actions=[
            Node(
                package="r2_behavior_tree",
                executable="r2_behavior_tree_node",
                name="r2_behavior_tree",
                output="screen",
            ),
        ]
    )

    ld = LaunchDescription()
    ld.add_action(loads_nodes)

    return ld

