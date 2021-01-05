cmake_minimum_required(VERSION 3.6)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/backends)
	set(BACKENDS_FOLDER "backends/")
else()
	set(BACKENDS_FOLDER "examples/")
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp)
    set(TABLES_SOURCE "imgui_tables.cpp")
else()
    set(TABLES_SOURCE "")
endif()


#general settings
file(GLOB IMGUI_SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_demo.cpp
    imgui_widgets.cpp
    ${TABLES_SOURCE}
)

#optional adding freetype
option(IMGUI_FREETYPE "add Freetype2" OFF)

if(IMGUI_FREETYPE)
    FIND_PACKAGE(freetype REQUIRED PATHS ${FREETYPE_PATH})
    list(APPEND IMGUI_LIBRARIES freetype)
    list(APPEND IMGUI_SOURCES misc/freetype/imgui_freetype.cpp)
    add_definitions("-DCIMGUI_FREETYPE=1")
endif(IMGUI_FREETYPE)

#sdl2
list(APPEND IMGUI_SOURCES ${BACKENDS_FOLDER}imgui_impl_sdl.cpp)
if(DEFINED SDL_PATH)
	message(STATUS "SDL_PATH defined as " ${SDL_PATH})
	FIND_PACKAGE(SDL2 REQUIRED PATHS ${SDL_PATH})
	get_target_property(SDL_INCLUDE SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
	message(STATUS "sdlinclude is " ${SDL_INCLUDE})
	if ("${SDL_INCLUDE}" STREQUAL "" OR "${SDL_INCLUDE}" STREQUAL "SDL_INCLUDE-NOTFOUND") #if not found latest SDL2 cmake config use older
		message(STATUS "sdlinclude2 is " ${SDL2_INCLUDE_DIRS})
		include_directories(${SDL2_INCLUDE_DIRS})
		set(IMGUI_SDL_LIBRARY ${SDL2_LIBRARIES})
		message(STATUS IMGUI_SDL_LIBRARY ${SDL2_LIBRARIES})
	else()#use new one SDL2 config
		include_directories(${SDL_INCLUDE})
		set(IMGUI_SDL_LIBRARY SDL2::SDL2)
		set(SDL_MAIN SDL2::SDL2main)
		message(STATUS ${SDL_MAIN} ${IMGUI_SDL_LIBRARY})
	endif()
else(DEFINED SDL_PATH)
	message(STATUS "SDL_PATH not defined")
	set(IMGUI_SDL_LIBRARY SDL2)
endif(DEFINED SDL_PATH)


add_library(imgui SHARED ${IMGUI_SOURCES})
target_link_libraries(imgui ${IMGUI_LIBRARIES} ${IMGUI_SDL_LIBRARY})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
