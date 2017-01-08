cmake_minimum_required(VERSION 3.6)

add_library(xgscore ADB.cc Device.cc Disk35.cc Disk525.cc DOC.cc Emulator.cc IWM.cc Mega2.cc Smartport.cc System.cc VGC.cc VGC_TextMode.cc VirtualDisk.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
