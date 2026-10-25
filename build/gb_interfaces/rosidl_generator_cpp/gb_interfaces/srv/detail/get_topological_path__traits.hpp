// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from gb_interfaces:srv/GetTopologicalPath.idl
// generated code does not contain a copyright notice

#ifndef GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__TRAITS_HPP_
#define GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "gb_interfaces/srv/detail/get_topological_path__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace gb_interfaces
{

namespace srv
{

inline void to_flow_style_yaml(
  const GetTopologicalPath_Request & msg,
  std::ostream & out)
{
  out << "{";
  // member: target_waypoint
  {
    out << "target_waypoint: ";
    rosidl_generator_traits::value_to_yaml(msg.target_waypoint, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const GetTopologicalPath_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: target_waypoint
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "target_waypoint: ";
    rosidl_generator_traits::value_to_yaml(msg.target_waypoint, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const GetTopologicalPath_Request & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace gb_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use gb_interfaces::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const gb_interfaces::srv::GetTopologicalPath_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  gb_interfaces::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use gb_interfaces::srv::to_yaml() instead")]]
inline std::string to_yaml(const gb_interfaces::srv::GetTopologicalPath_Request & msg)
{
  return gb_interfaces::srv::to_yaml(msg);
}

template<>
inline const char * data_type<gb_interfaces::srv::GetTopologicalPath_Request>()
{
  return "gb_interfaces::srv::GetTopologicalPath_Request";
}

template<>
inline const char * name<gb_interfaces::srv::GetTopologicalPath_Request>()
{
  return "gb_interfaces/srv/GetTopologicalPath_Request";
}

template<>
struct has_fixed_size<gb_interfaces::srv::GetTopologicalPath_Request>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<gb_interfaces::srv::GetTopologicalPath_Request>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<gb_interfaces::srv::GetTopologicalPath_Request>
  : std::true_type {};

}  // namespace rosidl_generator_traits

// Include directives for member types
// Member 'path'
#include "nav_msgs/msg/detail/path__traits.hpp"

namespace gb_interfaces
{

namespace srv
{

inline void to_flow_style_yaml(
  const GetTopologicalPath_Response & msg,
  std::ostream & out)
{
  out << "{";
  // member: path
  {
    out << "path: ";
    to_flow_style_yaml(msg.path, out);
    out << ", ";
  }

  // member: success
  {
    out << "success: ";
    rosidl_generator_traits::value_to_yaml(msg.success, out);
    out << ", ";
  }

  // member: message
  {
    out << "message: ";
    rosidl_generator_traits::value_to_yaml(msg.message, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const GetTopologicalPath_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: path
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "path:\n";
    to_block_style_yaml(msg.path, out, indentation + 2);
  }

  // member: success
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "success: ";
    rosidl_generator_traits::value_to_yaml(msg.success, out);
    out << "\n";
  }

  // member: message
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "message: ";
    rosidl_generator_traits::value_to_yaml(msg.message, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const GetTopologicalPath_Response & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace gb_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use gb_interfaces::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const gb_interfaces::srv::GetTopologicalPath_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  gb_interfaces::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use gb_interfaces::srv::to_yaml() instead")]]
inline std::string to_yaml(const gb_interfaces::srv::GetTopologicalPath_Response & msg)
{
  return gb_interfaces::srv::to_yaml(msg);
}

template<>
inline const char * data_type<gb_interfaces::srv::GetTopologicalPath_Response>()
{
  return "gb_interfaces::srv::GetTopologicalPath_Response";
}

template<>
inline const char * name<gb_interfaces::srv::GetTopologicalPath_Response>()
{
  return "gb_interfaces/srv/GetTopologicalPath_Response";
}

template<>
struct has_fixed_size<gb_interfaces::srv::GetTopologicalPath_Response>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<gb_interfaces::srv::GetTopologicalPath_Response>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<gb_interfaces::srv::GetTopologicalPath_Response>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rosidl_generator_traits
{

template<>
inline const char * data_type<gb_interfaces::srv::GetTopologicalPath>()
{
  return "gb_interfaces::srv::GetTopologicalPath";
}

template<>
inline const char * name<gb_interfaces::srv::GetTopologicalPath>()
{
  return "gb_interfaces/srv/GetTopologicalPath";
}

template<>
struct has_fixed_size<gb_interfaces::srv::GetTopologicalPath>
  : std::integral_constant<
    bool,
    has_fixed_size<gb_interfaces::srv::GetTopologicalPath_Request>::value &&
    has_fixed_size<gb_interfaces::srv::GetTopologicalPath_Response>::value
  >
{
};

template<>
struct has_bounded_size<gb_interfaces::srv::GetTopologicalPath>
  : std::integral_constant<
    bool,
    has_bounded_size<gb_interfaces::srv::GetTopologicalPath_Request>::value &&
    has_bounded_size<gb_interfaces::srv::GetTopologicalPath_Response>::value
  >
{
};

template<>
struct is_service<gb_interfaces::srv::GetTopologicalPath>
  : std::true_type
{
};

template<>
struct is_service_request<gb_interfaces::srv::GetTopologicalPath_Request>
  : std::true_type
{
};

template<>
struct is_service_response<gb_interfaces::srv::GetTopologicalPath_Response>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits

#endif  // GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__TRAITS_HPP_
