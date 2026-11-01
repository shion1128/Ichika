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
 * @file small_gicp_relocalization.cpp
 * @brief 基于 small_gicp 的高频点云重定位节点
 * 
 * 该节点的核心任务是维持全局坐标系 (map) 与里程计坐标系 (odom) 之间的联系。
 * 它通过将实时累积的局部点云与预先构建好的先验全局点云地图进行 GICP 匹配配准，
 * 进而计算出真实的绝对位姿，并持续发布 map -> odom 的坐标变换 (TF)。
 * 同时支持响应 Nav2 / RViz 发布的初始位姿 (/initialpose) 进行重置。
 */

#include "small_gicp_relocalization/small_gicp_relocalization.hpp"

#include "pcl/common/transforms.h"
#include "pcl_conversions/pcl_conversions.h"
#include "small_gicp/pcl/pcl_registration.hpp"
#include "small_gicp/util/downsampling_omp.hpp"
#include "tf2_eigen/tf2_eigen.hpp"

namespace small_gicp_relocalization
{

/**
 * @brief 构造函数：初始化 ROS 2 参数、加载全局地图、配置 GICP 求解器及发布订阅
 */
SmallGicpRelocalizationNode::SmallGicpRelocalizationNode(const rclcpp::NodeOptions & options)
: Node("small_gicp_relocalization", options),
  result_t_(Eigen::Isometry3d::Identity()),
  previous_result_t_(Eigen::Isometry3d::Identity())
{
  // --- 1. 声明并获取 ROS 2 参数 ---
  this->declare_parameter("num_threads", 4);
  this->declare_parameter("num_neighbors", 20);
  this->declare_parameter("max_iterations", 20);
  this->declare_parameter("global_leaf_size", 0.25);
  this->declare_parameter("registered_leaf_size", 0.25);
  this->declare_parameter("max_dist_sq", 1.0);
  this->declare_parameter("map_frame", "map");
  this->declare_parameter("odom_frame", "odom");
  this->declare_parameter("base_frame", "");
  this->declare_parameter("robot_base_frame", "");
  this->declare_parameter("lidar_frame", "");
  this->declare_parameter("prior_pcd_file", "");
  this->declare_parameter("init_pose", std::vector<double>{0., 0., 0., 0., 0., 0.});
  this->declare_parameter("input_cloud_topic", "registered_scan");

  this->get_parameter("num_threads", num_threads_);
  this->get_parameter("num_neighbors", num_neighbors_);
  this->get_parameter("max_iterations", max_iterations_);
  this->get_parameter("global_leaf_size", global_leaf_size_);
  this->get_parameter("registered_leaf_size", registered_leaf_size_);
  this->get_parameter("max_dist_sq", max_dist_sq_);
  this->get_parameter("map_frame", map_frame_);
  this->get_parameter("odom_frame", odom_frame_);
  this->get_parameter("base_frame", base_frame_);
  this->get_parameter("robot_base_frame", robot_base_frame_);
  this->get_parameter("lidar_frame", lidar_frame_);
  this->get_parameter("prior_pcd_file", prior_pcd_file_);
  this->get_parameter("init_pose", init_pose_);
  this->get_parameter("input_cloud_topic", input_cloud_topic_);

  // --- 2. 初始化先验位姿 ---
  // 根据参数 [x, y, z, roll, pitch, yaw] 设置初始的变换矩阵
  if (!init_pose_.empty() && init_pose_.size() >= 6) {
    result_t_.translation() << init_pose_[0], init_pose_[1], init_pose_[2];
    result_t_.linear() =
      Eigen::AngleAxisd(init_pose_[5], Eigen::Vector3d::UnitZ()) *
      Eigen::AngleAxisd(init_pose_[4], Eigen::Vector3d::UnitY()) *
      Eigen::AngleAxisd(init_pose_[3], Eigen::Vector3d::UnitX()).toRotationMatrix();
  }
  previous_result_t_ = result_t_;

  // --- 3. 初始化点云容器与 GICP 算法对象 ---
  accumulated_cloud_ = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
  global_map_ = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
  register_ = std::make_shared<
    small_gicp::Registration<small_gicp::GICPFactor, small_gicp::ParallelReductionOMP>>();

  // --- 4. 初始化 TF 监听与广播 ---
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_);
  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(this);

  // --- 5. 加载并预处理全局点云地图 ---
  loadGlobalMap(prior_pcd_file_);

  // 使用 OMP 多线程对全局地图进行体素下采样，并转换为附带协方差信息的点云格式
  target_ = small_gicp::voxelgrid_sampling_omp<
    pcl::PointCloud<pcl::PointXYZ>, pcl::PointCloud<pcl::PointCovariance>>(
    *global_map_, global_leaf_size_);

  // 计算目标点云的协方差 (GICP 算法的核心要求)
  small_gicp::estimate_covariances_omp(*target_, num_neighbors_, num_threads_);

  // 为目标点云构建 KdTree，以加速最近邻搜索
  target_tree_ = std::make_shared<small_gicp::KdTree<pcl::PointCloud<pcl::PointCovariance>>>(
    target_, small_gicp::KdTreeBuilderOMP(num_threads_));

  // --- 6. 创建订阅者与定时器 ---
  // 订阅对齐到 Odom 坐标系下的实时点云
  pcd_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
    input_cloud_topic_, 10,
    std::bind(&SmallGicpRelocalizationNode::registeredPcdCallback, this, std::placeholders::_1));

  // 订阅初始位姿估计 (如 RViz 的 2D Pose Estimate 工具)
  initial_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
    "initialpose", 10,
    std::bind(&SmallGicpRelocalizationNode::initialPoseCallback, this, std::placeholders::_1));

  // 配准定时器：以 2Hz 的频率执行 GICP 匹配
  register_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(200),  // 2 Hz
    std::bind(&SmallGicpRelocalizationNode::performRegistration, this));

  // TF 发布定时器：以 20Hz 的频率广播 map -> odom
  transform_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(50),  // 20 Hz
    std::bind(&SmallGicpRelocalizationNode::publishTransform, this));
}

/**
 * @brief 加载全局 PCD 地图文件并进行坐标系对齐
 * @param file_name PCD 地图的绝对路径
 */
void SmallGicpRelocalizationNode::loadGlobalMap(const std::string & file_name)
{
  if (pcl::io::loadPCDFile<pcl::PointXYZ>(file_name, *global_map_) == -1) {
    RCLCPP_ERROR(this->get_logger(), "Couldn't read PCD file: %s", file_name.c_str());
    return;
  }
  RCLCPP_INFO(this->get_logger(), "Loaded global map with %zu points", global_map_->points.size());

  // [注意]: FAST-LIO 等算法保存的 PCD 地图通常是基于其建图起始点 (lidar_odom) 的。
  // 为了和标准 Nav2 栈对接，需要利用底盘到雷达的外参，将整个全局地图预先转换到标准的 `odom` 原点下。
  Eigen::Affine3d odom_to_lidar_odom;
  while (true) {
    try {
      // 获取底盘到雷达的静态变换
      auto tf_stamped = tf_buffer_->lookupTransform(
        base_frame_, lidar_frame_, this->now(), rclcpp::Duration::from_seconds(1.0));
      odom_to_lidar_odom = tf2::transformToEigen(tf_stamped.transform);
      RCLCPP_INFO_STREAM(
        this->get_logger(), "odom_to_lidar_odom: translation = "
                              << odom_to_lidar_odom.translation().transpose() << ", rpy = "
                              << odom_to_lidar_odom.rotation().eulerAngles(0, 1, 2).transpose());
      break;
    } catch (tf2::TransformException & ex) {
      RCLCPP_WARN(this->get_logger(), "TF lookup failed: %s Retrying...", ex.what());
      rclcpp::sleep_for(std::chrono::seconds(1));
    }
  }
  // 对地图点云应用原点平移变换
  pcl::transformPointCloud(*global_map_, *global_map_, odom_to_lidar_odom);
}

/**
 * @brief 实时点云回调函数
 * @param msg 从底层里程计接口发来的对齐点云数据
 * @note 此处将接收到的多帧点云进行累加 (Accumulate)，以增加匹配的稠密度。
 */
void SmallGicpRelocalizationNode::registeredPcdCallback(
  const sensor_msgs::msg::PointCloud2::SharedPtr msg)
{
  // 记录最新帧的时间戳和坐标系，用于后续的 TF 广播和坐标转换
  last_scan_time_ = msg->header.stamp;
  current_scan_frame_id_ = msg->header.frame_id;

  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>());
  pcl::fromROSMsg(*msg, *scan);
  // 累加点云
  *accumulated_cloud_ += *scan;
}

/**
 * @brief 核心匹配逻辑：运行 GICP 计算机器人全局位姿
 */
void SmallGicpRelocalizationNode::performRegistration()
{
  if (accumulated_cloud_->empty()) {
    RCLCPP_WARN(this->get_logger(), "No accumulated points to process.");
    return;
  }

  // 对累加后的实时局部点云进行下采样，并构建带协方差的 Source 点云
  source_ = small_gicp::voxelgrid_sampling_omp<
    pcl::PointCloud<pcl::PointXYZ>, pcl::PointCloud<pcl::PointCovariance>>(
    *accumulated_cloud_, registered_leaf_size_);

  // 估算 Source 点云协方差
  small_gicp::estimate_covariances_omp(*source_, num_neighbors_, num_threads_);

  // 构建 Source 侧的 KdTree
  source_tree_ = std::make_shared<small_gicp::KdTree<pcl::PointCloud<pcl::PointCovariance>>>(
    source_, small_gicp::KdTreeBuilderOMP(num_threads_));

  if (!source_ || !source_tree_) {
    return;
  }

  // 配置配准参数
  register_->reduction.num_threads = num_threads_;
  register_->rejector.max_dist_sq = max_dist_sq_;
  register_->optimizer.max_iterations = max_iterations_;

  // 执行 GICP 匹配，使用上一周期的位姿 (previous_result_t_) 作为优化初值
  auto result = register_->align(*target_, *source_, *target_tree_, previous_result_t_);

  if (result.converged) {
    // 若匹配收敛，则更新位姿记录
    result_t_ = previous_result_t_ = result.T_target_source;
  } else {
    RCLCPP_WARN(this->get_logger(), "GICP did not converge.");
  }

  // 匹配完成后清空累加容器，准备下一轮收集
  accumulated_cloud_->clear();
}

/**
 * @brief 发布 Map 到 Odom 的坐标变换 (TF)
 */
void SmallGicpRelocalizationNode::publishTransform()
{
  // 如果位姿还是严格的单位矩阵且未曾更新过，则不发布
  // if (result_t_.matrix().isZero()) {
  //   return;
  // }

  geometry_msgs::msg::TransformStamped transform_stamped;
  // 给时间戳略微增加 0.1 秒的“未来时间”，这是一种经典的 ROS TF 容差补偿技巧。
  // 目的是防止高频局部控制器因为极小的 TF 延迟而报 extrapolation 错误。
  transform_stamped.header.stamp = last_scan_time_ + rclcpp::Duration::from_seconds(0.1);
  transform_stamped.header.frame_id = map_frame_;
  transform_stamped.child_frame_id = odom_frame_;

  // 提取平移和旋转部分并组装为 ROS TF 消息
  const Eigen::Vector3d translation = result_t_.translation();
  const Eigen::Quaterniond rotation(result_t_.rotation());

  transform_stamped.transform.translation.x = translation.x();
  transform_stamped.transform.translation.y = translation.y();
  transform_stamped.transform.translation.z = translation.z();
  transform_stamped.transform.rotation.x = rotation.x();
  transform_stamped.transform.rotation.y = rotation.y();
  transform_stamped.transform.rotation.z = rotation.z();
  transform_stamped.transform.rotation.w = rotation.w();

  tf_broadcaster_->sendTransform(transform_stamped);
}

/**
 * @brief 处理手动设定的初始位姿 (如 Rviz 的 2D Pose Estimate)
 * @param msg 包含位姿与协方差的消息
 */
void SmallGicpRelocalizationNode::initialPoseCallback(
  const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg)
{
  RCLCPP_INFO(
    this->get_logger(), "Received initial pose: [x: %f, y: %f, z: %f]", msg->pose.pose.position.x,
    msg->pose.pose.position.y, msg->pose.pose.position.z);

  // 将接收到的位姿转为 Eigen 矩阵 (map -> robot_base)
  Eigen::Isometry3d map_to_robot_base = Eigen::Isometry3d::Identity();
  map_to_robot_base.translation() << msg->pose.pose.position.x, msg->pose.pose.position.y,
    msg->pose.pose.position.z;
  map_to_robot_base.linear() = Eigen::Quaterniond(
                                 msg->pose.pose.orientation.w, msg->pose.pose.orientation.x,
                                 msg->pose.pose.orientation.y, msg->pose.pose.orientation.z)
                                 .toRotationMatrix();

  try {
    // 查询当前 robot_base -> odom (注意此时 scan 的 frame 通常是 odom 或 lidar_odom)
    auto transform =
      tf_buffer_->lookupTransform(robot_base_frame_, current_scan_frame_id_, tf2::TimePointZero);
    Eigen::Isometry3d robot_base_to_odom = tf2::transformToEigen(transform.transform);

    // 计算得出 map -> odom 的变换矩阵，并重置 GICP 的初始推断值
    Eigen::Isometry3d map_to_odom = map_to_robot_base * robot_base_to_odom;

    previous_result_t_ = result_t_ = map_to_odom;
  } catch (tf2::TransformException & ex) {
    RCLCPP_WARN(
      this->get_logger(), "Could not transform initial pose from %s to %s: %s",
      robot_base_frame_.c_str(), current_scan_frame_id_.c_str(), ex.what());
  }
}

}  // namespace small_gicp_relocalization

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(small_gicp_relocalization::SmallGicpRelocalizationNode)
