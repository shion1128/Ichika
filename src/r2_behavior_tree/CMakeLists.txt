cmake_minimum_required(VERSION 3.8)
project(r2_behavior_tree)

# 1. 编译选项
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# 2. 手动查找所有依赖项 (绝对不会漏)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(behaviortree_cpp_v3 REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(nav2_behavior_tree REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(gb_interfaces REQUIRED)

# 3. 创建可执行文件
add_executable(${PROJECT_NAME}_node
  src/r2_behavior_tree_node.cpp
)

# 4. 指定头文件路径
target_include_directories(${PROJECT_NAME}_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 5. 显式链接库文件
ament_target_dependencies(${PROJECT_NAME}_node
  rclcpp
  std_msgs
  geometry_msgs
  behaviortree_cpp_v3
  ament_index_cpp
  nav2_behavior_tree
  nav2_msgs
  gb_interfaces
)

# 6. 【核心：强制物理安装】
# 这句指令强制要求把节点安装到 ROS 2 标准的 lib 目录下！
install(TARGETS ${PROJECT_NAME}_node
  DESTINATION lib/${PROJECT_NAME}
)

# 6.1 安装 launch 文件夹到 share 目录
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

# 7. 测试与打包
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

# cmake_minimum_required(VERSION 3.8)
# project(r2_behavior_tree)

# # 1. 显式设置 C++ 语言标准为 C++17 (ROS 2 Humble 的标配)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # 2. 开启严谨的警告，以及针对高性能 CPU 的指令集与最高级别优化 (-O3 -march=native)
# if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#   add_compile_options(-Wall -Wextra -Wpedantic -O3)
# endif()

# # 3. 启用全自动构建宏，并自动查找 package.xml 中的所有依赖
# find_package(ament_cmake_auto REQUIRED)
# ament_auto_find_build_dependencies()

# # 创建可执行文件目标
# ament_auto_add_executable(${PROJECT_NAME}_node
#   src/r2_behavior_tree_node.cpp
# )

# # 为目标添加自己的 include 目录，这样编译器才能找到 "r2_behavior_tree/move_chassis_node.hpp"
# target_include_directories(r2_behavior_tree_node PUBLIC
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#   $<INSTALL_INTERFACE:include>
# )

# # 测试配置
# if(BUILD_TESTING)
#   find_package(ament_lint_auto REQUIRED)
#   ament_lint_auto_find_test_dependencies()
# endif()

# # 自动处理所有安装规则
# ament_auto_package(
#   INSTALL_TO_SHARE launch
# )


# # 指定构建该项目所需的最低 CMake 版本。
# # CMake 3.8 或更高版本是必需的。
# cmake_minimum_required(VERSION 3.8) 
# # 定义项目名称。这个名称将在后续的 CMake 命令中被引用，例如用于构建可执行文件或库。
# project(r2_behavior_tree) 

# # 检查编译器是否为 GNU C++ (GCC) 或 Clang。
# # 如果是，则添加一些常用的编译选项，以提高代码质量和捕获潜在错误。
# # -Wall: 开启所有常用的警告。
# # -Wextra: 开启 -Wall 未包含的额外警告。
# # -Wpedantic: 开启严格遵循 ISO C++ 标准的警告。
# # 注意：这个文件中缺少 -O3 优化选项，可以考虑添加。
# if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") 
#   add_compile_options(-Wall -Wextra -Wpedantic) 
# endif() 

# # ----- 查找依赖项 -----
# # 查找 ament_cmake 包。这是 ROS 2 构建系统的核心组件，提供了 CMake 函数和宏来处理 ROS 2 包的构建、安装和导出。
# # REQUIRED 表示如果找不到这个包，CMake 将会停止并报错。
# find_package(ament_cmake REQUIRED)
# # 查找 rclcpp 包。rclcpp 是 ROS 2 的 C++ 客户端库，用于编写 ROS 2 节点。
# # REQUIRED 表示如果找不到这个包，CMake 将会停止并报错。
# find_package(rclcpp REQUIRED)

# # 添加一个可执行文件目标。
# # r2_behavior_tree_node: 这是可执行文件的名称。
# # src/r2_behavior_tree_node.cpp: 这是构成该可执行文件的源文件。
# add_executable(r2_behavior_tree_node src/r2_behavior_tree_node.cpp) 
# # 为可执行文件指定头文件包含目录。
# # PUBLIC: 表示这些目录不仅在编译 r2_behavior_tree_node 时需要，任何链接到 r2_behavior_tree_node 的目标也需要这些目录。
# # $<BUILD_INTERFACE:...>: 指定在构建时（即在工作区内编译时）的头文件路径。
# # $<INSTALL_INTERFACE:include>: 指定在安装后（即在 install 目录中）的头文件路径。
# # 这种生成器表达式的使用是现代 CMake 的推荐做法，确保在不同情境下都能正确解析头文件路径。
# target_include_directories(r2_behavior_tree_node PUBLIC
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#   $<INSTALL_INTERFACE:include>)
# # 显式要求编译器为该目标启用 C99 标准（针对 C 代码，如果存在）和 C++17 标准（针对 C++ 代码）。
# # PUBLIC 意味着任何链接到此目标的其他目标也将继承这些编译特性要求。
# target_compile_features(r2_behavior_tree_node PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17
# # 将可执行文件 r2_behavior_tree_node 链接到 ROS 2 的 rclcpp 库。
# # ament_target_dependencies 是 ament_cmake 提供的一个宏，用于处理 ROS 包之间的依赖链接。
# ament_target_dependencies(
#   r2_behavior_tree_node
#   "rclcpp"
# )

# # 安装规则：将生成的可执行文件安装到指定位置。
# # TARGETS r2_behavior_tree_node: 指定要安装的目标。
# # DESTINATION lib/${PROJECT_NAME}: 指定安装路径。这通常会将可执行文件安装到 ROS 2 环境的 `install/<package_name>/lib/<package_name>/` 目录下。
# install(TARGETS r2_behavior_tree_node
#   DESTINATION lib/${PROJECT_NAME})

# # ----- 测试配置 -----
# # 仅当 BUILD_TESTING 变量为真时（例如，通过 `colcon test` 命令触发）才执行以下测试相关的配置。
# if(BUILD_TESTING)
# # 查找 ament_lint_auto 包，它会自动查找和配置 ROS 2 的代码风格检查和静态分析工具。
#   find_package(ament_lint_auto REQUIRED)
#   # the following line skips the linter which checks for copyrights
#   # comment the line when a copyright and license is added to all source files
#   set(ament_cmake_copyright_FOUND TRUE)
#   # the following line skips cpplint (only works in a git repo)
#   # comment the line when this package is in a git repo and when
#   # a copyright and license is added to all source files
#   set(ament_cmake_cpplint_FOUND TRUE)
# # 自动查找并处理在 package.xml 文件中声明的测试依赖项（例如 `ament_lint_common`）。
#   ament_lint_auto_find_test_dependencies()
# endif()

# # 这是一个由 ament_cmake 提供的宏，用于完成 ROS 2 包的安装和导出。
# # 它会处理包元数据、安装规则的导出以及其他必要的文件（如 package.xml）。
# # 对于使用 `ament_cmake_auto` 的包，通常会替换为 `ament_auto_package()` 以获得更多自动化。
# # 但由于此文件显式使用了 `find_package(ament_cmake)` 而非 `ament_cmake_auto`，因此 `ament_package()` 是合适的。
# ament_package()

# 1. 基础环境与项目声明 (环境初始化)
# 2. 编译器规则设定 (安全护栏)
# 3. 寻找外部依赖 (找帮手)
# 4. 核心目标构建 (造零件)
# 5. 依赖链接 (组装)
# 6. 安装规则定义 (发货交接)
# 7. 测试与打包收尾 (质检与封箱)
