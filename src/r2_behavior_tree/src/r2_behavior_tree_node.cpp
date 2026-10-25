#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/loggers/bt_cout_logger.h"
#include "ament_index_cpp/get_package_share_directory.hpp"

// 包含你的自定义节点头文件
#include "r2_behavior_tree/move_chassis_node.hpp"
#include "r2_behavior_tree/get_topological_path_node.hpp"

/*** 这个文件是整个行为树的入口点，负责加载 XML 文件、注册节点类型，并启动行为树的执行循环。(特别注意的是这个文件中的各个behavior tree节点的构造函数需要一个 rclcpp::Node::SharedPtr 参数，所以我们在 main 函数中创建了一个 ROS 2 节点，并把它传递给每个节点的构造函数。但是在 XML 文件中，我们只需要使用节点名称 "MoveChassis"，不需要关心构造函数的细节。行为树工厂会自动处理这些参数传递的细节。还有一个重要的细节是，在 main 循环中，我们必须调用 rclcpp::spin_some(node) 来处理 ROS 2 的回调函数，这样 Action Client 才能接收到底盘 PID 服务的结果，否则行为树中的 MoveChassisNode 将永远无法完成任务！除此之外这个行为树节点跟ros2中的节点不一样，它是一个行为树节点，专门用来在行为树中执行特定的动作或条件检查。它的生命周期由行为树管理，而不是由 ROS 2 的节点管理。这就是为什么我们需要在构造函数中传递一个 ROS 2 节点指针，以便它能够与 ROS 2 的通信机制（如 Action Client）进行交互，但它本身和ROS 2 节点完全不是一个东西只是名字相似都叫节点。)*/

int main(int argc, char **argv)
{
    // 初始化 ROS 2
    rclcpp::init(argc, argv);

    // 创建一个名为 "r2_behavior_tree" 的 ROS 2 节点，行为树将在这个节点中运行
    auto node = std::make_shared<rclcpp::Node>("r2_behavior_tree");
    RCLCPP_INFO(node->get_logger(), "ROS 2 行为树节点已启动");

    // 创建行为树工厂
    BT::BehaviorTreeFactory factory;

    // --- 注册所有自定义的行为树节点 ---
    // 使用 NodeBuilder (Lambda表达式) 来捕获外部的 'node' 指针，
    // 并在工厂创建节点实例时，将其传递给节点的构造函数。
    // 这是在 BT.CPP v3 中向节点构造函数传递额外参数的标准方法。

    // 注册路径规划节点 ("GetTopologicalPath" 是在 XML 中使用的名字)
    BT::NodeBuilder get_path_builder =
        [node](const std::string &name, const BT::NodeConfiguration &config)
    {
        return std::make_unique<GetTopologicalPath>(name, config, node);
    };
    factory.registerBuilder<GetTopologicalPath>("GetTopologicalPath", get_path_builder);

    // 注册底盘移动节点 ("MoveChassis" 是在 XML 中使用的名字)
    BT::NodeBuilder move_chassis_builder =
        [node](const std::string &name, const BT::NodeConfiguration &config)
    {
        return std::make_unique<MoveChassisNode>(name, config, node);
    };
    factory.registerBuilder<MoveChassisNode>("MoveChassis", move_chassis_builder);

    // 获取行为树 XML 文件的路径
    // 假设你的 XML 文件将位于 "install/r2_behavior_tree/share/r2_behavior_tree/behavior_trees/my_tree.xml"
    std::string package_path = ament_index_cpp::get_package_share_directory("gb2026_bringup");
    // --- 注意：请确保你的 launch 文件或代码逻辑中创建了这个目录和文件 ---
    std::string tree_xml_path = package_path + "/behavior_tree/r2_tree.xml";

    RCLCPP_INFO(node->get_logger(), "将要加载的行为树文件: %s", tree_xml_path.c_str());

    // 从文件创建行为树
    auto tree = factory.createTreeFromFile(tree_xml_path);

    // 添加一个简单的日志记录器，用于在控制台打印树的状态变化
    BT::StdCoutLogger logger_cout(tree);

    // 以 10Hz 的频率循环执行行为树，直到它完成
    rclcpp::Rate rate(10);
    // 初始化状态
    BT::NodeStatus status = BT::NodeStatus::RUNNING;

    while (rclcpp::ok() && status == BT::NodeStatus::RUNNING)
    {
        // 【最关键的一行！】处理 ROS 2 网络队列中的回调函数
        // 如果没有这行，Action Client 永远收不到底盘的 Result！
        rclcpp::spin_some(node);

        // 推动行为树向下执行一步
        status = tree.tickRoot();

        // 睡眠等待下一帧
        rate.sleep();
    }

    // 循环结束后，打印最终结果
    if (status == BT::NodeStatus::SUCCESS)
    {
        RCLCPP_INFO(node->get_logger(), "战术执行成功！");
    }
    else if (status == BT::NodeStatus::FAILURE)
    {
        RCLCPP_ERROR(node->get_logger(), "执行失败，树被中断或报错。");
    }
    else
    {
        RCLCPP_WARN(node->get_logger(), "执行中断或其他原因导致未完成。");
    }
    rclcpp::shutdown();
    return 0;
}
