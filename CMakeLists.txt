cmake_minimum_required(VERSION 3.25)

project(pcg VERSION 1.0)

option(SET_WARNINGS_ENABLED "Enable warnings" FALSE)
option(SET_CLANG_TIDY "Enable clang tidy" FALSE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ccache
find_program(CCACHE_PROGRAM ccache)
message(STATUS "ccache found: ${CCACHE_PROGRAM}")
set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})

if (SET_WARNINGS_ENABLED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wall -Wextra -Wpedantic -Werror")
endif ()


# libs
if (EXISTS ${CMAKE_SOURCE_DIR}/3rdparty/SDL3/CMakeLists.txt)
	add_subdirectory(3rdparty/SDL3 EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/SDL3_ttf EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/SDL3_image EXCLUDE_FROM_ALL)
else ()
	message(STATUS "3rdparty/ not found, fetching SDL3 via FetchContent...")
	include(FetchContent)
	fetchcontent_declare(SDL3
	                     GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
	                     GIT_TAG "release-3.4.8" EXCLUDE_FROM_ALL)
	fetchcontent_makeavailable(SDL3)
	fetchcontent_declare(SDL3_ttf
	                     GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
	                     GIT_TAG "release-3.2.2" EXCLUDE_FROM_ALL)
	fetchcontent_makeavailable(SDL3_ttf)
	fetchcontent_declare(SDL3_image
	                     GIT_REPOSITORY "https://github.com/libsdl-org/SDL_image.git"
	                     GIT_TAG "release-3.4.4" EXCLUDE_FROM_ALL)
	fetchcontent_makeavailable(SDL3_image)
endif ()

add_executable(pcg
               src/main.cpp
               src/g_clickcore.cpp
               src/g_cosmoclick.cpp
               src/g_commandstrike.cpp
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