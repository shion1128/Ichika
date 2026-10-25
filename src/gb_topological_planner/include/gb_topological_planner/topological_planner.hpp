#ifndef TOPOLOGICAL_PLANNER_NODE_HPP_
#define TOPOLOGICAL_PLANNER_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/path.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

// 包含你的自定义服务接口 (请根据实际包名修改)
#include "gb_interfaces/srv/get_topological_path.hpp"

#include <yaml-cpp/yaml.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace gb_navigation
{

    // ==========================================
    // 1. 图论数据结构定义
    // ==========================================
    struct TopoNode
    {
        std::string name;
        double x;
        double y;
        bool has_fixed_yaw; // 是否有强制方向
        double yaw;         // 强制的偏航角
        // 邻接表：记录与该节点相连的所有相邻节点的名称
        std::vector<std::string> neighbors; // 无向图，双向添加邻居
    };

    class TopologicalPlanner : public rclcpp::Node
    {
    public:
        TopologicalPlanner(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());
        ~TopologicalPlanner() override = default;

    private:
        // --- 核心模块 1：地图解析与存储 ---
        bool loadTopologyMap(const std::string &yaml_path);
        // 核心拓扑图：通过节点名称(string)快速查找节点详情(TopoNode)
        std::unordered_map<std::string, TopoNode> topology_graph_; // 底层是哈希表，查询效率 O(1)，适合频繁访问

        // --- 核心模块 2：服务提供 ---
        using GetTopoPathSrv = gb_interfaces::srv::GetTopologicalPath;
        rclcpp::Service<GetTopoPathSrv>::SharedPtr planner_service_;
        void handle_plan_request(
            const std::shared_ptr<GetTopoPathSrv::Request> request,
            std::shared_ptr<GetTopoPathSrv::Response> response);

        // --- 核心模块 3：图搜索与几何处理 ---
        // 寻找距离当前物理坐标最近的拓扑节点（确定起点）
        std::string findClosestNode(double current_x, double current_y);
        // 运行 Dijkstra 或 A* 算法，返回宏观航点序列
        std::vector<TopoNode> findShortestPath(const std::string &start_name, const std::string &goal_name);
        // 路径稠密插值 (为 Pure Pursuit 提供胡萝卜)
        nav_msgs::msg::Path interpolatePath(const std::vector<TopoNode> &coarse_path, double step_size = 0.05);

        // --- 辅助模块：获取自身位置 ---
        std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
        std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
        bool getCurrentPose(double &x, double &y);

        double interpolation_step_; // 插值密度参数，默认 0.05m
    };

} // namespace gb_navigation

#endif // TOPOLOGICAL_PLANNER_NODE_HPP_
