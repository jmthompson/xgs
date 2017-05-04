cmake_minimum_required(VERSION 3.6)

add_library(xgscore ADB.cc Device.cc DOC.cc Emulator.cc Mega2.cc System.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
