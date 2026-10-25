// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from gb_interfaces:srv/GetTopologicalPath.idl
// generated code does not contain a copyright notice

#ifndef GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__STRUCT_HPP_
#define GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Request __attribute__((deprecated))
#else
# define DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Request __declspec(deprecated)
#endif

namespace gb_interfaces
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct GetTopologicalPath_Request_
{
  using Type = GetTopologicalPath_Request_<ContainerAllocator>;

  explicit GetTopologicalPath_Request_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->target_waypoint = "";
    }
  }

  explicit GetTopologicalPath_Request_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : target_waypoint(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->target_waypoint = "";
    }
  }

  // field types and members
  using _target_waypoint_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _target_waypoint_type target_waypoint;

  // setters for named parameter idiom
  Type & set__target_waypoint(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->target_waypoint = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator> *;
  using ConstRawPtr =
    const gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Request
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Request
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Request_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetTopologicalPath_Request_ & other) const
  {
    if (this->target_waypoint != other.target_waypoint) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetTopologicalPath_Request_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetTopologicalPath_Request_

// alias to use template instance with default allocator
using GetTopologicalPath_Request =
  gb_interfaces::srv::GetTopologicalPath_Request_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace gb_interfaces


// Include directives for member types
// Member 'path'
#include "nav_msgs/msg/detail/path__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Response __attribute__((deprecated))
#else
# define DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Response __declspec(deprecated)
#endif

namespace gb_interfaces
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct GetTopologicalPath_Response_
{
  using Type = GetTopologicalPath_Response_<ContainerAllocator>;

  explicit GetTopologicalPath_Response_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : path(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->success = false;
      this->message = "";
    }
  }

  explicit GetTopologicalPath_Response_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : path(_alloc, _init),
    message(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->success = false;
      this->message = "";
    }
  }

  // field types and members
  using _path_type =
    nav_msgs::msg::Path_<ContainerAllocator>;
  _path_type path;
  using _success_type =
    bool;
  _success_type success;
  using _message_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _message_type message;

  // setters for named parameter idiom
  Type & set__path(
    const nav_msgs::msg::Path_<ContainerAllocator> & _arg)
  {
    this->path = _arg;
    return *this;
  }
  Type & set__success(
    const bool & _arg)
  {
    this->success = _arg;
    return *this;
  }
  Type & set__message(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->message = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator> *;
  using ConstRawPtr =
    const gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Response
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__gb_interfaces__srv__GetTopologicalPath_Response
    std::shared_ptr<gb_interfaces::srv::GetTopologicalPath_Response_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetTopologicalPath_Response_ & other) const
  {
    if (this->path != other.path) {
      return false;
    }
    if (this->success != other.success) {
      return false;
    }
    if (this->message != other.message) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetTopologicalPath_Response_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetTopologicalPath_Response_

// alias to use template instance with default allocator
using GetTopologicalPath_Response =
  gb_interfaces::srv::GetTopologicalPath_Response_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace gb_interfaces

namespace gb_interfaces
{

namespace srv
{

struct GetTopologicalPath
{
  using Request = gb_interfaces::srv::GetTopologicalPath_Request;
  using Response = gb_interfaces::srv::GetTopologicalPath_Response;
};

}  // namespace srv

}  // namespace gb_interfaces

#endif  // GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__STRUCT_HPP_
