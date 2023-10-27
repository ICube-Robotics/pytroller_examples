cmake_minimum_required(VERSION 3.16)
project(rtb_velocity_pytroller LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  controller_interface
  generate_parameter_library
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  realtime_tools
)

find_package(ament_cmake REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

# Set the parameter header file name
set(PARAM_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/rtb_velocity_pytroller_parameters/include)
set(PARAM_HEADER_FILE ${PARAM_INCLUDE_DIR}/rtb_velocity_pytroller_parameters.hpp)

# Make logic build directory
set(LOGIC_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}/rtb_velocity_pytroller_logic)
set(LOGIC_INCLUDE_DIR ${LOGIC_DIR}/include/rtb_velocity_pytroller)
file(MAKE_DIRECTORY ${LOGIC_DIR})
file(MAKE_DIRECTORY ${LOGIC_INCLUDE_DIR})

file (REMOVE ${LOGIC_DIR}/rtb_velocity_pytroller_logic.cpp)
file (REMOVE ${LOGIC_DIR}/rtb_velocity_pytroller_logic.h)
file (REMOVE ${LOGIC_INCLUDE_DIR}/rtb_velocity_pytroller_logic.h)

add_custom_command(
  OUTPUT ${LOGIC_DIR}/rtb_velocity_pytroller_logic.cpp ${LOGIC_DIR}/rtb_velocity_pytroller_logic.h
  COMMAND cython3 -3 --cplus ${CMAKE_CURRENT_SOURCE_DIR}/src/rtb_velocity_pytroller_logic.pyx
    -o ${LOGIC_DIR}/rtb_velocity_pytroller_logic.cpp -I ${PARAM_INCLUDE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rtb_velocity_pytroller_logic.pyx
)

# Copy the header file into the include directory
add_custom_command(
  OUTPUT ${LOGIC_INCLUDE_DIR}/rtb_velocity_pytroller_logic.h
  COMMAND ${CMAKE_COMMAND} -E copy
    ${LOGIC_DIR}/rtb_velocity_pytroller_logic.h ${LOGIC_INCLUDE_DIR}/rtb_velocity_pytroller_logic.h
  DEPENDS ${LOGIC_DIR}/rtb_velocity_pytroller_logic.h
)

generate_parameter_library(
  rtb_velocity_pytroller_parameters
  src/rtb_velocity_pytroller_parameters.yaml
)

file (REMOVE ${PARAM_INCLUDE_DIR}/rtb_velocity_pytroller_parameters.pxd)

# Generate the pxd for the library
add_custom_command(
  OUTPUT ${PARAM_INCLUDE_DIR}/rtb_velocity_pytroller_parameters.pxd
  COMMAND ros2 run pytroller_tools generate_pxd ${PARAM_INCLUDE_DIR}/rtb_velocity_pytroller_parameters.pxd ${PARAM_HEADER_FILE}
  DEPENDS ${PARAM_HEADER_FILE}
)

add_library(rtb_velocity_pytroller SHARED
  src/rtb_velocity_pytroller.cpp
  ${LOGIC_DIR}/rtb_velocity_pytroller_logic.cpp
  ${LOGIC_INCLUDE_DIR}/rtb_velocity_pytroller_logic.h
  ${PARAM_INCLUDE_DIR}/rtb_velocity_pytroller_parameters.pxd
)
target_compile_features(rtb_velocity_pytroller PUBLIC cxx_std_17)
target_include_directories(rtb_velocity_pytroller PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/rtb_velocity_pytroller>
)
target_include_directories(rtb_velocity_pytroller PUBLIC
  $<BUILD_INTERFACE:${LOGIC_DIR}/include>
  $<INSTALL_INTERFACE:include/rtb_velocity_pytroller>
)
target_link_libraries(rtb_velocity_pytroller PUBLIC
  rtb_velocity_pytroller_parameters
  ${PYTHON_LIBRARIES}
)
ament_target_dependencies(rtb_velocity_pytroller PUBLIC ${THIS_PACKAGE_INCLUDE_DEPENDS})
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(rtb_velocity_pytroller PRIVATE "PYTROLLER_BUILDING_DLL")
pluginlib_export_plugin_description_file(controller_interface controller_plugin.xml)


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_cmake_gmock REQUIRED)
  find_package(controller_manager REQUIRED)
  find_package(hardware_interface REQUIRED)
  find_package(ros2_control_test_assets REQUIRED)

  ament_lint_auto_find_test_dependencies()

  # Load test
  add_rostest_with_parameters_gmock(
    test_load_rtb_velocity_pytroller
    test/test_load_rtb_velocity_pytroller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_params.yaml
  )
  target_link_libraries(test_load_rtb_velocity_pytroller
    rtb_velocity_pytroller
  )
  ament_target_dependencies(test_load_rtb_velocity_pytroller
    controller_manager
    hardware_interface
    ros2_control_test_assets
  )

  # Controller test
  add_rostest_with_parameters_gmock(
    test_rtb_velocity_pytroller
    test/test_rtb_velocity_pytroller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_params.yaml
  )

  target_link_libraries(test_rtb_velocity_pytroller
    rtb_velocity_pytroller
  )

  ament_target_dependencies(test_load_rtb_velocity_pytroller
    controller_manager
    hardware_interface
  )

endif()

install(
  DIRECTORY ${LOGIC_DIR}/include
  DESTINATION include/rtb_velocity_pytroller
)
install(
  DIRECTORY include/
  DESTINATION include/rtb_velocity_pytroller
)
install(
  TARGETS
    rtb_velocity_pytroller
    rtb_velocity_pytroller_parameters
  EXPORT export_rtb_velocity_pytroller
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  INCLUDES DESTINATION include
)

ament_export_targets(export_rtb_velocity_pytroller HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
