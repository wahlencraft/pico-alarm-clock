cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(pico_alarm_clock C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

include("../TM1637-pico/PicoTM1637.cmake")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src)
