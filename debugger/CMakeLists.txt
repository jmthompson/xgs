cmake_minimum_required(VERSION 3.6)

add_library(debugger Debugger.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
