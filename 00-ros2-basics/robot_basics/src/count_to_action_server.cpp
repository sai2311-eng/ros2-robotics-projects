#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "robot_basics/action/count_to.hpp"

using namespace std::chrono_literals;

class CountToActionServer : public rclcpp::Node
{
public:
  using CountTo = robot_basics::action::CountTo;
  using GoalHandleCountTo = rclcpp_action::ServerGoalHandle<CountTo>;

  CountToActionServer()
  : Node("count_to_action_server")
  {
    action_server_ = rclcpp_action::create_server<CountTo>(
      this,
      "count_to",
      std::bind(
        &CountToActionServer::handle_goal,
        this,
        std::placeholders::_1,
        std::placeholders::_2),
      std::bind(
        &CountToActionServer::handle_cancel,
        this,
        std::placeholders::_1),
      std::bind(
        &CountToActionServer::handle_accepted,
        this,
        std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Action server is ready.");
  }

private:
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID &,
    std::shared_ptr<const CountTo::Goal> goal)
  {
    RCLCPP_INFO(
      this->get_logger(),
      "Received goal: count to %d",
      goal->target_count);

    if (goal->target_count < 0) {
      RCLCPP_WARN(this->get_logger(), "Negative goals are not allowed.");
      return rclcpp_action::GoalResponse::REJECT;
    }

    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleCountTo>)
  {
    RCLCPP_INFO(this->get_logger(), "Cancel request received.");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandleCountTo> goal_handle)
  {
    std::thread{
      std::bind(&CountToActionServer::execute, this, std::placeholders::_1),
      goal_handle}.detach();
  }

  void execute(const std::shared_ptr<GoalHandleCountTo> goal_handle)
  {
    const int target = goal_handle->get_goal()->target_count;

    auto feedback = std::make_shared<CountTo::Feedback>();
    auto result = std::make_shared<CountTo::Result>();

    for (int current = 0; current <= target; ++current) {
      if (goal_handle->is_canceling()) {
        result->final_count = current;
        goal_handle->canceled(result);

        RCLCPP_INFO(
          this->get_logger(),
          "Goal cancelled at %d",
          current);
        return;
      }

      feedback->current_count = current;
      goal_handle->publish_feedback(feedback);

      RCLCPP_INFO(
        this->get_logger(),
        "Feedback: %d",
        current);

      std::this_thread::sleep_for(1s);
    }

    result->final_count = target;
    goal_handle->succeed(result);

    RCLCPP_INFO(
      this->get_logger(),
      "Goal completed. Final count: %d",
      target);
  }

  rclcpp_action::Server<CountTo>::SharedPtr action_server_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CountToActionServer>());
  rclcpp::shutdown();
  return 0;
}
