cmake_minimum_required(VERSION 3.6)

project(xgs)
add_executable(xgs main.cc)

include_directories(${PROJECT_SOURCE_DIR})

set(xgs_VERSION_MAJOR 0)
set(xgs_VERSION_MINOR 60)

add_definitions(-DENABLE_DEBUGGER)

# Test endianness of the target system
INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
if(WORDS_BIGENDIAN)
    add_definitions(-DBIGENDIAN)
endif()

find_package(Boost 1.60.0 COMPONENTS filesystem program_options system)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(xgs ${Boost_LIBRARIES})
endif()

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
  include_directories(${SDL2_INCLUDE_DIRS})
  target_link_libraries(xgs ${SDL2_LIBRARIES})
endif()

add_subdirectory(M65816)
add_subdirectory(xgscore)
add_subdirectory(debugger)
add_subdirectory(disks)
add_subdirectory(video)

target_link_libraries(xgs xgscore M65816 debugger disks video)
