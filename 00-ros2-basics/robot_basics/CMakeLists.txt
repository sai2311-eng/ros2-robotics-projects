cmake_minimum_required(VERSION 3.8)
project(robot_basics)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "action/CountTo.action"
)

add_executable(my_talker src/my_talker.cpp)
ament_target_dependencies(my_talker rclcpp std_msgs std_srvs)

add_executable(my_listener src/my_listener.cpp)
ament_target_dependencies(my_listener rclcpp std_msgs)

add_executable(count_to_action_server src/count_to_action_server.cpp)
ament_target_dependencies(count_to_action_server rclcpp rclcpp_action)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(count_to_action_server "${cpp_typesupport_target}")

install(TARGETS
  my_talker
  my_listener
  count_to_action_server
  DESTINATION lib/${PROJECT_NAME}
)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
