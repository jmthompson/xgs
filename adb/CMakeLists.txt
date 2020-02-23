cmake_minimum_required(VERSION 3.6)

add_library(adb ADB.cc)
target_compile_features(adb PUBLIC cxx_std_17)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
