# Copyright 2023 ICube-Robotics
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
# author: Maciej Bednarczyk

# import os
# import math
# import yaml
import numpy as np

import roboticstoolbox as rtb


def pytroller_logic_impl(period, states, commands, msg, params):
    
    robot = rtb.models.UR5()

    robot.q = np.array([
      states[b'shoulder_pan_joint/position'],
      states[b'shoulder_lift_joint/position'],
      states[b'elbow_joint/position'],
      states[b'wrist_1_joint/position'],
      states[b'wrist_2_joint/position'],
      states[b'wrist_3_joint/position'],
    ])

    v = np.array([
      msg.linear.x,
      msg.linear.y,
      msg.linear.z,
      msg.angular.x,
      msg.angular.y,
      msg.angular.z,
    ])

    robot.qd = np.linalg.pinv(robot.jacobe(robot.q)) @ v

    commands[b'shoulder_pan_joint/position'] = robot.q[0] + robot.qd[0] * period
    commands[b'shoulder_lift_joint/position'] = robot.q[1] + robot.qd[1] * period
    commands[b'elbow_joint/position'] = robot.q[2] + robot.qd[2] * period
    commands[b'wrist_1_joint/position'] = robot.q[3] + robot.qd[3] * period
    commands[b'wrist_2_joint/position'] = robot.q[4] + robot.qd[4] * period
    commands[b'wrist_3_joint/position'] = robot.q[5] + robot.qd[5] * period

    return commands
