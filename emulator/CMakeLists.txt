cmake_minimum_required(VERSION 3.6)

add_library(emulator Device.cc Emulator.cc System.cc Video.cc shader_utils.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
