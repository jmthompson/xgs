cmake_minimum_required(VERSION 3.6)

add_library(M65816 Processor.cc)
target_compile_features(M65816 PUBLIC cxx_std_17)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
