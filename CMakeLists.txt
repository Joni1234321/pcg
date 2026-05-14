cmake_minimum_required(VERSION 3.25)

project(pcg VERSION 1.0)

option(_SET_WARNINGS_ENABLED "Enable warnings" FALSE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (_SET_WARNINGS_ENABLED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wall -Wextra -Wpedantic -Werror")
endif ()


# libs
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL3TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/SDL3_ttf EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/SDL3_image EXCLUDE_FROM_ALL)

add_executable(pcg
               src/main.cpp
               src/g_clickcore.cpp
               src/g_cosmoclick.cpp
               src/g_factory.cpp
               src/g_game.cpp
               src/1_systems/t_debug_system.cpp
               src/1_systems/r_ui_node.cpp
)

target_link_libraries(pcg
                      PRIVATE SDL3::SDL3
                      PRIVATE SDL3_ttf::SDL3_ttf
                      PRIVATE SDL3_image::SDL3_image
)

target_include_directories(pcg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_command(
		TARGET pcg POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:pcg>/assets"
		COMMENT "Copying assets to output directory"
		VERBATIM
)