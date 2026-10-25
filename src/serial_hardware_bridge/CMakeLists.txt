# 指定构建该项目所需的最低 CMake 版本。
cmake_minimum_required(VERSION 3.8)
# 定义项目名称，这个名称会在多个地方被引用，例如创建库和可执行文件时。
project(serial_hardware_bridge)

# 设置 C++ 语言标准为 C++17。
set(CMAKE_CXX_STANDARD 17)
# 强制要求必须使用 C++17 标准，如果编译器不支持则会报错。
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 检查编译器是否为 GCC (GNUCXX) 或 Clang。
# 如果是，则添加一些推荐的编译选项以提高代码质量和性能。
# -Wall: 开启所有常用警告。
# -Wextra: 开启 -Wall 未包含的额外警告。
# -Wpedantic: 开启严格遵循 ISO C++ 标准的警告。
# -O3: 开启最高级别的编译优化。
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# 查找 ament_cmake_auto 包，这是 ROS 2 ament 构建系统的一个自动化工具。
# REQUIRED 表示如果找不到这个包，CMake 将会失败。
find_package(ament_cmake_auto REQUIRED)
# ament_auto_find_build_dependencies 会自动查找并处理在 package.xml 文件中声明的所有构建依赖项。
# 这极大地简化了依赖管理。
ament_auto_find_build_dependencies()

# 显式查找非 ament 包的依赖项。这里我们需要 Boost 库的 system 组件。
find_package(Boost REQUIRED COMPONENTS system)

# ament_auto_add_library 是一个非常方便的宏，它会自动：
# 1. 创建一个与项目同名的共享库（SHARED library）。
# 2. 将 'src' 目录下的所有源文件 (.cpp) 添加到这个库中。
ament_auto_add_library(${PROJECT_NAME} SHARED
  DIRECTORY src
)

# 为我们创建的库（${PROJECT_NAME}）指定头文件包含目录。
# PUBLIC 意味着链接到这个库的其他目标也需要包含这些目录。
# $<BUILD_INTERFACE:...>: 指定在构建时（即在工作空间内）的头文件路径。
# $<INSTALL_INTERFACE:...>: 指定在安装后（即在 install 目录下）的头文件路径。
# 这种生成器表达式的写法是现代 CMake 的推荐做法，可以确保在不同场景下都能正确找到头文件。
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
# 将我们的库链接到它所依赖的库。这里是链接到之前找到的 Boost::system。
target_link_libraries(${PROJECT_NAME} Boost::system)


# 将我们创建的库注册为一个 ROS 2 的可组合节点（Composable Node）。
# 这是 ROS 2 的一个核心特性，允许将多个节点加载到同一个进程中以提高性能。
# PLUGIN: 指定作为插件的 C++ 类的完整命名空间和名称。
# EXECUTABLE: CMake 会自动创建一个名为 "serial_hardware_bridge_node" 的可执行文件，用于独立运行这个节点。
rclcpp_components_register_node(${PROJECT_NAME}
  PLUGIN "serial_hardware_bridge::SerialHardwareBridgeNode"
  EXECUTABLE serial_hardware_bridge_node
)


# ament_auto_package 是另一个强大的自动化宏，它会处理所有安装和导出相关的规则。
# USE_SCOPED_HEADER_INSTALL_DIR: 将头文件安装到带包名的子目录中（例如 install/include/serial_hardware_bridge/），避免命名冲突。
# INSTALL_TO_SHARE: 安装 package.xml 等资源文件到 share 目录。
# launch: 自动查找并安装 'launch' 目录下的所有 launch 文件。
ament_auto_package(
  USE_SCOPED_HEADER_INSTALL_DIR
  INSTALL_TO_SHARE
  launch
)


# cmake_minimum_required(VERSION 3.8)
# project(serial_hardware_bridge)

# if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#   add_compile_options(-Wall -Wextra -Wpedantic)
# endif()

# find_package(ament_cmake REQUIRED)
# find_package(rclcpp REQUIRED)
# find_package(rclcpp_components REQUIRED)
# find_package(geometry_msgs REQUIRED)
# find_package(Boost REQUIRED COMPONENTS system)

# # 包含目录
# include_directories(include)

# # 创建库 (符合组件化设计)
# add_library(${PROJECT_NAME} SHARED
#   src/serial_hardware_bridge.cpp
# )


# target_include_directories(${PROJECT_NAME} PUBLIC
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#   $<INSTALL_INTERFACE:include>
# )

# target_link_libraries(${PROJECT_NAME} Boost::system)

# ament_target_dependencies(${PROJECT_NAME}
#   rclcpp
#   rclcpp_components
#   geometry_msgs
# )

# # 注册组件
# rclcpp_components_register_node(${PROJECT_NAME}
#   PLUGIN "serial_hardware_bridge::SerialHardwareBridgeNode"
#   EXECUTABLE serial_hardware_bridge_node
# )

# # 安装规则
# install(TARGETS ${PROJECT_NAME}
#   ARCHIVE DESTINATION lib
#   LIBRARY DESTINATION lib
#   RUNTIME DESTINATION lib/${PROJECT_NAME}
# )

# install(DIRECTORY include/
#   DESTINATION include
# )

# ament_export_include_directories(include)
# ament_export_libraries(${PROJECT_NAME})
# ament_package()


