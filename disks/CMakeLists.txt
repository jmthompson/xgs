cmake_minimum_required(VERSION 3.6)

add_library(disks Disk35.cc Disk525.cc IWM.cc Smartport.cc VirtualDisk.cc)
target_compile_features(disks PUBLIC cxx_std_17)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
