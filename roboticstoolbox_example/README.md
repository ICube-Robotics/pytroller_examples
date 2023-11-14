# Robotics Toolbox example 

Example Python controller using the [`roboticstoolbox`](https://github.com/petercorke/robotics-toolbox-python) for Python. 

In this example, the `roboticstoolbox` is used to compute joint velocities from a Twist message to control a UR5 robot.

To run the demo use the provided launch file:

```shell
$ ros2 launch rtb_bringup rtb.launch.py
```

To start a GUI and send Twist messages to the `\cmd_vel` topic, the [slider_publisher](https://github.com/oKermorgant/slider_publisher/tree/ros2) package can be use:

```shell
$ sudo apt install ros-humble-slider-publisher
$ ros2 launch slider_publisher example.launch file:=Twist.yaml
```

**Note** : This Pytroller was generated using the CLI tools provided in the [Pytroller project](https://github.com/ICube-Robotics/pytroller) and only the logic [script](/roboticstoolbox_example/rtb_velocity_pytroller/script/rtb_velocity_pytroller_logic_impl.py) file was implemented.

**Note** : To use the [`roboticstoolbox`](https://github.com/petercorke/robotics-toolbox-python) with ROS 2 Humble, the version `1.0.3` is required
```shell
$ pip3 install roboticstoolbox-python==1.0.3
```