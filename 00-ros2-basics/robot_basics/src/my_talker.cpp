#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

class MyTalker : public rclcpp::Node
{
public:
  MyTalker()
  : Node("my_talker"), count_(0)
  {
    publisher_ = this->create_publisher<std_msgs::msg::String>(
      "robot_message",
      rclcpp::QoS(10).best_effort());

    timer_ = this->create_wall_timer(
      500ms,
      [this]() {
        publish_message();
      });

    reset_service_ = this->create_service<std_srvs::srv::Trigger>(
      "reset_counter",
      [this](
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
        (void)request;

        count_ = 0;
        response->success = true;
        response->message = "Counter reset to zero";

        RCLCPP_INFO(this->get_logger(), "Counter reset by service request");
      });
  }

private:
  void publish_message()
  {
    auto message = std_msgs::msg::String();
    message.data = "Hello from my own ROS 2 node: " + std::to_string(count_++);

    RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
    publisher_->publish(message);
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_service_;
  std::size_t count_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MyTalker>());
  rclcpp::shutdown();
  return 0;
}
