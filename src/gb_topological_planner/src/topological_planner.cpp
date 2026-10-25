/**
 * @file topological_planner.cpp
 * @brief 拓扑路径规划器节点实现
 *
 * 本节点实现了一个 ROS 2 服务，用于接收一个目标航点名称，
 * 并在预定义的拓扑地图上规划出一条可执行的物理路径。
 * 它的核心流程包括：
 * 1. 从 YAML 文件加载拓扑地图（航点及其连接关系）。
 * 2. 接收规划请求，并获取机器人当前位置。
 * 3. 使用 A* 算法在拓扑图上找到从当前最近航点到目标航点的最短路径（宏观路径）。
 * 4. 对宏观路径进行线性插值，生成一条密集的、可供底层控制器追踪的物理路径。
 */
#include "gb_topological_planner/topological_planner.hpp"
#include <queue>
#include <limits>
#include <algorithm>
#include <cmath>
#include <tf2/utils.h>
namespace gb_navigation
{
    TopologicalPlanner::TopologicalPlanner(const rclcpp::NodeOptions &options)
        : Node("topological_planner_node", options), interpolation_step_(0.05)
    {
        // 1. 初始化 TF 监听器，用于随时获取机器人当前物理位置
        tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this);

        // 2. 声明并获取拓扑地图文件的路径参数
        this->declare_parameter("map_path", "/home/mika/pointlio_lidar/pointlio_lidar/src/gb_topological_planner/config/topology_map.yaml");
        std::string map_path = this->get_parameter("map_path").as_string();

        // 加载拓扑地图，如果失败则记录致命错误并退出
        if (!loadTopologyMap(map_path))
        {
            RCLCPP_ERROR(this->get_logger(), "致命错误：拓扑地图加载失败！");
            return;
        }

        // 3. 启动“接线员”，监听行为树打来的算路电话
        planner_service_ = this->create_service<gb_interfaces::srv::GetTopologicalPath>(
            "get_topological_path",
            std::bind(&TopologicalPlanner::handle_plan_request, this, std::placeholders::_1, std::placeholders::_2));

        RCLCPP_INFO(this->get_logger(), "拓扑地图规划器已启动，随时准备算路！");
    }

    // ==========================================
    // 模块 1：解析 YAML 赛场法则
    // ==========================================
    /**
     * @brief 从 YAML 文件加载拓扑地图数据。
     * @details 该函数使用 yaml-cpp 库解析指定的 YAML 文件，提取航点（节点）的名称、坐标，
     *          以及航点之间的连接关系（边），并构建一个内存中的图表示（邻接表）。
     * @param yaml_path YAML 配置文件的绝对路径。
     * @return 如果地图成功加载并解析，返回 true；否则返回 false。
     */
    bool TopologicalPlanner::loadTopologyMap(const std::string &yaml_path)
    {
        try
        {
            YAML::Node config = YAML::LoadFile(yaml_path);
            YAML::Node map_node = config["topological_map"]; // 从刚刚读取的 YAML 配置文件中，提取出指定层级（名为 "topological_map"）的数据块。

            // 将 YAML 中的节点信息加载到 topology_graph_ （哈希表）
            for (YAML::const_iterator it = map_node["nodes"].begin(); it != map_node["nodes"].end(); ++it)
            {
                TopoNode node;
                node.name = it->first.as<std::string>();
                node.x = it->second["x"].as<double>();
                node.y = it->second["y"].as<double>();
                if (it->second["yaw"])
                {
                    node.has_fixed_yaw = true;
                    node.yaw = it->second["yaw"].as<double>();
                }
                else
                {
                    node.has_fixed_yaw = false;
                    node.yaw = 0.0;
                }
                topology_graph_[node.name] = node;
            }
            // 将 YAML 中的边信息加载到 topology_graph_ 的邻接表中
            for (const auto &edge : map_node["edges"])
            {
                std::string n1 = edge[0].as<std::string>();
                std::string n2 = edge[1].as<std::string>();
                topology_graph_[n1].neighbors.push_back(n2);
                topology_graph_[n2].neighbors.push_back(n1);
            }

            RCLCPP_INFO(this->get_logger(), "成功加载地图，共 %zu 个航点。", topology_graph_.size());
            return true;
        }
        catch (const YAML::Exception &e) // 捕获 yaml-cpp 解析异常
        {
            RCLCPP_ERROR(this->get_logger(), "YAML 解析炸了: %s", e.what());
            return false;
        }
    }

    // ==========================================
    // 模块 2：电话接听与主流程调度
    // ==========================================
    /**
     * @brief 处理路径规划服务请求的回调函数。
     * @details 这是拓扑规划的核心调度逻辑。它接收一个目标航点名称，然后依次执行：
     *          获取当前位姿 -> 寻找最近的起始航点 -> A*搜索宏观路径 -> 插值生成密集路径。
     * @param request 服务请求，包含目标航点名称 `target_waypoint`。
     * @param response 服务响应，包含规划是否成功 `success`、消息 `message` 和最终路径 `path`。
     */
    void TopologicalPlanner::handle_plan_request(
        const std::shared_ptr<gb_interfaces::srv::GetTopologicalPath::Request> request,
        std::shared_ptr<gb_interfaces::srv::GetTopologicalPath::Response> response)
    {
        RCLCPP_INFO(this->get_logger(), "收到行为树算路请求，目标: %s", request->target_waypoint.c_str());

        // 1. 检查目标点是否存在于图纸上
        if (topology_graph_.find(request->target_waypoint) == topology_graph_.end()) // 如果没有找到会返回一个 end() 迭代器，说明图纸上没有这个点
        {
            response->success = false;
            response->message = "查无此点！请检查 YAML 拼写。";
            return;
        }

        // 2. 探查机器人当前物理位置
        double current_x, current_y;
        if (!getCurrentPose(current_x, current_y))
        {
            response->success = false;
            response->message = "TF 树断裂，无法获取机器人当前位置！";
            return;
        }

        // 3. 找到距离机器人最近的航点作为起点
        std::string start_node_name = findClosestNode(current_x, current_y);
        RCLCPP_INFO(this->get_logger(), "当前最近航点判定为: %s", start_node_name.c_str());

        // 4. 使用 A* 图搜索算法计算出宏观路径（由拓扑航点组成的序列）
        std::vector<TopoNode> coarse_path = findShortestPath(start_node_name, request->target_waypoint);
        if (coarse_path.empty())
        {
            response->success = false;
            response->message = "这两点之间没有连通的道路！";
            return;
        }

        // 5. 极其关键的微观插值 (喂给 Pure Pursuit 胡萝卜)
        // 将机器人的当前真实物理位置作为路径的第一个点插入，
        // 这可以确保机器人从当前位置平滑地开始追踪，而不是突然跳到最近的航点。
        TopoNode real_start = {"real_start", current_x, current_y, false, 0.0, {}};
        coarse_path.insert(coarse_path.begin(), real_start);

        response->path = interpolatePath(coarse_path, interpolation_step_);
        response->success = true;
        response->message = "算路成功！";

        // 打印当前位置和路径最后一点的坐标信息
        if (!response->path.poses.empty())
        {
            double final_x = response->path.poses.back().pose.position.x;
            double final_y = response->path.poses.back().pose.position.y;
            double final_yaw = tf2::getYaw(response->path.poses.back().pose.orientation);
            RCLCPP_INFO(this->get_logger(), "规划完成！当前位置: (%.2f, %.2f) -> 路径终点: (%.2f, %.2f), 方向: %.2f", current_x, current_y, final_x, final_y, final_yaw);
        }
    }

    // ==========================================
    // 模块 3：底层算法库
    // ==========================================
    /**
     * @brief 获取机器人当前的全局位姿。
     * @details 使用 TF2 库查询从 "map" 坐标系到 "base_link" 坐标系的变换。
     * @param[out] x 用于存储机器人当前 x 坐标的引用。
     * @param[out] y 用于存储机器人当前 y 坐标的引用。
     * @return 如果成功获取位姿，返回 true；否则（如 TF 树中断）返回 false。
     */
    bool TopologicalPlanner::getCurrentPose(double &x, double &y)
    {
        try
        {
            auto transform = tf_buffer_->lookupTransform("map", "base_link", tf2::TimePointZero);
            x = transform.transform.translation.x;
            y = transform.transform.translation.y;
            return true;
        }
        catch (tf2::TransformException &ex)
        {
            RCLCPP_ERROR(this->get_logger(), "获取坐标失败: %s", ex.what());
            return false;
        }
    }

    /**
     * @brief 在拓扑图中查找距离给定物理坐标最近的航点。
     * @details 这是一个简单的线性搜索，遍历所有已知的拓扑航点，计算它们与当前物理位置的欧几里得距离，
     *          并返回距离最小的那个航点的名称。
     * @param current_x 当前 x 坐标。
     * @param current_y 当前 y 坐标。
     * @return 距离最近的航点的名称。
     */
    std::string TopologicalPlanner::findClosestNode(double current_x, double current_y)
    {
        std::string closest_name;
        double min_dist = std::numeric_limits<double>::max();

        for (const auto &pair : topology_graph_)
        {
            double dist = std::hypot(pair.second.x - current_x, pair.second.y - current_y);
            if (dist < min_dist)
            {
                min_dist = dist;
                closest_name = pair.first;
            }
        }
        return closest_name;
    }

    /**
     * @brief 使用 A* 算法在拓扑图中查找最短路径。
     * @details A* 是一种启发式图搜索算法。
     * 评估函数: f(n) = g(n) + h(n)
     * g(n): 从起点到节点 n 的已知【实际沉没代价】。
     * h(n): 从节点 n 到终点的【预估未来代价】（必须 <= 实际距离，算法才保证最优）。
     * @param start_name 起始航点的名称。
     * @param goal_name 目标航点的名称。
     * @return std::vector<TopoNode> 包含完整路径的数组。若不连通则返回空数组。
     */
    std::vector<TopoNode> TopologicalPlanner::findShortestPath(const std::string &start_name, const std::string &goal_name)
    {
        std::vector<TopoNode> path;

        // 1. 启发式函数 h(n)：使用 Lambda 表达式定义两点间的欧几里得距离 (直线距离)
        // 捕获 this 指针或 topology_graph_ 引用，确保能读取拓扑图坐标。
        auto heuristic = [&](const std::string &a, const std::string &b)
        {
            return std::hypot(topology_graph_[a].x - topology_graph_[b].x,
                              topology_graph_[a].y - topology_graph_[b].y);
        };

        // 2. 核心状态记录表 (使用哈希表实现 O(1) 的极速随机访问)
        std::unordered_map<std::string, double> g_score;
        std::unordered_map<std::string, double> f_score;
        std::unordered_map<std::string, std::string> came_from; // 记录"父节点"，用于回溯

        // 初始化：假设所有点都遥不可及 (无穷大)
        for (const auto &pair : topology_graph_)
        {
            g_score[pair.first] = std::numeric_limits<double>::infinity();
            f_score[pair.first] = std::numeric_limits<double>::infinity();
        }

        // 起点初始化：站在起点，沉没代价为 0，总代价全靠猜(h(n))
        g_score[start_name] = 0.0;
        f_score[start_name] = heuristic(start_name, goal_name);

        // 3. Open Set (待探索的边界集合)：使用优先队列 (Min-Heap)
        // C++ 的 priority_queue 默认是大顶堆，这里传入 '>' 强行将其翻转为【小顶堆】
        // 确保每次 top() 拿到的都是 f_score 最小（最有希望）的节点
        auto cmp = [&f_score](const std::string &left, const std::string &right)
        {
            return f_score[left] > f_score[right];
        };
        std::priority_queue<std::string, std::vector<std::string>, decltype(cmp)> open_set(cmp);

        // 辅助哈希表：因为 C++ 优先队列不支持 O(1) 的查找，用这个表来快速判断节点是否已在队列中
        std::unordered_map<std::string, bool> in_open_set;

        open_set.push(start_name);
        in_open_set[start_name] = true;

        // 4. A* 寻路主循环：只要还有待探索的节点，就继续搜
        while (!open_set.empty())
        {

            // 步骤 A：拔出最锐利的“矛”（f_score 最小的节点）
            std::string current = open_set.top();
            open_set.pop();
            in_open_set[current] = false;

            // 步骤 B：【胜利条件】戳中目标，立刻停止探索！
            if (current == goal_name)
            {
                std::string curr = goal_name;
                // 顺藤摸瓜：从终点一步步往回找父节点，直到回到起点
                while (curr != start_name)
                {
                    path.push_back(topology_graph_[curr]);
                    curr = came_from[curr];
                }
                path.push_back(topology_graph_[start_name]);

                // 因为是倒着摸回去的，最后必须将数组翻转
                std::reverse(path.begin(), path.end());
                return path;
            }

            // 步骤 C：向四周扩散（遍历当前节点的所有合法邻居）
            for (const std::string &neighbor : topology_graph_[current].neighbors)
            {

                // 计算从 current 走到 neighbor 的物理路程消耗
                double edge_weight = std::hypot(topology_graph_[current].x - topology_graph_[neighbor].x,
                                                topology_graph_[current].y - topology_graph_[neighbor].y);

                // 核心松弛操作 (Relaxation)：算一算，如果走 current 这条路去 neighbor，总耗费是多少？
                double tentative_g_score = g_score[current] + edge_weight;

                // 步骤 D：发现捷径！如果新的耗费 < 之前记录的耗费
                if (tentative_g_score < g_score[neighbor])
                {

                    // 毫不犹豫地抛弃旧路线，更新为这条捷径
                    came_from[neighbor] = current;
                    g_score[neighbor] = tentative_g_score;
                    // 更新该邻居的综合预估得分：实际已走路程 + 直线预估路程
                    f_score[neighbor] = g_score[neighbor] + heuristic(neighbor, goal_name);

                    // 如果这个邻居还没被放进待探索队列，就把它加进去排队
                    if (!in_open_set[neighbor])
                    {
                        open_set.push(neighbor);
                        in_open_set[neighbor] = true;
                    }
                }
            }
        }

        // 5. 探索完所有连通的节点都找不到目标，宣告失败
        RCLCPP_WARN(rclcpp::get_logger("A_Star"), "A* 搜索失败：[%s] 与 [%s] 之间不存在通路！", start_name.c_str(), goal_name.c_str());
        return path; // 返回空数组，外层逻辑接到空数组应执行急停或重新规划
    }

    /**
     * @brief 将宏观的、稀疏的拓扑路径插值为密集的物理路径。
     * @details 该函数接收一个由拓扑航点组成的粗略路径，然后在相邻航点之间进行线性插值，
     *          生成一系列间距约为 `step_size` 的密集点。每个插值点的朝向都指向其所在线段的下一个点。
     *          这是将宏观规划结果转化为底层控制器可执行指令的关键步骤。
     * @param coarse_path 由 A* 算法生成的、包含宏观航点的粗略路径。
     * @param step_size 插值的步长，单位为米。
     * @return 一个 `nav_msgs::msg::Path` 对象，包含一系列密集的 `PoseStamped`。
     */
    nav_msgs::msg::Path TopologicalPlanner::interpolatePath(const std::vector<TopoNode> &coarse_path, double step_size)
    {
        nav_msgs::msg::Path dense_path;
        dense_path.header.frame_id = "map";
        dense_path.header.stamp = this->now();

        for (size_t i = 0; i < coarse_path.size() - 1; ++i)
        {
            double x1 = coarse_path[i].x;
            double y1 = coarse_path[i].y;
            double x2 = coarse_path[i + 1].x;
            double y2 = coarse_path[i + 1].y;

            double dist = std::hypot(x2 - x1, y2 - y1);
            int num_steps = std::max(1, static_cast<int>(std::ceil(dist / step_size))); // 计算需要插值的步数

            double yaw = std::atan2(y2 - y1, x2 - x1);
            tf2::Quaternion q;
            q.setRPY(0, 0, yaw);

            for (int j = 0; j < num_steps; ++j)
            {
                geometry_msgs::msg::PoseStamped pose;
                pose.pose.position.x = x1 + (x2 - x1) * (static_cast<double>(j) / num_steps);
                pose.pose.position.y = y1 + (y2 - y1) * (static_cast<double>(j) / num_steps);
                pose.pose.orientation.x = q.x();
                pose.pose.orientation.y = q.y();
                pose.pose.orientation.z = q.z();
                pose.pose.orientation.w = q.w();
                dense_path.poses.push_back(pose);
            }
        }

        // 为确保路径的完整性，显式地将最后一个航点添加到路径末尾。
        geometry_msgs::msg::PoseStamped final_pose;
        // 1. 无论如何，先设置最终点的物理坐标
        final_pose.pose.position.x = coarse_path.back().x;
        final_pose.pose.position.y = coarse_path.back().y;

        // 2. 根据情况设置最终点的朝向
        if (coarse_path.back().has_fixed_yaw)
        {
            // 如果YAML中为终点指定了固定朝向，则使用该朝向
            tf2::Quaternion final_q;
            final_q.setRPY(0, 0, coarse_path.back().yaw);
            final_pose.pose.orientation = tf2::toMsg(final_q);
        }
        else if (!dense_path.poses.empty())
        {
            // 如果没有指定，则继承路径上一个点的朝向，以保证姿态连续
            final_pose.pose.orientation = dense_path.poses.back().pose.orientation;
        }
        dense_path.poses.push_back(final_pose);

        // RCLCPP_INFO(this->get_logger(), "final_x: %.2f, final_y: %.2f", final_pose.pose.position.x, final_pose.pose.position.y);

        // RCLCPP_INFO(this->get_logger(), "--- 打印完整密集路径点 (共 %zu 个) ---", dense_path.poses.size());
        // for (size_t i = 0; i < dense_path.poses.size(); i++)
        // {
        //     const auto &p = dense_path.poses[i].pose;
        //     RCLCPP_INFO(this->get_logger(), "  点 %zu: [X: %.2f, Y: %.2f, Yaw: %.2f]", i, p.position.x, p.position.y, tf2::getYaw(p.orientation));
        // }
        // RCLCPP_INFO(this->get_logger(), "--- 打印结束 ---");

        return dense_path;
    }

} // namespace gb_navigation

// 使用宏将这个类注册为 ROS 2 组件，使其可以被加载到可组合节点容器中。
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(gb_navigation::TopologicalPlanner)
