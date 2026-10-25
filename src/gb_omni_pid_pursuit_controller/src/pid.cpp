/**
 * @file pid.cpp
 * @brief 标准 PID 控制器实现文件
 *
 * 本文件提供了一个标准的离散时间 PID 控制器实现，包含比例 (P)、积分 (I) 和微分 (D) 控制，
 * 并且加入了积分抗饱和 (Anti-windup) 机制以及输出限幅功能。主要用于计算机器人的线速度与角速度控制指令。
 */

#include "gb_omni_pid_pursuit_controller/pid.hpp"
#include <cmath> // 引入 std::abs

namespace gb_omni_pid_pursuit_controller
{

    /**
     * @brief 构造函数：初始化 PID 控制器参数
     *
     * @param dt 控制周期/步长 (秒)
     * @param max 控制器输出的最大上限
     * @param min 控制器输出的最小下限
     * @param kp 比例增益 (Proportional)
     * @param ki 积分增益 (Integral)
     * @param kd 微分增益 (Derivative)
     */
    PID::PID(double dt, double max, double min, double kp, double ki, double kd)
        : dt_(dt), max_(max), min_(min), kp_(kp), ki_(ki), kd_(kd), pre_error_(0), integral_(0) {}

    /**
     * @brief 执行单步 PID 计算
     *
     * @param set_point 目标设定值 (Target/Goal)
     * @param pv 当前过程变量 (Process Variable/当前实际反馈值)
     * @return double 计算得出的控制指令输出
     */
    double PID::calculate(double set_point, double pv)
    {
        double raw_error = set_point - pv;

        double raw_derivative = (raw_error - pre_error_) / dt_;

        double filtered_derivative = alpha_ * raw_derivative + (1.0 - alpha_) * pre_derivative_;

        double d_out = kd_ * filtered_derivative;

        // 3. 误差死区判断 (分离 P 和 I 的使用误差)
        double error_for_p_i = raw_error;

        if (std::abs(raw_error) < deadband_)
        {
            error_for_p_i = 0.0;
            integral_ = 0.0;           // 清空积分
            d_out = 0.0;               // 死区内直接切断阻尼，防止残留抽动
            filtered_derivative = 0.0; // 清空滤波惯性
        }

        // 1. 比例项 (P)
        double p_out = kp_ * error_for_p_i;

        // 2. 积分项 (I) 累加
        integral_ += error_for_p_i * dt_;
        double i_out = ki_ * integral_;

        // 积分抗饱和 (Anti-windup)：限制积分项的累计，防止其单独超过控制器的输出限幅
        if (ki_ > 1e-6)
        {
            if (i_out > max_)
            {
                integral_ = max_ / ki_;
                i_out = max_;
            }
            else if (i_out < min_)
            {
                integral_ = min_ / ki_;
                i_out = min_;
            }
        }

        // 4. 总输出合成
        double output = p_out + i_out + d_out;

        // 全局输出限幅 (Clamping)
        if (output > max_)
            output = max_;
        else if (output < min_)
            output = min_;

        // 更新上一时刻误差，供下次微分计算使用
        pre_error_ = raw_error;
        return output;
    }

    /**
     * @brief 动态更新 PID 增益参数
     * @note 供 ROS 2 动态调参 (dynamic reconfigure) 机制调用
     */
    void PID::setGains(double kp, double ki, double kd)
    {
        kp_ = kp;
        ki_ = ki;
        kd_ = kd;
    }

    /**
     * @brief 动态更新 PID 输出限幅
     */
    void PID::setLimits(double max, double min)
    {
        max_ = max;
        min_ = min;
    }

    /**
     * @brief 手动设置当前的积分误差累计值
     * @param sum_error 欲设定的积分误差量
     */
    void PID::setSumError(double sum_error) { integral_ = sum_error; }

    /**
     * @brief 重置控制器状态 (清空历史误差与积分累计)
     * @note 当控制器被中断或分配了新的目标时，必须调用此函数防止历史积分导致机器人的瞬时冲动
     */
    void PID::reset()
    {
        pre_error_ = 0.0;
        integral_ = 0.0;
    }

} // namespace gb_omni_pid_pursuit_controller