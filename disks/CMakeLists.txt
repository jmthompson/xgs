cmake_minimum_required(VERSION 3.6)

add_library(disks Disk35.cc Disk525.cc IWM.cc Smartport.cc VirtualDisk.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
