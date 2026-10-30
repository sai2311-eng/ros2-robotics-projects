# 00: ROS 2 basics in C++ (`robot_basics`)

The first exercises of the series: the four core ROS 2 communication patterns written by hand in C++ with `rclcpp`, before moving on to the robot in Project 1.

| Pattern | Node / executable | Name | Type |
|---------|-------------------|------|------|
| Publisher | `my_talker` | topic `robot_message` | `std_msgs/msg/String` |
| Subscriber | `my_listener` | topic `robot_message` | `std_msgs/msg/String` |
| Service | `my_talker` | `reset_counter` | `std_srvs/srv/Trigger` |
| Action | `count_to_action_server` | `count_to` | `robot_basics/action/CountTo` (custom) |

## What each node does

**`my_talker`** publishes `"Hello from my own ROS 2 node: <n>"` every 500 ms on `robot_message`, with QoS depth 10 and **best effort** reliability. It also hosts the service `reset_counter`, which sets `<n>` back to 0 and replies `success=true, message="Counter reset to zero"`.

**`my_listener`** subscribes to `robot_message` and logs each message. It uses the same QoS (depth 10, best effort) as the talker. Publisher and subscriber QoS must be compatible: a *reliable* subscriber cannot receive from a *best-effort* publisher.

**`count_to_action_server`** implements the custom action `CountTo`:

```
int32 target_count      # goal
---
int32 final_count       # result
---
int32 current_count     # feedback
```

- A goal with a negative `target_count` is **rejected**.
- Otherwise it counts from 0 to `target_count`, one step per second, publishing `current_count` as feedback each step.
- It accepts cancel requests; on cancel it returns the count reached so far as `final_count`.
- The work runs in a detached thread so the node can keep spinning and handle cancel requests.

## Files

```
robot_basics/
├── package.xml
├── CMakeLists.txt                 builds 3 executables and generates the action interface
├── action/CountTo.action          custom action definition
└── src/
    ├── my_talker.cpp
    ├── my_listener.cpp
    └── count_to_action_server.cpp
```

In `CMakeLists.txt`, `rosidl_generate_interfaces` turns `CountTo.action` into C++ headers (`robot_basics/action/count_to.hpp`). The action server then links against that package's own typesupport with `rosidl_get_typesupport_target`.

## Build and run

Requires ROS 2 Jazzy (`ros-jazzy-desktop`).

```bash
cd ~/ros2_ws/src
cp -r <this-repo>/00-ros2-basics/robot_basics .
cd ~/ros2_ws
colcon build --packages-select robot_basics
source install/setup.bash
```

Publisher and subscriber (two terminals):

```bash
ros2 run robot_basics my_talker
ros2 run robot_basics my_listener
```

Service (while the talker runs):

```bash
ros2 service call /reset_counter std_srvs/srv/Trigger
```

The counter in the talker's messages restarts at 0.

Action:

```bash
ros2 run robot_basics count_to_action_server

# in another terminal
ros2 action send_goal /count_to robot_basics/action/CountTo "{target_count: 5}" --feedback
ros2 action send_goal /count_to robot_basics/action/CountTo "{target_count: -1}"   # rejected
```

## Verified

Built in a clean workspace with `colcon build`, then run: the listener received the talker's messages; the service returned `success=True, message='Counter reset to zero'`; a goal of 3 returned feedback 0, 1, 2, 3 and `SUCCEEDED` with `final_count: 3`; a goal of -1 printed `Goal was rejected.`

## Concepts practiced

- Nodes as classes derived from `rclcpp::Node`, spinning with `rclcpp::spin`
- Wall timers, publishers, subscriptions, services
- QoS profiles (`rclcpp::QoS(10).best_effort()`)
- Custom interface generation and the action goal / feedback / result / cancel lifecycle
