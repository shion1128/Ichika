from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():

    start_serial_hardware_bridge = Node(
        package="serial_hardware_bridge",
        executable="serial_hardware_bridge_node",
        name="serial_hardware_bridge",
        output="screen",
    )

    ld = LaunchDescription()

    # Add the actions
    
    ld.add_action(start_serial_hardware_bridge)

    return ld




