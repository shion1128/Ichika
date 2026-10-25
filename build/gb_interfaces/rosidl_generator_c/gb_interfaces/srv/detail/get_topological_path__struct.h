// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from gb_interfaces:srv/GetTopologicalPath.idl
// generated code does not contain a copyright notice

#ifndef GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__STRUCT_H_
#define GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'target_waypoint'
#include "rosidl_runtime_c/string.h"

/// Struct defined in srv/GetTopologicalPath in the package gb_interfaces.
typedef struct gb_interfaces__srv__GetTopologicalPath_Request
{
  rosidl_runtime_c__String target_waypoint;
} gb_interfaces__srv__GetTopologicalPath_Request;

// Struct for a sequence of gb_interfaces__srv__GetTopologicalPath_Request.
typedef struct gb_interfaces__srv__GetTopologicalPath_Request__Sequence
{
  gb_interfaces__srv__GetTopologicalPath_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} gb_interfaces__srv__GetTopologicalPath_Request__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'path'
#include "nav_msgs/msg/detail/path__struct.h"
// Member 'message'
// already included above
// #include "rosidl_runtime_c/string.h"

/// Struct defined in srv/GetTopologicalPath in the package gb_interfaces.
typedef struct gb_interfaces__srv__GetTopologicalPath_Response
{
  nav_msgs__msg__Path path;
  bool success;
  rosidl_runtime_c__String message;
} gb_interfaces__srv__GetTopologicalPath_Response;

// Struct for a sequence of gb_interfaces__srv__GetTopologicalPath_Response.
typedef struct gb_interfaces__srv__GetTopologicalPath_Response__Sequence
{
  gb_interfaces__srv__GetTopologicalPath_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} gb_interfaces__srv__GetTopologicalPath_Response__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // GB_INTERFACES__SRV__DETAIL__GET_TOPOLOGICAL_PATH__STRUCT_H_
