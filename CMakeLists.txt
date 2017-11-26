cmake_minimum_required(VERSION 3.6)

project(xgs)
add_executable(xgs emulator/main.cc)

include_directories(${PROJECT_SOURCE_DIR})

set(xgs_VERSION_MAJOR 0)
set(xgs_VERSION_MINOR 60)
add_definitions(-DVERSION_MAJOR=${xgs_VERSION_MAJOR} -DVERSION_MINOR=${xgs_VERSION_MINOR})

# enable the debugger
add_definitions(-DENABLE_DEBUGGER)

# Test endianness of the target system
INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
if(WORDS_BIGENDIAN)
    add_definitions(-DBIGENDIAN)
endif()

# Check for Raspberry Pi
if(BCMHOST)
    add_definitions(-DRPI)
    include_directories(/opt/vc/include)
    target_link_libraries(xgs
        /opt/vc/lib/libbcm_host.so
        /opt/vc/lib/libbrcmGLESv2.so
        /opt/vc/lib/libbrcmEGL.so
    )
else()
    find_package(OpenGL REQUIRED)
    if (OPENGL_FOUND)
        include_directories($(OPENGL_INCLUDE_DIRS))
        target_link_libraries(xgs ${OPENGL_LIBRARIES})
    endif()
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

#find_package(GLEW REQUIRED)
#if (GLEW_FOUND)
#    include_directories($(GLEW_INCLUDE_DIRS))
#    target_link_libraries(xgs ${GLEW_LIBRARIES})
#endif()

add_subdirectory(M65816)
add_subdirectory(emulator)
add_subdirectory(debugger)
add_subdirectory(adb)
add_subdirectory(disks)
add_subdirectory(doc)
add_subdirectory(mega2)
add_subdirectory(vgc)

add_subdirectory(imgui)

target_link_libraries(xgs emulator M65816 debugger disks adb doc mega2 vgc imgui)
