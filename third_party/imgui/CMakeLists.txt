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

set(OpenGL_GL_PREFERENCE GLVND) #needed for some linux systems
find_package(OpenGL REQUIRED)
#include_directories(${OPENGL_INCLUDE_DIRS})
#target_link_libraries(xgs ${OPENGL_LIBRARIES})

include_directories(${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/third_party/glm)
add_compile_definitions(IMGUI_FREETYPE)
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_CUSTOM="gl.h")
add_library(imgui SHARED ${IMGUI_SOURCES})
target_link_libraries(imgui ${OPENGL_LIBRARIES} ${IMGUI_LIBRARIES} ${FREETYPE_LIBRARIES} ${SDL2_LIBRARIES})
