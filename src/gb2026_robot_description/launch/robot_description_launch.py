import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, Command, PythonExpression
from launch_ros.actions import Node, PushRosNamespace
from launch_ros.descriptions import ParameterFile
from nav2_common.launch import RewrittenYaml

def generate_launch_description():
    # ==========================================
    # 1. 核心路径配置
    # ==========================================
    pkg_name = 'gb2026_robot_description'
    pkg_share = get_package_share_directory(pkg_name)
    
    default_urdf_path = os.path.join(pkg_share, 'urdf', 'gb2026_r2_robot.urdf.xacro')
    default_rviz_config_path = os.path.join(pkg_share, 'rviz', 'visualize_robot.rviz')
    # 新增：默认的 YAML 参数文件路径
    default_params_file_path = os.path.join(pkg_share, 'params', 'robot_description.yaml')

    # ==========================================
    # 2. 声明 Launch 参数变量
    # ==========================================
    namespace = LaunchConfiguration('namespace')
    use_sim_time = LaunchConfiguration('use_sim_time')
    use_rviz = LaunchConfiguration('use_rviz')
    use_respawn = LaunchConfiguration('use_respawn')
    log_level = LaunchConfiguration('log_level')
    rviz_config_file = LaunchConfiguration('rviz_config_file') 
    params_file = LaunchConfiguration('params_file') # 新增接收参数文件

    # ==========================================
    # 3. 声明命令行参数入口
    # ==========================================
    declare_args = [
        DeclareLaunchArgument('namespace', default_value='', description='Top-level namespace'),
        DeclareLaunchArgument('use_sim_time', default_value='false', description='Use simulation clock'),
        DeclareLaunchArgument('use_rviz', default_value='true', description='Whether to start RViz'),
        DeclareLaunchArgument('use_respawn', default_value='false', description='Respawn crashed nodes'), 
        DeclareLaunchArgument('log_level', default_value='info', description='Log level'),
        DeclareLaunchArgument('rviz_config_file', default_value=default_rviz_config_path, description='RViz config file path'),
        DeclareLaunchArgument('params_file', default_value=default_params_file_path, description='Full path to the ROS 2 parameters file')
    ]

    # ==========================================
    # 4. 核心魔法：动态重写 YAML 参数
    # ==========================================
    # 这里定义需要被 Launch 动态替换的 YAML 键值
    param_substitutions = {
        'use_sim_time': use_sim_time
    }

    # 包装成 ParameterFile 对象，处理命名空间嵌套和临时文件生成
    configured_params = ParameterFile(
        RewrittenYaml(
            source_file=params_file,
            root_key=namespace,
            param_rewrites=param_substitutions,
            convert_types=True,
        ),
        allow_substs=True,
    )

    # ==========================================
    # 5. 构建核心节点组
    # ==========================================
    bringup_cmd_group = GroupAction([
        # 仅当传入了实质性 namespace 时才压入，防止斜杠污染
        PushRosNamespace(
            namespace,
            condition=IfCondition(PythonExpression(["'", namespace, "' != ''"]))
        ),

        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            respawn=use_respawn,
            respawn_delay=2.0,
            # 将重写后的 configured_params 和 xacro 解析结果一起传给节点
            parameters=[configured_params, {'robot_description': Command(['xacro ', default_urdf_path])}],
            arguments=['--ros-args', '--log-level', log_level]
        ),

        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            name='joint_state_publisher',
            output='screen',
            respawn=use_respawn,
            # 直接喂入重写后的参数文件
            parameters=[configured_params],
            arguments=['--ros-args', '--log-level', log_level]
        )
    ])

    # ==========================================
    # 6. RViz 节点 
    # ==========================================
    rviz_node = Node(
        condition=IfCondition(use_rviz),
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', rviz_config_file, '--ros-args', '--log-level', log_level],
        # RViz 一般不需要吃那一套复杂的节点参数，直接传 use_sim_time 即可
        parameters=[{'use_sim_time': use_sim_time}]
    )

    # ==========================================
    # 7. 组装并返回
    # ==========================================
    return LaunchDescription(declare_args + [bringup_cmd_group, rviz_node])