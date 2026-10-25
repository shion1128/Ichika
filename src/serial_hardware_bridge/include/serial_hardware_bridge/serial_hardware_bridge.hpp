#ifndef SERIAL_HARDWARE_BRIDGE__SERIAL_HARDWARE_BRIDGE_HPP_
#define SERIAL_HARDWARE_BRIDGE__SERIAL_HARDWARE_BRIDGE_HPP_

#include <string>
#include <memory>
#include <mutex>
#include <boost/asio.hpp>

#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/bool.hpp>
#include "geometry_msgs/msg/twist.hpp"
#include "protocol.hpp"

namespace serial_hardware_bridge
{

    class SerialHardwareBridgeNode : public rclcpp::Node
    {
    public:
        explicit SerialHardwareBridgeNode(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());
        ~SerialHardwareBridgeNode() override;

    private:
    
        void cmdVelCallback(const geometry_msgs::msg::Twist::ConstSharedPtr msg);

        // 内部工具函数：计算简易校验和
        uint8_t calculateChecksum(const uint8_t *data, size_t length);

        // ROS 2 通讯接口
        rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;

        // 定时器：统一发送
        rclcpp::TimerBase::SharedPtr send_timer_;

        // Boost Asio 串口对象
        boost::asio::io_context io_context_;
        std::unique_ptr<boost::asio::serial_port> serial_port_;

        // 保护串口写入和状态变量的互斥锁
        std::mutex serial_mutex_;

        // 参数
        std::string port_name_;
        int baud_rate_;
    };

} // namespace serial_hardware_bridge

#endif // SERIAL_HARDWARE_BRIDGE__SERIAL_HARDWARE_BRIDGE_HPP_
