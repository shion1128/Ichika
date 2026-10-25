import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, TextSubstitution
from launch_ros.actions import Node
from launch_ros.descriptions import ParameterFile
from nav2_common.launch import RewrittenYaml

def generate_launch_description():

    serial_hardware_bridge_dir = get_package_share_directory('serial_hardware_bridge')

    # 2. 构造 IncludeLaunchDescription 对象
    start_serial_hardware_bridge_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(serial_hardware_bridge_dir, 'launch', 'serial_hardware_bridge_launch.py'))
    )
    
    # Create the launch description and populate
    ld = LaunchDescription()

    # Set environment variables


    # Declare the launch options
    ld.add_action(start_serial_hardware_bridge_cmd)

    
    return ld


