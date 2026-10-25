/**
 * @file omni_pid_pursuit_controller_node.cpp
 * @brief 全向轮 PID 纯追踪控制器 (Pure Pursuit Controller) 节点实现
 *
 * 本节点实现了一个 ROS 2 Action Server (nav2_msgs::action::FollowPath)，用于接收一条路径。
 * 它的核心思想是：
 * 1. 采用 Pure Pursuit 算法，在接收到的路径上寻找一个前视点（"胡萝卜点"）。
 * 2. 使用两个独立的 PID 控制器：一个用于控制机器人向"胡萝卜点"移动的线速度，另一个用于控制机器人朝向"胡萝卜点"姿态的角速度。
 * 3. 针对全向底盘，它会将计算出的全局速度矢量，通过坐标变换分解到机器人的局部坐标系 (base_link) 下的 vx 和 vy。
 * 4. 实现了完整的 Action 抢占机制、平滑减速、加速度限制和稳定到达判定，
 * 最终发布 cmd_vel 指令以驱动机器人平滑到达目标点。
 */

#include "gb_omni_pid_pursuit_controller/omni_pid_pursuit_controller_node.hpp"
#include <tf2/utils.h>
#include <cmath>
#include <algorithm>
#include "std_msgs/msg/empty.hpp"
#include <chrono>
#include <memory>
// 包含标准的 Nav2 路径追踪 Action 定义
#include <nav2_msgs/action/follow_path.hpp>
#include <nav_msgs/msg/path.hpp>

// 使用 C++ 标准库中的时间字面量，如 100ms
using namespace std::chrono_literals;

namespace gb_omni_pid_pursuit_controller
{
    /**
     * @brief 构造函数：初始化 TF、发布者、Action Server 以及动态参数回调
     */
    GbOmniPidPursuitController::GbOmniPidPursuitController(const rclcpp::NodeOptions &options)
        : Node("gb_omni_pid_pursuit_controller_node", options)
    {
        // 1. 初始化 TF 缓冲和监听器，用于获取 "map" -> "base_link" 的坐标变换
        tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this);

        // 2. 创建速度指令发布者，话题名为 "cmd_vel"
        vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", rclcpp::SystemDefaultsQoS());

        // 3. 创建 Action Server，并绑定三个核心回调函数
        action_server_ = rclcpp_action::create_server<FollowPath>(
            this, "gb_move_to_pose",
            // handle_goal: "门卫"，决定是否接受新的任务请求
            [this](const rclcpp_action::GoalUUID &uuid, std::shared_ptr<const FollowPath::Goal> goal)
            {
                return this->handle_goal(uuid, goal);
            },
            // handle_cancel: 当客户端请求取消任务时被调用
            [this](const std::shared_ptr<GoalHandleFollowPath> goal_handle)
            {
                return this->handle_cancel(goal_handle);
            },
            // handle_accepted: 当任务被接受后，正式开始执行任务
            [this](const std::shared_ptr<GoalHandleFollowPath> goal_handle)
            {
                this->handle_accepted(goal_handle);
            });

        // 4. 声明并初始化本节点所需的所有参数，并提供默认值
        double move_kp = this->declare_parameter("move_kp", 1.5);
        double move_ki = this->declare_parameter("move_ki", 0.0);
        double move_kd = this->declare_parameter("move_kd", 0.3);
        double heading_kp = this->declare_parameter("heading_kp", 2.0);
        double heading_ki = this->declare_parameter("heading_ki", 0.0);
        double heading_kd = this->declare_parameter("heading_kd", 0.2);

        // 最大线/角速度：写入成员变量，同时用于 PID 输出限幅
        max_linear_vel_ = this->declare_parameter("max_linear_vel_", 1.0);
        max_angular_vel_ = this->declare_parameter("max_angular_vel_", 1.0);

        // 纯追踪前视距离 / 加速度上限 / 到达容差 / 稳定拍数：写入成员变量
        lookahead_dist_ = this->declare_parameter("lookahead_dist", 1.0);
        max_accel_ = this->declare_parameter("max_accel_", 1.0);
        xy_goal_tolerance_ = this->declare_parameter("xy_goal_tolerance", 0.01);
        yaw_goal_tolerance_ = this->declare_parameter("yaw_goal_tolerance", 0.01);
        settle_ticks_ = this->declare_parameter("settle_ticks", 10);

        // 接近目标时开始减速的距离阈值
        approach_velocity_scaling_dist_ = this->declare_parameter("approach_dist", 0.4);
        // 接近目标时的最小保护速度，防止速度缩放后变为0导致机器人停滞
        min_approach_linear_velocity_ = this->declare_parameter("min_approach_vel", 0.08);

        // 5. 根据参数初始化两个 PID 控制器实例
        double dt = 1.0 / control_frequency_;
        move_pid_ = std::make_shared<PID>(dt, max_linear_vel_, -max_linear_vel_, move_kp, move_ki, move_kd);
        heading_pid_ = std::make_shared<PID>(dt, max_angular_vel_, -max_angular_vel_, heading_kp, heading_ki, heading_kd);

        // 6. 注册动态参数修改回调函数，这使得我们可以通过 rqt_reconfigure 等工具在运行时实时调参
        dyn_params_handler_ = this->add_on_set_parameters_callback(
            [this](const std::vector<rclcpp::Parameter> &parameters)
            {
                return this->dynamicParametersCallback(parameters);
            });

        // 测试使用
        time_ = this->create_wall_timer(100ms, [this]()
                                        { return this->time_callback(); });

        RCLCPP_INFO(this->get_logger(), "GB Omni PID Pursuit Controller 已启动 (50Hz)。");
    }

    GbOmniPidPursuitController::~GbOmniPidPursuitController() {}

    /**
     * @brief 处理新的 Action 目标请求
     * @param uuid 目标的唯一ID
     * @param goal 客户端发来的目标内容 (这里是 nav2_msgs::action::FollowPath::Goal)
     * @return 返回 ACCEPT_AND_EXECUTE 表示接受并立即执行，或 REJECT 表示拒绝
     */
    rclcpp_action::GoalResponse GbOmniPidPursuitController::handle_goal(
        const rclcpp_action::GoalUUID & /*uuid*/, std::shared_ptr<const FollowPath::Goal> goal)
    {
        // 检查收到的路径是否为空，如果非空则记录日志
        if (!goal->path.poses.empty())
        {
            double final_x = goal->path.poses.back().pose.position.x;
            double final_y = goal->path.poses.back().pose.position.y;
            RCLCPP_INFO(this->get_logger(),
                        "收到新路线！共 %zu 个点，最终目的地: [X: %.2f, Y: %.2f]",
                        goal->path.poses.size(), final_x, final_y);
        }
        else
        {
            RCLCPP_WARN(this->get_logger(), "收到了一条空路线！");
        }
        // 默认总是接受新目标
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    }

    /**
     * @brief 处理 Action 取消请求
     * @param goal_handle 正在执行的目标的句柄
     * @return 返回 ACCEPT 表示同意取消
     */
    rclcpp_action::CancelResponse GbOmniPidPursuitController::handle_cancel(
        const std::shared_ptr<GoalHandleFollowPath> /*goal_handle*/)
    {
        RCLCPP_INFO(this->get_logger(), "接收到取消指令，执行紧急刹车。");
        return rclcpp_action::CancelResponse::ACCEPT;
    }

    /**
     * @brief 目标被接受后的处理逻辑
     * @details 这是实现抢占式任务的核心。它会检查当前是否有任务在运行，
     *          如果有，则中止旧任务，并用新任务替换。然后启动一个独立的后台线程来执行控制循环 (execute 函数)。
     * @param goal_handle 新接受的目标的句柄
     */
    void GbOmniPidPursuitController::handle_accepted(const std::shared_ptr<GoalHandleFollowPath> goal_handle)
    {
        {
            // 使用互斥锁保护共享资源，确保线程安全
            std::lock_guard<std::mutex> lock(mutex_);
            // 如果已经有一个任务在运行，则执行抢占逻辑
            if (is_running_ && active_goal_handle_)
            {
                RCLCPP_WARN(this->get_logger(), "收到新目标，正在抢占并中止上一个目标！");
                auto result = std::make_shared<FollowPath::Result>();
                // 将上一个目标标记为 ABORTED (被中止)
                active_goal_handle_->abort(result);
            }
            // 用新的目标句柄和路径数据覆盖旧的
            active_goal_handle_ = goal_handle;
            current_goal_ = goal_handle->get_goal()->path;
            has_active_goal_ = true;
        }
        // 如果当前没有控制线程在运行
        if (!is_running_)
        {
            // 如果旧线程存在且可汇合，先回收它 (极其重要的内存安全操作)
            if (control_thread_.joinable())
            {
                control_thread_.join();
            }
            // 标记为正在运行，并创建一个新的后台线程来执行 `execute` 函数
            is_running_ = true;
            control_thread_ = std::thread(&GbOmniPidPursuitController::execute, this);
        }
    }

    /**
     * @brief 核心控制循环：运行在独立线程中，持续计算并发布速度直至到达目标
     * @details 这是控制器算法的核心，它以 `control_frequency_` 的频率循环执行，
     *          直到任务完成、被取消或被抢占。
     * @param goal_handle 当前正在执行的目标的句柄 (注意：在此实现中，该参数未使用，
     *                    而是通过线程安全的成员变量 `active_goal_handle_` 获取最新句柄)
     */
    void GbOmniPidPursuitController::execute()
    {
        // 创建一个用于返回给客户端的结果对象
        auto result = std::make_shared<FollowPath::Result>();

        // ==========================================
        // 算法核心参数
        // ==========================================
        double lookahead_dist = lookahead_dist_; // 纯追踪算法的"胡萝卜"前视距离 (m)

        // 创建循环速率控制器
        rclcpp::Rate loop_rate(control_frequency_);
        double dt = 1.0 / control_frequency_;

        // 循环内状态变量
        std::shared_ptr<GoalHandleFollowPath> last_handle = nullptr;
        double final_x = 0.0;
        double final_y = 0.0;
        double final_yaw = 0.0;
        size_t current_carrot_idx = 0;

        // 主控制循环
        while (rclcpp::ok() && is_running_)
        {
            // 在循环开始时，从共享成员变量中安全地获取最新的目标句柄和路径。
            // 这确保了即使在循环中任务被抢占，我们也能在下一次迭代时处理新任务。
            std::shared_ptr<GoalHandleFollowPath> current_handle;
            nav_msgs::msg::Path goal_pose;

            {
                // 安全读取当前目标
                std::lock_guard<std::mutex> lock(mutex_);
                if (!has_active_goal_ || !active_goal_handle_)
                {
                    // 如果没有活动目标，说明任务已被外部（如抢占）终止，退出循环
                    is_running_ = false;
                    break;
                }
                current_handle = active_goal_handle_;
                goal_pose = current_goal_;
            }

            // 如果目标句柄发生了变化（新任务或抢占），则重置所有控制状态
            if (last_handle != current_handle)
            {
                RCLCPP_INFO(this->get_logger(), "检测到新目标/抢占，正在重新初始化控制循环...");
                if (goal_pose.poses.empty())
                {
                    RCLCPP_ERROR(this->get_logger(), "新目标路径为空，中止任务！");
                    current_handle->abort(result);
                    last_handle = current_handle; // 标记为已处理，防止重复报错
                    continue;                     // 等待下一个有效目标
                }
                // 重新初始化所有与路径相关的状态
                final_x = goal_pose.poses.back().pose.position.x;
                final_y = goal_pose.poses.back().pose.position.y;
                final_yaw = tf2::getYaw(goal_pose.poses.back().pose.orientation);
                current_carrot_idx = 0;
                move_pid_->reset();
                heading_pid_->reset();
                settle_counter_ = 0;
                last_lin_vel_ = 0.0;
                last_handle = current_handle; // 更新“上一个句柄”的记录
            }

            // 1. 检查客户端（如行为树）是否请求取消当前任务
            if (current_handle->is_canceling())
            {
                // 使用锁来安全地更新共享状态
                std::lock_guard<std::mutex> lock(mutex_);
                has_active_goal_ = false;
                is_running_ = false;

                // 发布零速指令，使机器人立即停止
                cmd_vel_msg_.linear.x = 0.0;
                cmd_vel_msg_.linear.y = 0.0;
                cmd_vel_msg_.angular.z = 0.0;
                vel_pub_->publish(cmd_vel_msg_);

                // 将目标标记为 CANCELED 并通知客户端
                current_handle->canceled(result);
                RCLCPP_INFO(this->get_logger(), "任务已取消，执行停车。");
                return;
            }

            // 2. 获取 TF 全局真实位姿
            geometry_msgs::msg::PoseStamped current_pose;
            if (!get_current_pose(current_pose))
            {
                // 如果获取TF失败，发出警告，发布零速，并等待下一个周期
                RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "等待 TF 树 (map -> base_link)...");
                vel_pub_->publish(geometry_msgs::msg::Twist());
                loop_rate.sleep();
                continue;
            }

            // 从获取到的位姿中提取出机器人的物理坐标 (pv_x, pv_y) 和航向角 current_yaw
            double pv_x = current_pose.pose.position.x;
            double pv_y = current_pose.pose.position.y;
            double current_yaw = tf2::getYaw(current_pose.pose.orientation);

            // ==========================================
            // 3. 胡萝卜寻点魔法 (Pure Pursuit 核心)
            // ==========================================
            bool carrot_found = false;
            // 从上一次找到的点开始，继续在路径上寻找下一个"胡萝卜点"
            for (size_t i = current_carrot_idx; i < goal_pose.poses.size(); ++i)
            {
                double px = goal_pose.poses[i].pose.position.x;
                double py = goal_pose.poses[i].pose.position.y;

                double dist = std::hypot(px - pv_x, py - pv_y);

                // 找到路径上第一个与机器人当前位置的距离大于或等于前视距离的点
                if (dist >= lookahead_dist)
                {
                    current_carrot_idx = i;
                    carrot_found = true;
                    break;
                }
            }

            // 【关键修复】如果遍历完所有点都没找到足够远的点 (通常发生在接近路径终点时)，
            // 则强制将路径的最后一个点作为胡萝卜点，确保机器人能跑完全程。
            if (!carrot_found && !goal_pose.poses.empty())
            {
                current_carrot_idx = goal_pose.poses.size() - 1;
            }

            // 提取这一帧追踪目标的“胡萝卜”位姿
            double carrot_x = goal_pose.poses[current_carrot_idx].pose.position.x;
            double carrot_y = goal_pose.poses[current_carrot_idx].pose.position.y;
            double carrot_yaw = tf2::getYaw(goal_pose.poses[current_carrot_idx].pose.orientation);

            // RCLCPP_INFO(this->get_logger(), "current_carrot: X: %.2f, Y: %.2f, Yaw: %.2f", carrot_x, carrot_y, carrot_yaw);

            // ==========================================
            // 4. 计算底盘控制误差 (注意两者的区别！)
            // ==========================================

            // A. 用于底层 PID 驱动的误差 (始终以"胡萝卜点"为目标)
            double dx_to_carrot = carrot_x - pv_x;
            double dy_to_carrot = carrot_y - pv_y;
            double distance_to_carrot = std::hypot(dx_to_carrot, dy_to_carrot);

            // 航向误差也以"胡萝卜点"的姿态为目标
            double yaw_error = carrot_yaw - current_yaw;
            // 将角度误差归一化到 [-PI, PI] 区间
            yaw_error = std::atan2(std::sin(yaw_error), std::cos(yaw_error));

            // B. 用于判断最终是否到达的误差 (只看路径的绝对终点，不看"胡萝卜点")
            double dist_to_final = std::hypot(final_x - pv_x, final_y - pv_y);
            double final_yaw_error = final_yaw - current_yaw;
            final_yaw_error = std::atan2(std::sin(final_yaw_error), std::cos(final_yaw_error));

            // ==========================================
            // 5. 稳定到达判定 (Settle Logic)
            // ==========================================
            // 检查机器人是否同时满足位置和姿态的容忍度要求
            if (dist_to_final < xy_goal_tolerance_ && std::abs(final_yaw_error) < yaw_goal_tolerance_)
            {
                // 如果满足，则开始累加稳定计数器
                settle_counter_++;
                // 如果稳定计数器达到要求值，则认为任务成功
                if (settle_counter_ >= settle_ticks_)
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    has_active_goal_ = false;
                    is_running_ = false;

                    cmd_vel_msg_.linear.x = 0.0;
                    cmd_vel_msg_.linear.y = 0.0;
                    cmd_vel_msg_.angular.z = 0.0;
                    vel_pub_->publish(cmd_vel_msg_);

                    // 标记任务成功并退出
                    current_handle->succeed(result);
                    RCLCPP_INFO(this->get_logger(), "完美稳定到达最终目标点！");
                    return;
                }
            }
            else
            {
                // 如果机器人移出了容忍度范围，则清零计数器
                settle_counter_ = 0;
            }

            // ==========================================
            // 6. PID 计算与平滑限幅
            // ==========================================
            // 线速度PID：输入是距离"胡萝卜点"的误差，目标是0 (即到达胡萝卜点)
            double lin_vel = move_pid_->calculate(distance_to_carrot, 0.0);
            // 角速度PID：输入是朝向"胡萝卜点"的航向误差，目标是0
            // ★ 注意参数顺序必须是 (0.0, yaw_error)，即 raw_error = 0 - yaw_error。
            //   因为下面 angular.z = -ang_vel 还有一次取反，两次负号相消后
            //   angular.z = +kp*yaw_error，才符合 ROS 标准（+z 为逆时针、yaw 增大）。
            //   写成 calculate(yaw_error, 0.0) 会得到 angular.z = -kp*yaw_error，
            //   即航向环变成正反馈：机器人会边跑边转、画圆弧、最后倒着走。
            double ang_vel = heading_pid_->calculate(0.0, yaw_error);

            // 当机器人接近路径的最终目标点时，执行平滑减速
            if (dist_to_final < approach_velocity_scaling_dist_)
            {
                // 根据距离按比例缩放线速度
                double scaling = dist_to_final / approach_velocity_scaling_dist_;
                lin_vel *= scaling;

                if (dist_to_final > xy_goal_tolerance_)
                {
                    // 为缩放后的速度设置一个最小速度，防止机器人因速度过小而停滞
                    // ★ 只做"正向下限"，绝不能用 copysign：否则 PID 输出为负时会
                    //   被强行改成 -min_approach_vel，导致接近目标时指令正负来回翻转。
                    if (lin_vel < min_approach_linear_velocity_)
                    {
                        lin_vel = min_approach_linear_velocity_;
                    }
                }
                else
                {
                    // 已进入位置容差，不允许被抬速
                    lin_vel = 0.0;
                }
            }

            // 简易加速度限幅 (Slew Rate Limiter)，防止速度突变
            double max_vel_change = max_accel_ * dt;
            if (lin_vel > last_lin_vel_ + max_vel_change)
            {
                lin_vel = last_lin_vel_ + max_vel_change;
            }
            else if (lin_vel < last_lin_vel_ - max_vel_change)
            {
                lin_vel = last_lin_vel_ - max_vel_change;
            }
            // 保存当前周期的速度，供下一周期使用
            last_lin_vel_ = lin_vel;

            // ==========================================
            // 7. 全向轮底盘矢量解算
            // ==========================================
            // 计算从机器人当前位置指向"胡萝卜点"的矢量在全局坐标系("map")下的角度
            double global_move_angle = std::atan2(dy_to_carrot, dx_to_carrot);
            // 将该全局角度转换到机器人自身的局部坐标系("base_link")下
            double local_move_angle = global_move_angle - current_yaw;

            // 将总的线速度 `lin_vel` 分解到机器人局部坐标系的 x 和 y 轴上
            cmd_vel_msg_.linear.x = lin_vel * std::cos(local_move_angle);
            cmd_vel_msg_.linear.y = lin_vel * std::sin(local_move_angle);
            // 设置角速度。注意：这里的负号取决于你的底盘角速度正方向定义。
            // 如果 `cmd_vel` 的 `angular.z` 为正，底盘逆时针旋转，则这里需要负号。反之则去掉。
            cmd_vel_msg_.angular.z = -ang_vel;

            // 仅在调试时打开，避免终端刷屏导致卡顿
            // RCLCPP_INFO(this->get_logger(), "Vx: %.2f, Vy: %.2f, Wz: %.2f | 距终点: %.2fm",
            //             cmd_vel_msg_.linear.x, cmd_vel_msg_.linear.y, cmd_vel_msg_.angular.z, dist_to_final);

            // 发布最终的速度指令
            vel_pub_->publish(cmd_vel_msg_);

            loop_rate.sleep();
        }
    }

    /**
     * @brief 利用 TF 树获取机器人当前的全局位姿
     * @param[out] global_pose 用于存储获取到的位姿信息
     * @return 成功获取返回 true，因 TF 延迟或丢失则返回 false
     */
    bool GbOmniPidPursuitController::get_current_pose(geometry_msgs::msg::PoseStamped &global_pose)
    {
        try
        {
            // 查找从 "map" (全局坐标系) 到 "base_link" (机器人坐标系) 的最新坐标变换
            auto transform = tf_buffer_->lookupTransform(
                "map", "base_link", tf2::TimePointZero, tf2::durationFromSec(0.0));

            // 将获取到的变换信息填充到 PoseStamped 消息中
            global_pose.header.stamp = this->now();
            global_pose.header.frame_id = "map";
            global_pose.pose.position.x = transform.transform.translation.x;
            global_pose.pose.position.y = transform.transform.translation.y;
            global_pose.pose.orientation = transform.transform.rotation;
            return true;
        }
        catch (const tf2::TransformException &ex)
        {
            // 如果 TF 变换暂时不可用，捕获异常并返回 false
            return false;
        }
    }

    /**
     * @brief 动态参数配置回调
     * @note 允许用户在不重启节点的情况下，通过 rqt_reconfigure 等工具实时修改 PID 增益
     * @param parameters 一个包含所有被修改参数的向量
     * @return 返回一个表明参数设置是否成功的结果
     */
    rcl_interfaces::msg::SetParametersResult GbOmniPidPursuitController::dynamicParametersCallback(
        std::vector<rclcpp::Parameter> parameters)
    {
        rcl_interfaces::msg::SetParametersResult result;
        result.successful = true;

        // 关键步骤：在处理传入的参数之前，先用局部变量读取所有相关参数的当前值。
        // 这是因为回调函数只包含被修改的参数，如果不先读取，未被修改的参数将会丢失其当前值。
        double move_kp = this->get_parameter("move_kp").as_double();
        double move_ki = this->get_parameter("move_ki").as_double();
        double move_kd = this->get_parameter("move_kd").as_double();
        double head_kp = this->get_parameter("heading_kp").as_double();
        double head_ki = this->get_parameter("heading_ki").as_double();
        double head_kd = this->get_parameter("heading_kd").as_double();

        double max_lin_vel = this->get_parameter("max_linear_vel_").as_double();
        double max_ang_vel = this->get_parameter("max_angular_vel_").as_double();

        // 遍历所有从客户端（如rqt_reconfigure）发来的修改请求，并更新对应的局部变量
        for (const auto &param : parameters)
        {
            if (param.get_name() == "move_kp")
                move_kp = param.as_double();
            else if (param.get_name() == "move_ki")
                move_ki = param.as_double();
            else if (param.get_name() == "move_kd")
                move_kd = param.as_double();
            else if (param.get_name() == "heading_kp")
                head_kp = param.as_double();
            else if (param.get_name() == "heading_ki")
                head_ki = param.as_double();
            else if (param.get_name() == "heading_kd")
                head_kd = param.as_double();
            else if (param.get_name() == "approach_dist")
                approach_velocity_scaling_dist_ = param.as_double();
            else if (param.get_name() == "min_approach_vel")
                min_approach_linear_velocity_ = param.as_double();
            else if (param.get_name() == "max_linear_vel_")
                max_lin_vel = param.as_double();
            else if (param.get_name() == "max_angular_vel_")
                max_ang_vel = param.as_double();
        }

        // 将更新后的 PID 增益应用到对应的 PID 控制器实例中
        move_pid_->setGains(move_kp, move_ki, move_kd);
        heading_pid_->setGains(head_kp, head_ki, head_kd);

        // 更新类的成员变量，并把新的速度限制应用到 PID 控制器的输出限幅
        max_linear_vel_ = max_lin_vel;
        max_angular_vel_ = max_ang_vel;
        move_pid_->setLimits(max_linear_vel_, -max_linear_vel_);
        heading_pid_->setLimits(max_angular_vel_, -max_angular_vel_);

        return result;
    }

    // 测试用的定时器回调函数，可以用于周期性地执行一些调试任务
    void GbOmniPidPursuitController::time_callback()
    {
        try
        {
            auto transform = tf_buffer_->lookupTransform(
                "map", "base_link", tf2::TimePointZero, tf2::durationFromSec(0.1));

            // RCLCPP_INFO(this->get_logger(), "current_pose: [X: %.2f, Y: %.2f, Yaw: %.2f] ", transform.transform.translation.x, transform.transform.translation.y, tf2::getYaw(transform.transform.rotation));

            // RCLCPP_INFO(this->get_logger(), "current_pose: [X: %.2f, Y: %.2f, Z: %.2f]", transform.transform.translation.x, transform.transform.translation.y, transform.transform.translation.z);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

} // namespace gb_omni_pid_pursuit_controller

// 使用宏将这个类注册为 ROS 2 组件，使其可以被加载到可组合节点容器中
#include "rclcpp_components/register_node_macro.hpp"

RCLCPP_COMPONENTS_REGISTER_NODE(gb_omni_pid_pursuit_controller::GbOmniPidPursuitController)
