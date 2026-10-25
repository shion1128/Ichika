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
 * @file loam_interface.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-03-16
 * 
 * @copyright Copyright (c) 2026
 * 
 */

/*
 * @brief LIO (激光惯性里程计) 与 Nav2 标准导航栈的接口桥梁节点
 * 
 * 本节点的核心解决的问题是“坐标系原点对齐”。
 * LIO 算法通常以其启动时的雷达位置作为里程计原点 (lidar_odom)，
 * 而 ROS 2 导航栈期望以机器人底盘启动时的位置作为里程计原点 (odom)。
 * 本节点通过监听初始外参，计算出两个原点之间的静态偏移量，
 * 从而将 LIO 实时输出的点云和里程计无缝转换到标准的 odom 全局坐标系下。
*/

#include "loam_interface/loam_interface.hpp"

// 引入 PCL 点云与 ROS TF2 坐标变换互转工具
#include "pcl_ros/transforms.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace loam_interface
{

/**
   * @brief 构造函数：初始化参数、TF 组件及发布/订阅者
   */
LoamInterfaceNode::LoamInterfaceNode(const rclcpp::NodeOptions & options)
: Node("loam_interface", options)
{
  // --- 1. 声明并获取 ROS 2 参数 ---
  // state_estimation_topic: LIO 算法输出的原始里程计话题 (如 /aft_mapped_to_init)
  // registered_scan_topic : LIO 算法输出的对齐后的点云话题 (如 /cloud_registered)
  this->declare_parameter<std::string>("state_estimation_topic", "");
  this->declare_parameter<std::string>("registered_scan_topic", "");
  this->declare_parameter<std::string>("odom_frame", "odom");
  this->declare_parameter<std::string>("base_frame", "");
  this->declare_parameter<std::string>("lidar_frame", "");

  this->get_parameter("state_estimation_topic", state_estimation_topic_);
  this->get_parameter("registered_scan_topic", registered_scan_topic_);
  this->get_parameter("odom_frame", odom_frame_);
  this->get_parameter("base_frame", base_frame_);
  this->get_parameter("lidar_frame", lidar_frame_);

  // 初始化标志位：标记是否已成功获取 base_frame 到 lidar_frame 的静态坐标变换
  base_frame_to_lidar_initialized_ = false;

  // --- 2. 初始化 TF 监听器 ---
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_, this);

  // --- 3. 创建发布者 ---
  pcd_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("registered_scan", 5);
  odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("lidar_odometry", 5);

  // --- 4. 创建订阅者 ---
  pcd_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
    registered_scan_topic_, 5,
    std::bind(&LoamInterfaceNode::pointCloudCallback, this, std::placeholders::_1));
  odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
    state_estimation_topic_, 5,
    std::bind(&LoamInterfaceNode::odometryCallback, this, std::placeholders::_1));
}

/**
 * @brief 点云处理回调函数
 * @param msg LIO 算法输出的原始点云 (位于 lidar_odom 坐标系)
 * @note 利用已计算好的静态偏移矩阵，将点云旋转平移投射到 ROS 标准的 odom 坐标系下。
 */
void LoamInterfaceNode::pointCloudCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr msg)
{
  auto out = std::make_shared<sensor_msgs::msg::PointCloud2>();
  pcl_ros::transformPointCloud(odom_frame_, tf_odom_to_lidar_odom_, *msg, *out);
  pcd_pub_->publish(*out);
}

/**
 * @brief 里程计处理回调函数
 * @param msg LIO 算法输出的原始里程计 (描述雷达在 lidar_odom 坐标系下的位姿)
 * @note 包含核心初始化逻辑：利用初始时刻的 TF 树，计算 odom 与 lidar_odom 之间的固定偏差。
 */
void LoamInterfaceNode::odometryCallback(const nav_msgs::msg::Odometry::ConstSharedPtr msg)
{
  // [阶段一]：首次运行时的坐标系对齐 (Lazy Initialization)
  if (!base_frame_to_lidar_initialized_) {
    try {
      // 查询从 base_frame 到 lidar_frame 的静态外参 (msg 的 stamp 仅作对齐参考)
      auto tf_stamped = tf_buffer_->lookupTransform(
        base_frame_, lidar_frame_, msg->header.stamp, rclcpp::Duration::from_seconds(0.5));
      tf2::Transform tf_base_frame_to_lidar;

      // 将 ROS 通信消息格式转换为底层的 tf2 数学矩阵格式，以便进行矩阵乘法
      tf2::fromMsg(tf_stamped.transform, tf_base_frame_to_lidar);

      // 【坐标系对齐的核心数学原理】
      // 假设机器人在 t=0 时刻启动。ROS 的标准 odom 坐标系原点与 base_frame(t=0) 重合。
      // LIO 算法的里程计原点 (lidar_odom) 与 lidar_frame(t=0) 重合。
      // 因为底盘和雷达是刚性连接的，所以 odom 和 lidar_odom 这两个"原点"之间的相对变换，
      // 就精确等于 base_frame 和 lidar_frame 之间的静态相对变换 (外参)！
      // 因此：T_(odom -> lidar_odom) = T_(base -> lidar)
      tf_odom_to_lidar_odom_ = tf_base_frame_to_lidar;
      base_frame_to_lidar_initialized_ = true;
    } catch (tf2::TransformException & ex) {
      RCLCPP_WARN(this->get_logger(), "TF lookup failed: %s Retrying...", ex.what());
      return;
    }
  }

  // [阶段二]：动态里程计位姿转换
  // 1. 将输入的雷达局部里程计消息提取为 TF 数学格式矩阵
  tf2::Transform tf_lidar_odom_to_lidar;
  tf2::fromMsg(msg->pose.pose, tf_lidar_odom_to_lidar);

  // 2. 矩阵乘法计算：得出 Lidar 在全局 odom 坐标系下的实时绝对位姿
  // 公式：T_(odom -> lidar) = T_(odom -> lidar_odom) * T_(lidar_odom -> lidar)
  tf2::Transform tf_odom_to_lidar = tf_odom_to_lidar_odom_ * tf_lidar_odom_to_lidar;

  // [阶段三]：封装并发布标准里程计
  nav_msgs::msg::Odometry out;
  out.header.stamp = msg->header.stamp;
  out.header.frame_id = odom_frame_;
  // 此处表述的是 lidar 在 odom 下的位姿，因此 child_frame_id 保持为 lidar_frame
  out.child_frame_id = lidar_frame_;

  const auto & origin = tf_odom_to_lidar.getOrigin();
  out.pose.pose.position.x = origin.x();
  out.pose.pose.position.y = origin.y();
  out.pose.pose.position.z = origin.z();
  out.pose.pose.orientation = tf2::toMsg(tf_odom_to_lidar.getRotation());

  odom_pub_->publish(out);
}

}  // namespace loam_interface

// 宏注册：将节点注册为 ROS 2 组件，使其支持 Component 容器加载，实现极低延迟的进程内通信
#include "rclcpp_components/register_node_macro.hpp"

RCLCPP_COMPONENTS_REGISTER_NODE(loam_interface::LoamInterfaceNode)
