/**
 * @file get_topological_path_node.hpp
 * @brief 行为树节点：调用拓扑路径规划服务
 *
 * 本节点是一个同步行为节点 (SyncActionNode)，它的职责是：
 * 1. 从行为树的输入端口接收一个目标航点的名称。
 * 2. 创建一个 ROS 2 Service Client，用于调用 `gb_topological_planner` 提供的服务。
 * 3. 同步调用该服务，并等待路径规划结果。
 * 4. 如果规划成功，将返回的 `nav_msgs::msg::Path` 写入到行为树的输出端口（黑板变量）。
 * 5. 根据服务的执行结果，向行为树返回 SUCCESS 或 FAILURE。
 */
#pragma once

#include <behaviortree_cpp_v3/action_node.h>
#include <rclcpp/rclcpp.hpp>
#include "gb_interfaces/srv/get_topological_path.hpp" // 包含你自定义的服务类型
#include "nav_msgs/msg/path.hpp"

class GetTopologicalPath : public BT::SyncActionNode
{
public:
    // 构造函数：需要传入 ROS 2 节点指针
    GetTopologicalPath(const std::string &name, const BT::NodeConfiguration &config, rclcpp::Node::SharedPtr node)
        : BT::SyncActionNode(name, config), node_(node)
    {
        // 创建一个用于调用拓扑规划服务的客户端
        client_ = node_->create_client<gb_interfaces::srv::GetTopologicalPath>("get_topological_path");
    }

    // 定义该节点需要的输入和输出端口
    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("target_waypoint", "要到达的目标航点名称"),
            BT::OutputPort<nav_msgs::msg::Path>("path", "规划成功后生成的路径")};
    }

    // 核心执行逻辑
    BT::NodeStatus tick() override
    {
        // 1. 检查服务是否可用
        if (!client_->wait_for_service(std::chrono::seconds(5)))
        {
            RCLCPP_ERROR(node_->get_logger(), "拓扑规划服务 'get_topological_path' 不可用！");
            return BT::NodeStatus::FAILURE;
        }

        // 2. 从输入端口获取目标航点名称
        BT::Optional<std::string> target = getInput<std::string>("target_waypoint");
        if (!target)
        {
            RCLCPP_ERROR(node_->get_logger(), "缺少 'target_waypoint' 输入端口！");
            return BT::NodeStatus::FAILURE;
        }

        // 3. 准备并发送服务请求
        auto request = std::make_shared<gb_interfaces::srv::GetTopologicalPath::Request>();
        request->target_waypoint = target.value();

        auto future_result = client_->async_send_request(request);

        // 4. 同步等待服务结果
        if (rclcpp::spin_until_future_complete(node_, future_result, std::chrono::seconds(5)) != rclcpp::FutureReturnCode::SUCCESS)
        {
            RCLCPP_ERROR(node_->get_logger(), "调用拓扑规划服务超时或失败！");
            return BT::NodeStatus::FAILURE;
        }

        auto response = future_result.get();
        if (response->success)
        {
            RCLCPP_INFO(node_->get_logger(), "拓扑规划成功，路径已生成！");
            // 5. 将成功的路径写入到输出端口（黑板）
            setOutput("path", response->path);
            return BT::NodeStatus::SUCCESS;
        }
        else
        {
            RCLCPP_ERROR(node_->get_logger(), "拓扑规划失败: %s", response->message.c_str());
            return BT::NodeStatus::FAILURE;
        }
    }

private:
    rclcpp::Node::SharedPtr node_;
    rclcpp::Client<gb_interfaces::srv::GetTopologicalPath>::SharedPtr client_;
};