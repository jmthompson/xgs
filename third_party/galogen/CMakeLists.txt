cmake_minimum_required(VERSION 3.11)

project(galogen)

add_executable(galogen galogen.cpp third_party/tinyxml2.cpp)
target_compile_features(galogen PUBLIC cxx_std_17)

if(BCMHOST)
    add_library(gl_extension_loader generated_files/gles_2.c)
else()
    add_library(gl_extension_loader generated_files/gl_33.c)
endif()


