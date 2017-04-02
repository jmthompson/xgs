cmake_minimum_required(VERSION 3.6)

INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

if(WORDS_BIGENDIAN)
    add_definitions(-DBIGENDIAN)
endif()

add_definitions(-DENABLE_DEBUGGER)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

find_package(Boost 1.60.0 COMPONENTS filesystem program_options system)
find_package(SDL2 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS}) 

add_subdirectory(M65816)
add_subdirectory(xgscore)
add_subdirectory(debugger)
add_subdirectory(disks)
add_subdirectory(video)

add_executable(xgs main.cc)
target_link_libraries(xgs xgscore M65816 debugger disks video ${Boost_LIBRARIES} ${SDL2_LIBRARIES})
