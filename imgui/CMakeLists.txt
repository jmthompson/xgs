cmake_minimum_required(VERSION 3.6)

#general settings
file(GLOB IMGUI_SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_demo.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_sdl.cpp
    misc/freetype/imgui_freetype.cpp
)

find_package(Freetype REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
add_compile_definitions(IMGUI_FREETYPE)
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_CUSTOM="gl/gl.h")
add_library(imgui SHARED ${IMGUI_SOURCES})
target_link_libraries(imgui ${IMGUI_LIBRARIES} ${FREETYPE_LIBRARIES} ${SDL2_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
