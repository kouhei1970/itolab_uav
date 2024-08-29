# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_copter C CXX ASM)
set(CMAKE_C_STNDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

pico_sdk_init()
include_directories(/Users/kouhei/pico/project/eigen)

add_executable(pico_copter
  main.cpp
  sensor.cpp
  ekf.cpp
  lsm9ds1_reg.c
  pwm.cpp
  radio.cpp
  control.cpp
)

target_link_libraries(pico_copter 
  hardware_spi 
  hardware_pwm 
  hardware_uart 
  pico_stdlib
  pico_multicore)

pico_enable_stdio_usb(pico_copter 1)
pico_enable_stdio_uart(pico_copter 0)

pico_add_extra_outputs(pico_copter)

