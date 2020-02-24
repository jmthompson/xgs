cmake_minimum_required(VERSION 3.6)

add_library(emulator Device.cc Emulator.cc GUI.cc System.cc Video.cc shader_utils.cc)
target_compile_features(emulator PUBLIC cxx_std_17)
target_include_directories(emulator PRIVATE ../third_party/glm 
                                            ../third_party/galogen/generated_files)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
