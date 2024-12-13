cmake_minimum_required(VERSION 3.6)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(xgs)

option(Boost_USE_STATIC_LIBS "Use boost static libs" OFF) 
option(Boost_USE_STATIC_RUNTIME "Use boost static runtime" OFF)

add_executable(xgs emulator/main.cc)
target_compile_features(xgs PUBLIC cxx_std_17)

include_directories(${PROJECT_SOURCE_DIR})

set(xgs_VERSION_MAJOR 0)
set(xgs_VERSION_MINOR 70)
add_definitions(-DVERSION_MAJOR=${xgs_VERSION_MAJOR} -DVERSION_MINOR=${xgs_VERSION_MINOR})

# enable the debugger
add_definitions(-DENABLE_DEBUGGER)

# Test endianness of the target system
INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
if(WORDS_BIGENDIAN)
    add_definitions(-DBIGENDIAN)
endif()

set(OpenGL_GL_PREFERENCE GLVND) #needed for some linux systems
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(xgs ${OPENGL_LIBRARIES})

set(Boost_USE_MULTITHREADED ON)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
include_directories(third_party/glm)
include_directories(imgui)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -pg")

add_subdirectory(adb)
add_subdirectory(debugger)
add_subdirectory(disks)
add_subdirectory(doc)
add_subdirectory(emulator)
add_subdirectory(gl)
add_subdirectory(M65816)
add_subdirectory(mega2)
add_subdirectory(scc)
add_subdirectory(vgc)

#add_subdirectory(third_party/galogen)
add_subdirectory(imgui)

target_link_libraries(xgs   emulator #gcc needs this to be first
                            adb
                            debugger 
                            doc 
                            disks  
                            M65816 
                            mega2 
                            scc
                            vgc 
                            imgui 
                            gl
                            ${Boost_LIBRARIES} 
                            ${SDL2_LIBRARIES})

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(xgs stdc++fs) #<filesystem>
endif()
