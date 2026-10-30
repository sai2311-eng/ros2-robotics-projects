#include <chrono>
#include <memory>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class MyListener : public rclcpp::Node
{
public:
  MyListener()
  : Node("my_listener")
  {
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      "robot_message",
      rclcpp::QoS(10).best_effort(),
      [this](const std_msgs::msg::String::SharedPtr message) {
        RCLCPP_INFO(
          this->get_logger(),
          "Received: '%s'",
          message->data.c_str());

      });
  }

private:
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MyListener>());
  rclcpp::shutdown();
  return 0;
}
