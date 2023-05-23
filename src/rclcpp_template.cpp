// STL and OS Deps
#include <chrono>
#include <functional>
#include <memory>
#include <string>

// ROS Deps
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp" 
#include "std_srvs/srv/trigger.hpp" 
#include "rcl_interfaces/msg/set_parameters_result.hpp"

using namespace std::chrono_literals;
using namespace std::placeholders;

class RclcppTemplate : public rclcpp::Node
{

private:
    // ROS
    std::shared_ptr<rclcpp::ParameterEventHandler> param_event_handler;
    std::shared_ptr<rclcpp::ParameterCallbackHandle> param_update_handler_message;
    OnSetParametersCallbackHandle::SharedPtr param_on_set_handle;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_echo;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_message;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_shout;
    
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr service_reset_counter;    
    rclcpp::TimerBase::SharedPtr timer_main_loop;

    // User
    size_t count;

public:
    RclcppTemplate()
    : Node("rclcpp_template")
    {
        // ROS Parameter
        params_initialization();

        // ROS Interface
        pub_echo = this->create_publisher<std_msgs::msg::String>("echo", 10);
        pub_message = this->create_publisher<std_msgs::msg::String>("message", 10);
        pub_echo = this->create_publisher<std_msgs::msg::String>("echo", 10);
        sub_shout = this->create_subscription<std_msgs::msg::String>("shout", 10, std::bind(&RclcppTemplate::sub_callback_shout, this, _1));

        service_reset_counter = this->create_service<std_srvs::srv::Trigger>("reset_counter", std::bind(&RclcppTemplate::service_callback_reset_counter, this, _1, _2));

        timer_main_loop = this->create_wall_timer( param_main_loop_rate()*1s, std::bind(&RclcppTemplate::timer_callback_main_loop, this));

        // User Initialization
        count = 0;
    }

private:

    // ROS Parameter
    // -------------

    void params_initialization()
    {
        param_event_handler = std::make_shared<rclcpp::ParameterEventHandler>(this);

        this->declare_parameter("main_loop_rate", 1.0);
        this->declare_parameter("message", "Hello World");

        param_update_handler_message = param_event_handler->add_parameter_callback(
                "message", std::bind(&RclcppTemplate::param_update_callback_message, this, _1));

        param_on_set_handle = this->add_on_set_parameters_callback(
                std::bind(&RclcppTemplate::param_set_callback, this, _1));
    }

    double param_main_loop_rate(){ return this->get_parameter("main_loop_rate").as_double(); }
    std::string param_message(){ return this->get_parameter("message").as_string(); }

    // Prevent certain parameters from being updated
    rcl_interfaces::msg::SetParametersResult param_set_callback(const std::vector<rclcpp::Parameter> & parameters)
    {
        // RCLCPP_INFO_STREAM(this->get_logger(), "Parameters updated: " << parameters.size());
        rcl_interfaces::msg::SetParametersResult result;
        result.successful = true;
        for (const auto & p : parameters) {
            // RCLCPP_INFO(this->get_logger(), "%s", p.get_name().c_str());
            if ( p.get_name() == "main_loop_rate" ) {
                result.successful = false;
                result.reason = "Loop rate cannot be changed dynamically";
            }
        }
        if (result.successful == false) {
            RCLCPP_WARN(this->get_logger(), result.reason.c_str());
        }
        return result;
    }

    void param_update_callback_message(const rclcpp::Parameter &p)
    {
        count = 0;
    }

    // ROS Loop Functions
    // ------------------
 
    void timer_callback_main_loop()
    {
        auto message = std_msgs::msg::String();
        message.data = param_message() + " " + std::to_string(count++);
        RCLCPP_INFO_STREAM(this->get_logger(), "Publishing: " << message.data.c_str());
        pub_message->publish(message);
    }

    // ROS Interface Functions
    // -----------------------

    void sub_callback_shout(const std_msgs::msg::String & msg)
    {
        RCLCPP_INFO_STREAM(this->get_logger(), "Echoing message: " << msg.data.c_str());
        pub_echo->publish(msg);
    }

    void service_callback_reset_counter(
            const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
            std::shared_ptr<std_srvs::srv::Trigger::Response> response
    ) {
        RCLCPP_INFO_STREAM(this->get_logger(), "Reset Counter");
        count = 0;
        response->success = true;
    }

    // User Functions
    // --------------

};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RclcppTemplate>());
    rclcpp::shutdown();
    return 0;
}