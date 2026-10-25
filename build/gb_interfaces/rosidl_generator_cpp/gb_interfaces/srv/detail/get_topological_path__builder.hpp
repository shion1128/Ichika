// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from gb_interfaces:srv/GetTopologicalPath.idl
// generated code does not contain a copyright notice

#ifndef GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__BUILDER_HPP_
#define GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "gb_interfaces/srv/detail/get_topological_path__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace gb_interfaces
{

namespace srv
{

namespace builder
{

class Init_GetTopologicalPath_Request_target_waypoint
{
public:
  Init_GetTopologicalPath_Request_target_waypoint()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::gb_interfaces::srv::GetTopologicalPath_Request target_waypoint(::gb_interfaces::srv::GetTopologicalPath_Request::_target_waypoint_type arg)
  {
    msg_.target_waypoint = std::move(arg);
    return std::move(msg_);
  }

private:
  ::gb_interfaces::srv::GetTopologicalPath_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::gb_interfaces::srv::GetTopologicalPath_Request>()
{
  return gb_interfaces::srv::builder::Init_GetTopologicalPath_Request_target_waypoint();
}

}  // namespace gb_interfaces


namespace gb_interfaces
{

namespace srv
{

namespace builder
{

class Init_GetTopologicalPath_Response_message
{
public:
  explicit Init_GetTopologicalPath_Response_message(::gb_interfaces::srv::GetTopologicalPath_Response & msg)
  : msg_(msg)
  {}
  ::gb_interfaces::srv::GetTopologicalPath_Response message(::gb_interfaces::srv::GetTopologicalPath_Response::_message_type arg)
  {
    msg_.message = std::move(arg);
    return std::move(msg_);
  }

private:
  ::gb_interfaces::srv::GetTopologicalPath_Response msg_;
};

class Init_GetTopologicalPath_Response_success
{
public:
  explicit Init_GetTopologicalPath_Response_success(::gb_interfaces::srv::GetTopologicalPath_Response & msg)
  : msg_(msg)
  {}
  Init_GetTopologicalPath_Response_message success(::gb_interfaces::srv::GetTopologicalPath_Response::_success_type arg)
  {
    msg_.success = std::move(arg);
    return Init_GetTopologicalPath_Response_message(msg_);
  }

private:
  ::gb_interfaces::srv::GetTopologicalPath_Response msg_;
};

class Init_GetTopologicalPath_Response_path
{
public:
  Init_GetTopologicalPath_Response_path()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetTopologicalPath_Response_success path(::gb_interfaces::srv::GetTopologicalPath_Response::_path_type arg)
  {
    msg_.path = std::move(arg);
    return Init_GetTopologicalPath_Response_success(msg_);
  }

private:
  ::gb_interfaces::srv::GetTopologicalPath_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::gb_interfaces::srv::GetTopologicalPath_Response>()
{
  return gb_interfaces::srv::builder::Init_GetTopologicalPath_Response_path();
}

}  // namespace gb_interfaces

#endif  // GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__BUILDER_HPP_
