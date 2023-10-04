// Copyright 2023 ICube-Robotics
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// author: Maciej Bednarczyk

#include "rtb_velocity_pytroller/rtb_velocity_pytroller.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <limits>

#include "controller_interface/helpers.hpp"
#include "hardware_interface/loaned_command_interface.hpp"
#include "rclcpp/logging.hpp"
#include "rclcpp/qos.hpp"

#include <Python.h>
#include "rtb_velocity_pytroller_logic.h"

namespace rtb_velocity_pytroller
{
RtbVelocityPytroller::RtbVelocityPytroller()
: controller_interface::ControllerInterface(),
  rt_command_ptr_(nullptr),
  command_subscriber_(nullptr)
{
}

controller_interface::CallbackReturn RtbVelocityPytroller::on_init()
{
  try
  {
    declare_parameters();
  }
  catch (const std::exception & e)
  {
    fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }

  if (PyImport_AppendInittab("rtb_velocity_pytroller_logic", PyInit_rtb_velocity_pytroller_logic) == -1) {
    fprintf(stderr, "Error: could not extend in-built modules table\n");
    return controller_interface::CallbackReturn::ERROR;
  }

  Py_Initialize();
  PyImport_ImportModule("rtb_velocity_pytroller_logic");

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn RtbVelocityPytroller::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  auto ret = this->read_parameters();
  if (ret != controller_interface::CallbackReturn::SUCCESS)
  {
    return ret;
  }

  for (auto index = 0ul; index < command_interfaces_.size(); ++index)
  {
    commands_.insert({command_interfaces_[index].get_name(), command_interfaces_[index].get_value()});
  }

  for (auto index = 0ul; index < state_interfaces_.size(); ++index)
  {
    states_.insert({state_interfaces_[index].get_name(), state_interfaces_[index].get_value()});
  }

  command_subscriber_ = get_node()->create_generic_subscription(
    params_.command_topic_name, params_.command_topic_type, rclcpp::SystemDefaultsQoS(),
    [this](std::shared_ptr<rclcpp::SerializedMessage> msg) { rt_command_ptr_.writeFromNonRT(msg); });

  RCLCPP_INFO(get_node()->get_logger(), "configure successful");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::InterfaceConfiguration
RtbVelocityPytroller::command_interface_configuration() const
{
  controller_interface::InterfaceConfiguration command_interfaces_config;
  command_interfaces_config.type = controller_interface::interface_configuration_type::INDIVIDUAL;
  command_interfaces_config.names = command_interface_types_;

  return command_interfaces_config;
}

controller_interface::InterfaceConfiguration RtbVelocityPytroller::state_interface_configuration()
  const
{
  return controller_interface::InterfaceConfiguration{
    controller_interface::interface_configuration_type::ALL};
}

controller_interface::CallbackReturn RtbVelocityPytroller::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  std::vector<std::reference_wrapper<hardware_interface::LoanedCommandInterface>>
    ordered_interfaces;
  if (
    !controller_interface::get_ordered_interfaces(
      command_interfaces_, command_interface_types_, std::string(""), ordered_interfaces) ||
    command_interface_types_.size() != ordered_interfaces.size())
  {
    RCLCPP_ERROR(
      get_node()->get_logger(), "Expected %zu command interfaces, got %zu",
      command_interface_types_.size(), ordered_interfaces.size());
    return controller_interface::CallbackReturn::ERROR;
  }

  // reset command buffer if a command came through callback when controller was inactive
  rt_command_ptr_ = realtime_tools::RealtimeBuffer<std::shared_ptr<rclcpp::SerializedMessage>>(nullptr);

  RCLCPP_INFO(get_node()->get_logger(), "activate successful");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn RtbVelocityPytroller::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // reset command buffer
  rt_command_ptr_ = realtime_tools::RealtimeBuffer<std::shared_ptr<rclcpp::SerializedMessage>>(nullptr);
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::return_type RtbVelocityPytroller::update(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  auto msg = rt_command_ptr_.readFromRT();

  // no command received yet
  if (!msg || !(*msg)) {
    return controller_interface::return_type::OK;
  }

  // fill commands and states maps to be passed to python
  for (auto index = 0ul; index < command_interfaces_.size(); ++index) {
    commands_[command_interfaces_[index].get_name()] = std::numeric_limits<double>::quiet_NaN();
  }
  for (auto index = 0ul; index < state_interfaces_.size(); ++index) {
    states_[state_interfaces_[index].get_name()] = state_interfaces_[index].get_value();
  }

  // update parameters if changed
  if (param_listener_->is_old(params_)) {
    params_ = param_listener_->get_params();
  }

  // fill message buffer to be passed to python
  std::vector<int> message((*msg)->get_rcl_serialized_message().buffer,
    (*msg)->get_rcl_serialized_message().buffer +  (*msg)->size());

  // run cython function
  if (rtb_velocity_pytroller_logic(states_, commands_, message, params_)) {
    RCLCPP_ERROR_THROTTLE(
      get_node()->get_logger(), *(get_node()->get_clock()), 1000,
      "rtb_velocity_pytroller logic failed.");
    return controller_interface::return_type::ERROR;
  }

  // retrieve commands from python and fill command interfaces
  for (auto index = 0ul; index < command_interfaces_.size(); ++index) {
    command_interfaces_[index].set_value(commands_[command_interfaces_[index].get_name()]);
  }

  return controller_interface::return_type::OK;
}

void RtbVelocityPytroller::declare_parameters()
{
  param_listener_ = std::make_shared<ParamListener>(get_node());
}

controller_interface::CallbackReturn RtbVelocityPytroller::read_parameters()
{
  if (!param_listener_)
  {
    RCLCPP_ERROR(get_node()->get_logger(), "Error encountered during init");
    return controller_interface::CallbackReturn::ERROR;
  }
  params_ = param_listener_->get_params();

  if (params_.joints.empty())
  {
    RCLCPP_ERROR(get_node()->get_logger(), "'joints' parameter was empty");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (params_.interface_name.empty())
  {
    RCLCPP_ERROR(get_node()->get_logger(), "'interface_name' parameter was empty");
    return controller_interface::CallbackReturn::ERROR;
  }

  for (const auto & joint : params_.joints)
  {
    command_interface_types_.push_back(joint + "/" + params_.interface_name);
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

}  // namespace rtb_velocity_pytroller

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  rtb_velocity_pytroller::RtbVelocityPytroller, controller_interface::ControllerInterface)
