cmake_minimum_required(VERSION 3.25)

project(pcg VERSION 1.0)

option(SET_WARNINGS_ENABLED "Enable warnings" FALSE)
option(SET_CLANG_TIDY "Enable clang tidy" FALSE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")

# ccache
find_program(CCACHE_PROGRAM ccache REQUIRED)
set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})

# libs — downloaded once into 3rdparty/, reused across all build dirs
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/3rdparty" CACHE PATH "" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
include(FetchContent)
fetchcontent_declare(SDL3
                     GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
                     GIT_TAG "release-3.4.8" EXCLUDE_FROM_ALL)
set(SDL3TTF_FREETYPE_VENDORED ON  CACHE BOOL "" FORCE)
set(SDL3TTF_HARFBUZZ          OFF CACHE BOOL "" FORCE)
fetchcontent_declare(SDL3_ttf
                     GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
                     GIT_TAG "release-3.2.2" EXCLUDE_FROM_ALL)
fetchcontent_declare(SDL3_image
                     GIT_REPOSITORY "https://github.com/libsdl-org/SDL_image.git"
                     GIT_TAG "release-3.4.4" EXCLUDE_FROM_ALL)
fetchcontent_declare(imgui
                     GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
                     GIT_TAG "v1.91.6" EXCLUDE_FROM_ALL)
fetchcontent_makeavailable(SDL3 SDL3_ttf SDL3_image imgui)

# imgui ships without a CMakeLists.txt — build a static lib from its sources
# plus the SDL3 + SDLRenderer3 backends we use.
add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(imgui PUBLIC
                           ${imgui_SOURCE_DIR}
                           ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

add_executable(pcg
               src/pcg.cpp
              # src/g_clickcore.cpp
              # src/g_cosmoclick.cpp
               #src/g_commandstrike.cpp
              # src/g_battlesim.cpp
               src/g_hex.cpp
              # src/g_hexbattle.cpp
              # src/g_factory.cpp
               #src/g_game.cpp
               src/1_systems/t_debug_system.cpp
               src/1_systems/r_ui_node.cpp
)

target_link_libraries(pcg
                      PRIVATE SDL3::SDL3
                      PRIVATE SDL3_ttf::SDL3_ttf
                      PRIVATE SDL3_image::SDL3_image
                      PRIVATE imgui
)

target_include_directories(pcg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (SET_WARNINGS_ENABLED)
	target_compile_options(pcg PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif ()

# clang-tidy — resolved after add_executable so it only applies to pcg
if (SET_CLANG_TIDY)
	find_program(CLANG_TIDY_PROGRAM clang-tidy REQUIRED)
	set_target_properties(pcg PROPERTIES
	                      CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM};--extra-arg=-std=c++26;--use-color"
	)

endif ()

add_custom_command(
		TARGET pcg POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:pcg>/assets"
		COMMENT "Copying assets to output directory"
		VERBATIM
)