from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node



def generate_launch_description():

    start_omni_pid_pursuit_controller_node = Node(
        package="gb_omni_pid_pursuit_controller",
        executable="gb_omni_pid_pursuit_controller_node",
        name="gb_omni_pid_pursuit_controller",
        output="screen",

    )

    ld = LaunchDescription()

    # Add the actions
    ld.add_action(start_omni_pid_pursuit_controller_node)

    return ld
