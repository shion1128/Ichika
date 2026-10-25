#ifndef GB_OMNI_PID_PURSUIT_CONTROLLER_HPP_
#define GB_OMNI_PID_PURSUIT_CONTROLLER_HPP_

#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"

#include "pid.hpp"

#include <nav2_msgs/action/follow_path.hpp>
#include <nav_msgs/msg/path.hpp>

namespace gb_omni_pid_pursuit_controller
{

    class GbOmniPidPursuitController : public rclcpp::Node
    {
    public:
        using NavigateToPose = nav2_msgs::action::NavigateToPose;
        using GoalHandleNavigate = rclcpp_action::ServerGoalHandle<NavigateToPose>;

        using FollowPath = nav2_msgs::action::FollowPath;
        using GoalHandleFollowPath = rclcpp_action::ServerGoalHandle<FollowPath>;

        explicit GbOmniPidPursuitController(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());
        ~GbOmniPidPursuitController() override;

    private:
        rclcpp_action::GoalResponse handle_goal(const rclcpp_action::GoalUUID &uuid, std::shared_ptr<const FollowPath::Goal> goal);
        rclcpp_action::CancelResponse handle_cancel(const std::shared_ptr<GoalHandleFollowPath> goal_handle);
        void handle_accepted(const std::shared_ptr<GoalHandleFollowPath> goal_handle);
        void execute();

        bool get_current_pose(geometry_msgs::msg::PoseStamped &global_pose);
        rcl_interfaces::msg::SetParametersResult dynamicParametersCallback(std::vector<rclcpp::Parameter> parameters);

        rclcpp_action::Server<FollowPath>::SharedPtr action_server_;
        rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr vel_pub_;

        std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
        std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
        rclcpp::TimerBase::SharedPtr time_; // 测试使用的定时器

        rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr dyn_params_handler_;

        std::shared_ptr<PID> move_pid_;
        std::shared_ptr<PID> heading_pid_;

        std::mutex mutex_;
        nav_msgs::msg::Path current_goal_;
        std::atomic<bool> has_active_goal_{false};

        geometry_msgs::msg::Twist cmd_vel_msg_;

        // 线程管理与抢占机制
        std::shared_ptr<GoalHandleFollowPath> active_goal_handle_;
        std::thread control_thread_;
        std::atomic<bool> is_running_{false};

        // 到达稳定判定
        int settle_counter_ = 0;
        int settle_ticks_{10}; // 稳定拍数 (可由 params.yaml 配置)

        // 纯追踪前视距离 (m)
        double lookahead_dist_{0.4};

        // 加速度限制 (保存上一周期的速度)
        double last_lin_vel_ = 0.0;
        double max_accel_ = 1.0; // 最大线加速度 m/s^2

        double xy_goal_tolerance_{0.01};
        double yaw_goal_tolerance_{0.01}; // 约 1度的误差死区，防止抖动
        double max_linear_vel_{1.0};
        double max_angular_vel_{2.0};
        double control_frequency_{50.0};

        double approach_velocity_scaling_dist_{0.4};
        double min_approach_linear_velocity_{0.01};

        void time_callback();
    };

} // namespace gb_omni_pid_pursuit_controller

#endif // GB_OMNI_PID_PURSUIT_CONTROLLER_HPP_