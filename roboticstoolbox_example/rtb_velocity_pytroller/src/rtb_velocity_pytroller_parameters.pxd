# distutils: language = c++

from libcpp.vector cimport vector
from libcpp.string cimport string

cdef extern from "/home/mcbed/dev/github_dev/pytroller_examples/build/rtb_velocity_pytroller/rtb_velocity_pytroller_parameters/include/rtb_velocity_pytroller_parameters.hpp" namespace "rtb_velocity_pytroller":
	cdef struct Params:
		vector[string] joints
		string interface_name
		string command_topic_name
		string command_topic_type
