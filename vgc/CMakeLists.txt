cmake_minimum_required(VERSION 3.6)

add_library(vgc Text40Col.cc Text80Col.cc Lores.cc DblLores.cc Hires.cc DblHires.cc SuperHires.cc VGC.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
