#include <behaviortree_cpp_v3/action_node.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <nav2_msgs/action/follow_path.hpp>
#include <nav_msgs/msg/path.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <sstream> // 用于解析字符串
#include <vector>  // 用于存储解析出的坐标

using FollowPath = nav2_msgs::action::FollowPath;

class MoveChassisNode : public BT::StatefulActionNode
{
public:
    // 构造函数：获取 ROS 2 节点指针，并创建 Action Client
    MoveChassisNode(const std::string &name, const BT::NodeConfiguration &config, rclcpp::Node::SharedPtr node)
        : BT::StatefulActionNode(name, config), node_(node)
    {
        // 对接你辛辛苦苦写好的 PID Action Server!
        client_ = rclcpp_action::create_client<FollowPath>(node_, "gb_move_to_pose");
    }

    // 告诉行为树：我要从 XML 里读取 x, y, yaw 这三个端口数据
    static BT::PortsList providedPorts()
    {
        // 输入端口改为一个 nav_msgs::msg::Path 类型的 "path"
        return {BT::InputPort<nav_msgs::msg::Path>("path")};
    }

    // --- 核心生命周期 1：首次 Tick 触发 ---
    BT::NodeStatus onStart() override
    {
        if (!client_->wait_for_action_server(std::chrono::seconds(5)))
        {
            RCLCPP_ERROR(node_->get_logger(), "找不到底盘 PID 服务！");
            return BT::NodeStatus::FAILURE;
        }

        // 1. 从黑板 (blackboard) 读取由规划器生成的路径
        BT::Optional<nav_msgs::msg::Path> path_msg = getInput<nav_msgs::msg::Path>("path");
        if (!path_msg)
        {
            RCLCPP_ERROR(node_->get_logger(), "MoveChassisNode 缺少 'path' 输入端口！");
            return BT::NodeStatus::FAILURE;
        }

        // 2. 打包发送给你的 PID 控制器
        auto goal_msg = FollowPath::Goal();
        goal_msg.path = path_msg.value();

        auto send_goal_options = rclcpp_action::Client<FollowPath>::SendGoalOptions();
        // 发送动作指令并保存 future 句柄
        goal_handle_future_ = client_->async_send_goal(goal_msg, send_goal_options);

        // 重置已存储的 goal_handle，为新任务做准备
        goal_handle_ = nullptr;
        result_requested_ = false;

        RCLCPP_INFO(node_->get_logger(), "发车！接收到路径，包含 %zu 个点。", goal_msg.path.poses.size());

        // 3. 立刻告诉包工头：我正在跑！
        return BT::NodeStatus::RUNNING;
    }

    // --- 核心生命周期 2：后续的 50Hz 持续 Tick 触发 ---
    BT::NodeStatus onRunning() override
    {
        // 阶段1: 如果我们还没有拿到有效的 goal_handle，就先尝试获取它
        if (!goal_handle_)
        {
            if (goal_handle_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                goal_handle_ = goal_handle_future_.get();
                if (!goal_handle_)
                {
                    RCLCPP_ERROR(node_->get_logger(), "目标被服务器拒绝！");
                    return BT::NodeStatus::FAILURE;
                }
            }
            else
            {
                // 服务器还没响应，继续等待
                return BT::NodeStatus::RUNNING;
            }
        }

        if (goal_handle_ && !result_requested_)
        {
            result_future_ = client_->async_get_result(goal_handle_);
            result_requested_ = true; // 上锁，以后绝不再请求
        }

        if (result_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            auto result = result_future_.get();
            if (result.code == rclcpp_action::ResultCode::SUCCEEDED)
            {
                RCLCPP_INFO(node_->get_logger(), "底盘完美到达！");
                return BT::NodeStatus::SUCCESS; // 任务大功告成！
            }
            else
            {
                RCLCPP_ERROR(node_->get_logger(), "目标执行失败或被中断！");
                return BT::NodeStatus::FAILURE; // 被打断或出错
            }
        }

        // 结果还没出来，继续告诉行为树：还在跑，别急
        return BT::NodeStatus::RUNNING;
    }

    // --- 核心生命周期 3：紧急抢占打断 ---
    void onHalted() override
    {
        RCLCPP_WARN(node_->get_logger(), "收到紧急停止命令！发送 Cancel 给 PID");
        if (goal_handle_)
        {
            client_->async_cancel_goal(goal_handle_);
        }
    }

private:
    rclcpp::Node::SharedPtr node_;
    rclcpp_action::Client<FollowPath>::SharedPtr client_;
    std::shared_future<rclcpp_action::ClientGoalHandle<FollowPath>::SharedPtr> goal_handle_future_;
    rclcpp_action::ClientGoalHandle<FollowPath>::SharedPtr goal_handle_;
    // 保存结果的 future 以及请求锁
    using WrappedResult = rclcpp_action::ClientGoalHandle<FollowPath>::WrappedResult;
    std::shared_future<WrappedResult> result_future_;
    bool result_requested_ = false;
};
