// Copyright 2025 Lihan Chen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file sensor_scan_generation.cpp
 * @brief R2 机器人传感器数据同步与坐标系转换节点
 * 
 * 本节点是 Nav2 导航栈与底层 LIO (激光惯性里程计) 之间的重要桥梁，主要完成三大任务：
 * 1. 精确时间同步：对齐全局点云与全局里程计数据。
 * 2. 局部视图还原：将点云从 Odom 全局系逆投影回雷达局部系，供地形分析 (Terrain Analysis) 与避障使用。
 * 3. 速度反馈生成：利用位姿差分计算 R2 底盘的实时线速度与角速度 (Twist)，满足 Nav2 控制器的闭环需求。
 */

#include "sensor_scan_generation/sensor_scan_generation.hpp"

#include "pcl_ros/transforms.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace sensor_scan_generation
{

/**
 * @brief 构造函数：初始化 ROS 2 参数、TF 监听/广播器、发布订阅以及时间同步器
 */
SensorScanGenerationNode::SensorScanGenerationNode(const rclcpp::NodeOptions & options)
: Node("sensor_scan_generation", options)
{
  // --- 1. 获取坐标系参数 ---
  // lidar_frame      : R2搭载的激光雷达坐标系 (如 "livox_frame")
  // base_frame       : R2底盘地面投影中心，规划与控制的基准 (如 "base_footprint")
  // robot_base_frame : R2上层机构或主框架坐标系
  this->declare_parameter<std::string>("lidar_frame", "");
  this->declare_parameter<std::string>("base_frame", "");
  this->declare_parameter<std::string>("robot_base_frame", "");

  this->get_parameter("lidar_frame", lidar_frame_);
  this->get_parameter("base_frame", base_frame_);
  this->get_parameter("robot_base_frame", robot_base_frame_);

  // --- 2. 初始化 TF 系统 ---
  // 用于查询静态外参 (如 雷达 -> 底盘)
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_, this);
  // 用于广播动态内参 (如 odom -> 底盘)
  br_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  // --- 3. 创建核心发布者 ---
  pub_laser_cloud_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("sensor_scan", 2);
  pub_chassis_odometry_ = this->create_publisher<nav_msgs::msg::Odometry>("odometry", 2);

  // --- 4. 配置 QoS 策略 ---
  // 采用 Best Effort 和 Volatile 策略，专门适配底层高频点云算法的通信机制
  rmw_qos_profile_t qos_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_LAST,
    1,
    RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT,
    RMW_QOS_POLICY_DURABILITY_VOLATILE,
    RMW_QOS_DEADLINE_DEFAULT,
    RMW_QOS_LIFESPAN_DEFAULT,
    RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT,
    RMW_QOS_LIVELINESS_LEASE_DURATION_DEFAULT,
    false};

  // --- 5. 建立精确时间同步 (Exact Time Synchronization) ---
  odometry_sub_.subscribe(this, "lidar_odometry", qos_profile);
  laser_cloud_sub_.subscribe(this, "registered_scan", qos_profile);

  sync_ = std::make_unique<message_filters::Synchronizer<SyncPolicy>>(
    SyncPolicy(100), odometry_sub_, laser_cloud_sub_);
  sync_->registerCallback(
    std::bind(
      &SensorScanGenerationNode::laserCloudAndOdometryHandler, this, std::placeholders::_1,
      std::placeholders::_2));
}

/**
 * @brief 时间同步回调函数 (R2 导航感知的核心预处理)
 * @param odometry_msg LIO 输出的全局里程计 (描述雷达当前在世界系下的绝对位姿)
 * @param pcd_msg LIO 输出的、在 odom 全局系下拼接好的点云
 */
void SensorScanGenerationNode::laserCloudAndOdometryHandler(
  const nav_msgs::msg::Odometry::ConstSharedPtr & odometry_msg,
  const sensor_msgs::msg::PointCloud2::ConstSharedPtr & pcd_msg)
{
  // 定义需要的 TF 变换变量
  tf2::Transform tf_lidar_to_origin;
  tf2::Transform tf_odom_to_origin;
  tf2::Transform tf_odom_to_robot_base;
  tf2::Transform tf_odom_to_lidar;

  // [阶段一]：提取雷达全局位姿 (odom -> lidar)
  tf2::fromMsg(odometry_msg->pose.pose, tf_odom_to_lidar);

  // [阶段二]：查询机器人的静态外参
  tf_lidar_to_robot_base_ = getTransform(lidar_frame_, robot_base_frame_, pcd_msg->header.stamp);
  tf_lidar_to_origin = getTransform(lidar_frame_, base_frame_, pcd_msg->header.stamp);

  // [阶段三]：矩阵运算计算底盘全局位姿
  // 数学推导: T_(odom -> chassis) = T_(odom -> lidar) * T_(lidar -> chassis)
  tf_odom_to_origin = tf_odom_to_lidar * tf_lidar_to_origin;
  tf_odom_to_robot_base = tf_odom_to_lidar * tf_lidar_to_robot_base_;

  // [阶段四]：发布数据供 Nav2 使用
  // 1. 发布底盘的 TF 树变换
  publishTransform(
    tf_odom_to_origin, odometry_msg->header.frame_id, base_frame_, pcd_msg->header.stamp);
  // 2. 发布含有速度 (Twist) 的底盘里程计话题
  publishOdometry(
    tf_odom_to_robot_base, odometry_msg->header.frame_id, robot_base_frame_, pcd_msg->header.stamp);

  // [阶段五]：局部点云还原
  // 业务逻辑：地形分析算法需要当前局部的点云数据，因此利用 tf_odom_to_lidar 的【逆矩阵】，
  // 将全局点云强行拉回并固连到当前的雷达坐标系 (lidar_frame) 下。
  sensor_msgs::msg::PointCloud2 out;
  pcl_ros::transformPointCloud(lidar_frame_, tf_odom_to_lidar.inverse(), *pcd_msg, out);
  pub_laser_cloud_->publish(out);
}

/**
 * @brief 安全获取静态坐标系转换 (TF Lookup)
 * @param target_frame 目标坐标系
 * @param source_frame 源坐标系
 * @param time 指定的时间戳 (由于此处有规避策略，实际未使用该时间)
 * @return tf2::Transform 返回二者的相对变换关系矩阵
 */
tf2::Transform SensorScanGenerationNode::getTransform(
  const std::string & target_frame, const std::string & source_frame, const rclcpp::Time & time)
{
  try {
    // 【防报错修正】: 对于 Livox 此类存在硬件时钟偏离、可能导致
    // "Extrapolation into the future" 报错的硬件，
    // 考虑到外参是死死固定的静态变换，直接传入 rclcpp::Time(0) 强制获取系统中最新可用的 TF 数据。
    auto transform_stamped = tf_buffer_->lookupTransform(
      target_frame, source_frame, rclcpp::Time(0), rclcpp::Duration::from_seconds(0.5));
    tf2::Transform transform;
    tf2::fromMsg(transform_stamped.transform, transform);
    return transform;
  } catch (tf2::TransformException & ex) {
    RCLCPP_WARN(this->get_logger(), "TF lookup failed: %s. Returning identity.", ex.what());
    return tf2::Transform::getIdentity();
  }
}

/**
 * @brief 工具函数：打包并向系统广播 Transform 变换
 */
void SensorScanGenerationNode::publishTransform(
  const tf2::Transform & transform, const std::string & parent_frame,
  const std::string & child_frame, const rclcpp::Time & stamp)
{
  geometry_msgs::msg::TransformStamped transform_msg;
  transform_msg.header.stamp = stamp;
  transform_msg.header.frame_id = parent_frame;
  transform_msg.child_frame_id = child_frame;
  transform_msg.transform = tf2::toMsg(transform);
  br_->sendTransform(transform_msg);
}

/**
 * @brief 工具函数：发布带速度的里程计 (Odometry with Twist)
 * @note Nav2 局部控制器极度依赖这里计算出的速度 (Twist) 来维持闭环跟随和判断机器人状态。
 */
void SensorScanGenerationNode::publishOdometry(
  const tf2::Transform & transform, std::string parent_frame, const std::string & child_frame,
  const rclcpp::Time & stamp)
{
  // 1. 组装 Pose (位姿) 部分
  nav_msgs::msg::Odometry out;
  out.header.stamp = stamp;
  out.header.frame_id = parent_frame;
  out.child_frame_id = child_frame;

  const auto & origin = transform.getOrigin();
  out.pose.pose.position.x = origin.x();
  out.pose.pose.position.y = origin.y();
  out.pose.pose.position.z = origin.z();
  out.pose.pose.orientation = tf2::toMsg(transform.getRotation());

  // 2. 计算微分时间 dt (基于主机时钟)
  // 【潜在隐患警告】:
  // 当前使用 steady_clock (实际物理时间) 求导。实车运行极其稳定。
  // 若用于回放 ROS 2 Bag 文件进行算法仿真，请务必将此处的 dt 计算修改为 (stamp - previous_stamp)，
  // 否则会导致仿真环境下算出的速度突变甚至爆炸。
  static tf2::Transform previous_transform;
  static auto previous_time = std::chrono::steady_clock::now();
  const auto current_time = std::chrono::steady_clock::now();

  const double dt =
    std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - previous_time).count() *
    1e-9;

  // 3. 数值差分算速度 (v = dx/dt, ω = dθ/dt)
  if (dt > 0) {
    const auto linear_velocity = (transform.getOrigin() - previous_transform.getOrigin()) / dt;

    const tf2::Quaternion q_diff =
      transform.getRotation() * previous_transform.getRotation().inverse();
    const auto angular_velocity = q_diff.getAxis() * q_diff.getAngle() / dt;

    // 4. 组装 Twist (速度) 部分
    out.twist.twist.linear.x = linear_velocity.x();
    out.twist.twist.linear.y = linear_velocity.y();
    out.twist.twist.linear.z = linear_velocity.z();
    out.twist.twist.angular.x = angular_velocity.x();
    out.twist.twist.angular.y = angular_velocity.y();
    out.twist.twist.angular.z = angular_velocity.z();
  }

  // 更新静态变量供下个周期求导使用
  previous_transform = transform;
  previous_time = current_time;

  pub_chassis_odometry_->publish(out);
}

}  // namespace sensor_scan_generation

#include "rclcpp_components/register_node_macro.hpp"

RCLCPP_COMPONENTS_REGISTER_NODE(sensor_scan_generation::SensorScanGenerationNode)
