# 此代码为26届RC的算法测试 纯里程计 使用pointlio

# 运行指令：

```bash
#建图命令：
    ros2 launch gb2026_bringup slam_launch.py loam_type:=point_lio
    
    执行之后直接ctrl+c打断即可保存pcd文件到存放文件夹下

#整机启动：
    ros2 launch gb2026_bringup rc_r2_bringup_launch.py

#行为树启动：
    ros2 launch r2_behavior_tree bringup_BT_launch.py