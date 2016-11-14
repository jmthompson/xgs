cmake_minimum_required(VERSION 3.6)

add_library(M65816 M65816.cc ExecuteOpcode.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ExecuteOpcode.cc
  DEPENDS ExecuteOpcode.cc.m4 macros.m4 opcodes.m4
  COMMAND m4 -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/ExecuteOpcode.cc.m4 > ${CMAKE_CURRENT_BINARY_DIR}/ExecuteOpcode.cc
)
