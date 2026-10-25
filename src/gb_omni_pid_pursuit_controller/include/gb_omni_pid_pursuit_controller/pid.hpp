#ifndef GB_OMNI_PID_PURSUIT_CONTROLLER_PID_HPP_
#define GB_OMNI_PID_PURSUIT_CONTROLLER_PID_HPP_

namespace gb_omni_pid_pursuit_controller
{

    class PID
    {
    public:
        PID(double dt, double max, double min, double kp, double ki, double kd);
        ~PID() = default;

        double calculate(double set_point, double pv);
        void setSumError(double sum_error);
        void setGains(double kp, double ki, double kd);
        void setLimits(double max, double min);
        void reset();

    private:
        double dt_;
        double max_;
        double min_;
        double kp_;
        double ki_;
        double kd_;
        double pre_error_;
        double integral_;

        // ===== 本次工业级升级新增的变量 =====
        double pre_derivative_{0.0}; // 记录上一时刻的滤波后微分值，用于低通滤波计算
        double deadband_{0.003};     // 误差死区阈值（默认约 1度 或 1.7cm），用于消除传感器底噪导致的抖动
        double alpha_{0.1};          // 一阶低通滤波系数 (0.0~1.0)，越小越平滑，越大延迟越小
    };

} // namespace gb_omni_pid_pursuit_controller

#endif // GB_OMNI_PID_PURSUIT_CONTROLLER_PID_HPP_