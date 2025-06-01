cmake_minimum_required(VERSION 3.6)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(xgs)

option(Boost_USE_STATIC_LIBS "Use boost static libs" OFF) 
option(Boost_USE_STATIC_RUNTIME "Use boost static runtime" OFF)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(xgs src/main.cc)
target_compile_features(xgs PUBLIC cxx_std_17)

include_directories(${PROJECT_SOURCE_DIR}/include)

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

if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(SDL2 REQUIRED)

set(OpenGL_GL_PREFERENCE GLVND) #needed for some linux systems
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(xgs ${OPENGL_LIBRARIES})

set(Boost_USE_MULTITHREADED ON)

include_directories(${SDL2_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
include_directories(third_party/glm)
include_directories(third_party/imgui)

add_subdirectory(src/core)
add_subdirectory(src/cpu)
add_subdirectory(src/misc)
add_subdirectory(src/sound)
add_subdirectory(src/storage)
add_subdirectory(src/video)
add_subdirectory(third_party/imgui)

target_link_libraries(xgs imgui ${Boost_LIBRARIES} ${SDL2_LIBRARIES} Boost::program_options)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(xgs stdc++fs) #<filesystem>
endif()
