# Copyright 2023 ICube Laboratory, University of Strasbourg
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Maciej Bednarczyk

import os

import yaml
from launch import LaunchDescription
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros import parameter_descriptions
from ament_index_python.packages import get_package_share_directory


def load_yaml(package_name, file_path):
    package_path = get_package_share_directory(package_name)
    absolute_file_path = os.path.join(package_path, file_path)

    try:
        with open(absolute_file_path) as file:
            return yaml.safe_load(file)
    except OSError:  # parent of IOError, OSError *and* WindowsError where available
        return None


def generate_launch_description():
    script_filename = PathJoinSubstitution(
      [FindPackageShare("ur_robot_driver"), "resources", "ros_control.urscript"]
    )

    initial_positions_file = PathJoinSubstitution(
      [FindPackageShare('rtb_bringup'), 'config', 'initial_positions.yaml']
    )

    robot_description_content = Command(
      [
        PathJoinSubstitution([FindExecutable(name='xacro')]),
        ' ',
        PathJoinSubstitution(
            [FindPackageShare('ur_description'), 'urdf', 'ur.urdf.xacro']),
        ' ',
        'safety_limits:=true',
        ' ',
        'safety_pos_margin:=0.15',
        ' ',
        'safety_k_position:=20',
        ' ',
        'name:=ur',
        ' ',
        'ur_type:=ur5',
        ' ',
        'prefix:=""',
        ' ',
        'trans_name:=h1',
        ' ',
        'trans_prefix:=""',
        ' ',
        'trans_steerable_focal:=false',
        ' ',
        "script_filename:=",
        script_filename,
        " ",
        "use_fake_hardware:=true",
        " ",
        "initial_positions_file:=",
        initial_positions_file,
      ]
    )
    robot_description = {
      'robot_description': parameter_descriptions.ParameterValue(robot_description_content, value_type=str)
    }

    robot_controllers = PathJoinSubstitution(
      [FindPackageShare('rtb_bringup'), 'config', 'controllers.yaml']
    )

    rviz_config_file = PathJoinSubstitution(
      [FindPackageShare('rtb_bringup'), 'config', 'rtb.rviz']
    )

    robot_state_publisher_node = Node(
      package='robot_state_publisher',
      executable='robot_state_publisher',
      output='both',
      parameters=[robot_description],
    )

    rviz_node = Node(
      package='rviz2',
      executable='rviz2',
      name='rviz2',
      output='log',
      arguments=['-d', rviz_config_file],
      parameters=[robot_description],
    )

    control_node = Node(
      package='controller_manager',
      executable='ros2_control_node',
      parameters=[robot_description, robot_controllers],
      remappings=[('/rtb_velocity_pytroller/commands', '/cmd_vel')],
      output='both',
    )

    robot_controller_spawner = Node(
      package='controller_manager',
      executable='spawner',
      arguments=['rtb_velocity_pytroller'],
    )

    joint_state_broadcaster_spawner = Node(
      package='controller_manager',
      executable='spawner',
      arguments=['joint_state_broadcaster'],
    )

    nodes_to_start = [
      joint_state_broadcaster_spawner,
      robot_state_publisher_node,
      rviz_node,
      control_node,
      robot_controller_spawner,
    ]

    return LaunchDescription(nodes_to_start)
