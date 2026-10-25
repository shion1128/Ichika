// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from gb_interfaces:srv/GetTopologicalPath.idl
// generated code does not contain a copyright notice
#include "gb_interfaces/srv/detail/get_topological_path__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"

// Include directives for member types
// Member `target_waypoint`
#include "rosidl_runtime_c/string_functions.h"

bool
gb_interfaces__srv__GetTopologicalPath_Request__init(gb_interfaces__srv__GetTopologicalPath_Request * msg)
{
  if (!msg) {
    return false;
  }
  // target_waypoint
  if (!rosidl_runtime_c__String__init(&msg->target_waypoint)) {
    gb_interfaces__srv__GetTopologicalPath_Request__fini(msg);
    return false;
  }
  return true;
}

void
gb_interfaces__srv__GetTopologicalPath_Request__fini(gb_interfaces__srv__GetTopologicalPath_Request * msg)
{
  if (!msg) {
    return;
  }
  // target_waypoint
  rosidl_runtime_c__String__fini(&msg->target_waypoint);
}

bool
gb_interfaces__srv__GetTopologicalPath_Request__are_equal(const gb_interfaces__srv__GetTopologicalPath_Request * lhs, const gb_interfaces__srv__GetTopologicalPath_Request * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // target_waypoint
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->target_waypoint), &(rhs->target_waypoint)))
  {
    return false;
  }
  return true;
}

bool
gb_interfaces__srv__GetTopologicalPath_Request__copy(
  const gb_interfaces__srv__GetTopologicalPath_Request * input,
  gb_interfaces__srv__GetTopologicalPath_Request * output)
{
  if (!input || !output) {
    return false;
  }
  // target_waypoint
  if (!rosidl_runtime_c__String__copy(
      &(input->target_waypoint), &(output->target_waypoint)))
  {
    return false;
  }
  return true;
}

gb_interfaces__srv__GetTopologicalPath_Request *
gb_interfaces__srv__GetTopologicalPath_Request__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  gb_interfaces__srv__GetTopologicalPath_Request * msg = (gb_interfaces__srv__GetTopologicalPath_Request *)allocator.allocate(sizeof(gb_interfaces__srv__GetTopologicalPath_Request), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(gb_interfaces__srv__GetTopologicalPath_Request));
  bool success = gb_interfaces__srv__GetTopologicalPath_Request__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
gb_interfaces__srv__GetTopologicalPath_Request__destroy(gb_interfaces__srv__GetTopologicalPath_Request * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    gb_interfaces__srv__GetTopologicalPath_Request__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
gb_interfaces__srv__GetTopologicalPath_Request__Sequence__init(gb_interfaces__srv__GetTopologicalPath_Request__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  gb_interfaces__srv__GetTopologicalPath_Request * data = NULL;

  if (size) {
    data = (gb_interfaces__srv__GetTopologicalPath_Request *)allocator.zero_allocate(size, sizeof(gb_interfaces__srv__GetTopologicalPath_Request), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = gb_interfaces__srv__GetTopologicalPath_Request__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        gb_interfaces__srv__GetTopologicalPath_Request__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
gb_interfaces__srv__GetTopologicalPath_Request__Sequence__fini(gb_interfaces__srv__GetTopologicalPath_Request__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      gb_interfaces__srv__GetTopologicalPath_Request__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

gb_interfaces__srv__GetTopologicalPath_Request__Sequence *
gb_interfaces__srv__GetTopologicalPath_Request__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  gb_interfaces__srv__GetTopologicalPath_Request__Sequence * array = (gb_interfaces__srv__GetTopologicalPath_Request__Sequence *)allocator.allocate(sizeof(gb_interfaces__srv__GetTopologicalPath_Request__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = gb_interfaces__srv__GetTopologicalPath_Request__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
gb_interfaces__srv__GetTopologicalPath_Request__Sequence__destroy(gb_interfaces__srv__GetTopologicalPath_Request__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    gb_interfaces__srv__GetTopologicalPath_Request__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
gb_interfaces__srv__GetTopologicalPath_Request__Sequence__are_equal(const gb_interfaces__srv__GetTopologicalPath_Request__Sequence * lhs, const gb_interfaces__srv__GetTopologicalPath_Request__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!gb_interfaces__srv__GetTopologicalPath_Request__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
gb_interfaces__srv__GetTopologicalPath_Request__Sequence__copy(
  const gb_interfaces__srv__GetTopologicalPath_Request__Sequence * input,
  gb_interfaces__srv__GetTopologicalPath_Request__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(gb_interfaces__srv__GetTopologicalPath_Request);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    gb_interfaces__srv__GetTopologicalPath_Request * data =
      (gb_interfaces__srv__GetTopologicalPath_Request *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!gb_interfaces__srv__GetTopologicalPath_Request__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          gb_interfaces__srv__GetTopologicalPath_Request__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!gb_interfaces__srv__GetTopologicalPath_Request__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}


// Include directives for member types
// Member `path`
#include "nav_msgs/msg/detail/path__functions.h"
// Member `message`
// already included above
// #include "rosidl_runtime_c/string_functions.h"

bool
gb_interfaces__srv__GetTopologicalPath_Response__init(gb_interfaces__srv__GetTopologicalPath_Response * msg)
{
  if (!msg) {
    return false;
  }
  // path
  if (!nav_msgs__msg__Path__init(&msg->path)) {
    gb_interfaces__srv__GetTopologicalPath_Response__fini(msg);
    return false;
  }
  // success
  // message
  if (!rosidl_runtime_c__String__init(&msg->message)) {
    gb_interfaces__srv__GetTopologicalPath_Response__fini(msg);
    return false;
  }
  return true;
}

void
gb_interfaces__srv__GetTopologicalPath_Response__fini(gb_interfaces__srv__GetTopologicalPath_Response * msg)
{
  if (!msg) {
    return;
  }
  // path
  nav_msgs__msg__Path__fini(&msg->path);
  // success
  // message
  rosidl_runtime_c__String__fini(&msg->message);
}

bool
gb_interfaces__srv__GetTopologicalPath_Response__are_equal(const gb_interfaces__srv__GetTopologicalPath_Response * lhs, const gb_interfaces__srv__GetTopologicalPath_Response * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // path
  if (!nav_msgs__msg__Path__are_equal(
      &(lhs->path), &(rhs->path)))
  {
    return false;
  }
  // success
  if (lhs->success != rhs->success) {
    return false;
  }
  // message
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->message), &(rhs->message)))
  {
    return false;
  }
  return true;
}

bool
gb_interfaces__srv__GetTopologicalPath_Response__copy(
  const gb_interfaces__srv__GetTopologicalPath_Response * input,
  gb_interfaces__srv__GetTopologicalPath_Response * output)
{
  if (!input || !output) {
    return false;
  }
  // path
  if (!nav_msgs__msg__Path__copy(
      &(input->path), &(output->path)))
  {
    return false;
  }
  // success
  output->success = input->success;
  // message
  if (!rosidl_runtime_c__String__copy(
      &(input->message), &(output->message)))
  {
    return false;
  }
  return true;
}

gb_interfaces__srv__GetTopologicalPath_Response *
gb_interfaces__srv__GetTopologicalPath_Response__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  gb_interfaces__srv__GetTopologicalPath_Response * msg = (gb_interfaces__srv__GetTopologicalPath_Response *)allocator.allocate(sizeof(gb_interfaces__srv__GetTopologicalPath_Response), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(gb_interfaces__srv__GetTopologicalPath_Response));
  bool success = gb_interfaces__srv__GetTopologicalPath_Response__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
gb_interfaces__srv__GetTopologicalPath_Response__destroy(gb_interfaces__srv__GetTopologicalPath_Response * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    gb_interfaces__srv__GetTopologicalPath_Response__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
gb_interfaces__srv__GetTopologicalPath_Response__Sequence__init(gb_interfaces__srv__GetTopologicalPath_Response__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  gb_interfaces__srv__GetTopologicalPath_Response * data = NULL;

  if (size) {
    data = (gb_interfaces__srv__GetTopologicalPath_Response *)allocator.zero_allocate(size, sizeof(gb_interfaces__srv__GetTopologicalPath_Response), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = gb_interfaces__srv__GetTopologicalPath_Response__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        gb_interfaces__srv__GetTopologicalPath_Response__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
gb_interfaces__srv__GetTopologicalPath_Response__Sequence__fini(gb_interfaces__srv__GetTopologicalPath_Response__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      gb_interfaces__srv__GetTopologicalPath_Response__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

gb_interfaces__srv__GetTopologicalPath_Response__Sequence *
gb_interfaces__srv__GetTopologicalPath_Response__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  gb_interfaces__srv__GetTopologicalPath_Response__Sequence * array = (gb_interfaces__srv__GetTopologicalPath_Response__Sequence *)allocator.allocate(sizeof(gb_interfaces__srv__GetTopologicalPath_Response__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = gb_interfaces__srv__GetTopologicalPath_Response__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
gb_interfaces__srv__GetTopologicalPath_Response__Sequence__destroy(gb_interfaces__srv__GetTopologicalPath_Response__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    gb_interfaces__srv__GetTopologicalPath_Response__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
gb_interfaces__srv__GetTopologicalPath_Response__Sequence__are_equal(const gb_interfaces__srv__GetTopologicalPath_Response__Sequence * lhs, const gb_interfaces__srv__GetTopologicalPath_Response__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!gb_interfaces__srv__GetTopologicalPath_Response__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
gb_interfaces__srv__GetTopologicalPath_Response__Sequence__copy(
  const gb_interfaces__srv__GetTopologicalPath_Response__Sequence * input,
  gb_interfaces__srv__GetTopologicalPath_Response__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(gb_interfaces__srv__GetTopologicalPath_Response);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    gb_interfaces__srv__GetTopologicalPath_Response * data =
      (gb_interfaces__srv__GetTopologicalPath_Response *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!gb_interfaces__srv__GetTopologicalPath_Response__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          gb_interfaces__srv__GetTopologicalPath_Response__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!gb_interfaces__srv__GetTopologicalPath_Response__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
