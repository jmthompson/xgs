cmake_minimum_required(VERSION 3.6)

add_library(gl gl.c)
target_compile_features(emulator PUBLIC cxx_std_17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
