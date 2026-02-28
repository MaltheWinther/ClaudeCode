
#Root CMakeLists.txt for sys_hat project

cmake_minimum_required(VERSION 3.15)
project(sys_hat C CXX)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()

# Add subdirectories (each subdir should have its own CMakeLists.txt)
include_directories("${CMAKE_BINARY_DIR}/copied_headers")
add_subdirectory(com)
add_subdirectory(BMI160_SensorAPI)
add_subdirectory(SSD1306_OLED_RPI)
