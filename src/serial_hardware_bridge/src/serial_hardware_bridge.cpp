#include "serial_hardware_bridge/serial_hardware_bridge.hpp"
#include <iostream>
#include <chrono>
#include <memory>

using namespace std::chrono_literals;

namespace serial_hardware_bridge
{

    SerialHardwareBridgeNode::SerialHardwareBridgeNode(const rclcpp::NodeOptions &options)
        : Node("serial_hardware_bridge", options)
    {
        // 1. 声明并获取参数
        this->declare_parameter<std::string>("port_name", "/dev/ttyACM0");
        this->declare_parameter<int>("baud_rate", 115200);

        this->get_parameter("port_name", port_name_);
        this->get_parameter("baud_rate", baud_rate_);

        // 2. 初始化 Boost Asio 串口
        try
        {
            serial_port_ = std::make_unique<boost::asio::serial_port>(io_context_, port_name_);
            serial_port_->set_option(boost::asio::serial_port_base::baud_rate(baud_rate_));
            serial_port_->set_option(boost::asio::serial_port_base::character_size(8));
            serial_port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
            serial_port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
            serial_port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

            RCLCPP_INFO(this->get_logger(), "串口 %s 初始化成功，波特率: %d", port_name_.c_str(), baud_rate_);
        }
        catch (const std::exception &e)
        {
            RCLCPP_ERROR(this->get_logger(), "串口初始化失败: %s. 请检查权限 (sudo chmod 777 %s) 或端口名。", e.what(), port_name_.c_str());
        }

        // cmd_vel 是控制指令，必须使用 Reliable QoS，否则与 ros2 topic pub 默认的
        // Reliable 发布方不匹配，订阅永远收不到消息（SensorDataQoS 为 BestEffort 不兼容）。
        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel",
            rclcpp::QoS(rclcpp::KeepLast(10)),
            [this](const geometry_msgs::msg::Twist::ConstSharedPtr msg)
            {
                return this->cmdVelCallback(msg);
            });
    }

    SerialHardwareBridgeNode::~SerialHardwareBridgeNode()
    {
        if (serial_port_ && serial_port_ ->is_open())
        {
            ChassisControlPacket stop_pkt;
            stop_pkt.vx = 0;
            stop_pkt.vy = 0;
            stop_pkt.vw = 0;
            // 与 cmdVelCallback 保持一致：校验和只覆盖 vx+vy+vw（不包含帧头和帧尾）
            stop_pkt.crc8 = calculateChecksum(reinterpret_cast<uint8_t *>(&stop_pkt.vx),
                                              sizeof(stop_pkt.vx) + sizeof(stop_pkt.vy) + sizeof(stop_pkt.vw));
            boost::asio::write(*serial_port_, boost::asio::buffer(&stop_pkt, sizeof(stop_pkt)));

            serial_port_->close();
        }
    }

    void SerialHardwareBridgeNode::cmdVelCallback(const geometry_msgs::msg::Twist::ConstSharedPtr msg)
    {
        if (!serial_port_ || !serial_port_->is_open())
        {
            return;
        }
        

        ChassisControlPacket pkt;

        // 将单位从 m/s 转换为 mm/s 并转为 int16_t
        pkt.vx = static_cast<int16_t>(msg->linear.x * 1000.0);
        pkt.vy = static_cast<int16_t>(msg->linear.y * 1000.0);
        pkt.vw = static_cast<int16_t>(msg->angular.z * 1000.0);

        // 计算校验和 (不包含帧头和帧尾)
        uint8_t *payload_ptr = reinterpret_cast<uint8_t *>(&pkt.vx);
        size_t payload_len = sizeof(pkt.vx) + sizeof(pkt.vy) + sizeof(pkt.vw);
        pkt.crc8 = calculateChecksum(payload_ptr, payload_len);

        // 发送到底层硬件
        try
        {
            boost::asio::write(*serial_port_, boost::asio::buffer(&pkt, sizeof(pkt)));
        }
        catch (const std::exception &e)
        {
            RCLCPP_ERROR(this->get_logger(), "串口发送异常: %s", e.what());
        }
    }

    uint8_t SerialHardwareBridgeNode::calculateChecksum(const uint8_t *data, size_t length)
    {
        uint8_t sum = 0;
        for (size_t i = 0; i < length; ++i)
        {
            sum += data[i];
        }
        return sum;
    }

} // namespace serial_hardware_bridge

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(serial_hardware_bridge::SerialHardwareBridgeNode)
