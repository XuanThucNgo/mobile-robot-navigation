execute_process(COMMAND "/home/ross/catkin_ws/build/rqt_robot_steering/catkin_generated/python_distutils_install.sh" RESULT_VARIABLE res)

if(NOT res EQUAL 0)
  message(FATAL_ERROR "execute_process(/home/ross/catkin_ws/build/rqt_robot_steering/catkin_generated/python_distutils_install.sh) returned error code ")
endif()
