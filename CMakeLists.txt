cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(uav_x C CXX ASM)
set(CMAKE_C_STNDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

pico_sdk_init()
#include_directories(/Users/kouhei/pico/project/eigen)

add_executable(uav_x
  main.cpp
  sensor.cpp
  ekf.cpp
  lsm9ds1_reg.c
  pwm.cpp
  radio.cpp
  control.cpp
  pid.cpp
)

target_link_libraries(uav_x 
  hardware_spi 
  hardware_pwm 
  hardware_uart 
  pico_stdlib
  pico_multicore)

pico_enable_stdio_usb(uav_x 1)
pico_enable_stdio_uart(uav_x 0)

pico_add_extra_outputs(uav_x)

