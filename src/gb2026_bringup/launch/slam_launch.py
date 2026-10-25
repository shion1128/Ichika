import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument,GroupAction,SetEnvironmentVariable,IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration,PythonExpression
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node,LoadComposableNodes,PushRosNamespace
from launch_ros.descriptions import ParameterFile
from nav2_common.launch import RewrittenYaml
from launch.conditions import IfCondition 
from launch_ros.descriptions import ComposableNode, ParameterFile


def generate_launch_description():
    # Getting directories and launch-files
    bringup_dir = get_package_share_directory("gb2026_bringup")

    robot_description_dir = get_package_share_directory("gb2026_robot_description")

    # livox_dir = get_package_share_directory("livox_ros_driver2")

    # Input parameters declaration
    namespace = LaunchConfiguration("namespace")
    params_file = LaunchConfiguration("params_file")
    use_composition = LaunchConfiguration("use_composition")
    container_name = LaunchConfiguration("container_name")
    container_name_full = (namespace, "/", container_name)
    use_sim_time = LaunchConfiguration("use_sim_time")
    use_respawn = LaunchConfiguration("use_respawn")
    log_level = LaunchConfiguration("log_level")
    loam_type = LaunchConfiguration("loam_type")

    
    # Create our own temporary YAML files that include substitutions
    param_substitutions = {"use_sim_time": use_sim_time}

    configured_params = ParameterFile(
        RewrittenYaml(
            source_file=params_file,
            root_key=namespace,
            param_rewrites=param_substitutions,
            convert_types=True,
        ),
        allow_substs=True,
    )

    stdout_linebuf_envvar = SetEnvironmentVariable(
        "RCUTILS_LOGGING_BUFFERED_STREAM", "1"
    )

    colorized_output_envvar = SetEnvironmentVariable("RCUTILS_COLORIZED_OUTPUT", "1")

    # Declare the launch arguments
    declare_namespace_cmd = DeclareLaunchArgument(
        "namespace", default_value="", description="Top-level namespace"
    )

    declare_use_sim_time_cmd = DeclareLaunchArgument(
        "use_sim_time",
        default_value="false",
        description="Use simulation (Gazebo) clock if true",
    )

    declare_params_file_cmd = DeclareLaunchArgument(
        "params_file",
        default_value=os.path.join(bringup_dir, "config", "params.yaml"),
        description="Full path to the ROS2 parameters file to use for all launched nodes",
    )

    declare_use_composition_cmd = DeclareLaunchArgument(
        "use_composition",
        default_value="True",
        description="Use composed bringup if True",
    )

    declare_container_name_cmd = DeclareLaunchArgument(
        "container_name",
        default_value="nav2_container",
        description="the name of container that nodes will load in if use composition",
    )


    declare_use_respawn_cmd = DeclareLaunchArgument(
        "use_respawn",
        default_value="False",
        description="Whether to respawn if a node crashes. Applied when composition is disabled.",
    )

    declare_log_level_cmd = DeclareLaunchArgument(
        "log_level", default_value="info", description="log level"
    )

    declare_loam_type_cmd = DeclareLaunchArgument(
        "loam_type",default_value="point_lio",description="slam type"
    )

    # start_livox_cmd = IncludeLaunchDescription(
    #     PythonLaunchDescriptionSource(os.path.join(livox_dir, 'launch', 'msg_MID360_launch.py'))
    # )

    start_livox_ros_driver2_node = Node(
        package="livox_ros_driver2",
        executable="livox_ros_driver2_node",
        name="livox_ros_driver2",
        output="screen",
        namespace=namespace,
        parameters=[configured_params],
    )

    point_lio_node_cmd = Node(
        condition=IfCondition(PythonExpression(["'", loam_type, "' == 'point_lio'"])),
        package="point_lio",
        executable="pointlio_mapping",
        name="point_lio",
        output="screen",
        respawn=use_respawn,
        respawn_delay=2.0,
        parameters=[configured_params,
                    {"pcd_save.pcd_save_en": True},
            ],
        arguments=["--ros-args", "--log-level", log_level],
    )

    fast_lio_node_cmd = Node(
        condition=IfCondition(PythonExpression(["'", loam_type, "' == 'fast_lio'"])),
        package="fast_lio",
        executable="fastlio_mapping",
        name="fast_lio",
        output="screen",
        respawn=use_respawn,
        respawn_delay=2.0,
        parameters=[configured_params,
                    {"pcd_save.pcd_save_en": True,},
            ],
        arguments=["--ros-args", "--log-level", log_level],
    )

    start_robot_description_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(robot_description_dir, 'launch', 'robot_description_launch.py'))
    )


    load_nodes = GroupAction(
        actions=[
            Node(
                package="loam_interface",
                executable="loam_interface_node",
                name="loam_interface",
                output="screen",
                respawn=use_respawn,
                respawn_delay=2.0,
                parameters=[configured_params],
                arguments=["--ros-args", "--log-level", log_level],
            ),
            Node(
                package="sensor_scan_generation",
                executable="sensor_scan_generation_node",
                name="sensor_scan_generation",
                output="screen",
                respawn=use_respawn,
                respawn_delay=2.0,
                parameters=[configured_params],
                arguments=["--ros-args", "--log-level", log_level],
            ),
        ]
    )


    load_composable_nodes = LoadComposableNodes(
        condition=IfCondition(use_composition),
        target_container=container_name_full,
        composable_node_descriptions=[
            ComposableNode(
                package="loam_interface",
                plugin="loam_interface::LoamInterfaceNode",
                name="loam_interface",
                parameters=[configured_params],
            ),
            ComposableNode(
                package="sensor_scan_generation",
                plugin="sensor_scan_generation::SensorScanGenerationNode",
                name="sensor_scan_generation",
                parameters=[configured_params],
            ),
        ]
    )
    
    # Create the launch description and populate
    ld = LaunchDescription()

    # Set environment variables
    ld.add_action(stdout_linebuf_envvar)
    ld.add_action(colorized_output_envvar)

    # Declare the launch options
    ld.add_action(declare_namespace_cmd)
    ld.add_action(declare_use_respawn_cmd)
    ld.add_action(declare_log_level_cmd)
    ld.add_action(declare_loam_type_cmd)
    ld.add_action(declare_use_sim_time_cmd)
    ld.add_action(declare_params_file_cmd)
    ld.add_action(declare_use_composition_cmd)
    ld.add_action(declare_container_name_cmd)

    # Add the actions to launch all of the navigation nodes
    ld.add_action(start_livox_ros_driver2_node)
    ld.add_action(point_lio_node_cmd)
    ld.add_action(fast_lio_node_cmd)
    ld.add_action(start_robot_description_cmd)
    ld.add_action(load_composable_nodes)
    ld.add_action(load_nodes)

    return ld


