cmake_minimum_required(VERSION 3.6)

add_library(doc DOC.cc)
target_compile_features(doc PUBLIC cxx_std_17)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
