cmake_minimum_required(VERSION 3.30)

project(pcg VERSION 1.0)

option(SET_WARNINGS_ENABLED "Enable warnings" FALSE)
option(SET_CLANG_TIDY "Enable clang tidy" FALSE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (WIN32 OR MINGW OR MSYS)
    foreach (FLAG_VAR CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
        if (DEFINED ${FLAG_VAR})
            string(REPLACE "-stdlib=libc++" "" ${FLAG_VAR} "${${FLAG_VAR}}")
            set(${FLAG_VAR} "${${FLAG_VAR}}" CACHE STRING "" FORCE)
        endif ()
    endforeach ()
else ()
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif ()

# ccache
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
endif ()

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
fetchcontent_makeavailable(SDL3 SDL3_ttf SDL3_image)

add_executable(pcg
               src/pcg.cpp
              # src/g_clickcore.cpp
              # src/g_cosmoclick.cpp
               #src/g_commandstrike.cpp
              # src/g_battlesim.cpp
               src/8_hex/g_hex.cpp
              # src/g_hexbattle.cpp
              # src/g_factory.cpp
               #src/g_game.cpp
               src/1_systems/t_debug_system.cpp
               src/1_systems/r_ui_node.cpp
)

target_sources(pcg
               PRIVATE
               FILE_SET cxx_modules TYPE CXX_MODULES FILES
               src/8_hex/r_hex.ixx
)

target_link_libraries(pcg
                      PRIVATE SDL3::SDL3
                      PRIVATE SDL3_ttf::SDL3_ttf
                      PRIVATE SDL3_image::SDL3_image
)

target_include_directories(pcg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(pcg PRIVATE -Wno-unsequenced)

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

if (WIN32 OR MINGW OR MSYS OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    # Find runtime DLLs by scanning the actual import table of pcg.exe and copy
    # them next to it. This ships *only* what the exe really needs (SDL3, image
    # libs, libc++, etc.) instead of bulk-copying the entire compiler prefix.
    get_filename_component(CXX_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(PCG_RUNTIME_SEARCH_DIRS "${CXX_COMPILER_BIN_DIR}")
    if (CMAKE_PREFIX_PATH)
        list(APPEND PCG_RUNTIME_SEARCH_DIRS "${CMAKE_PREFIX_PATH}/bin")
    endif ()

    # Dev convenience: do the same bundling into the build dir so double-click
    # works straight from build/xd/.
    add_custom_command(
            TARGET pcg POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                    -DEXECUTABLE=$<TARGET_FILE:pcg>
                    -DOUTPUT_DIR=$<TARGET_FILE_DIR:pcg>
                    "-DSEARCH_DIRS=${PCG_RUNTIME_SEARCH_DIRS}"
                    -P "${CMAKE_SOURCE_DIR}/cmake/bundle_runtime_deps.cmake"
            COMMENT "Bundling runtime DLLs next to pcg.exe"
            VERBATIM
    )
endif ()

# ---------------------------------------------------------------------------
# Shipping / packaging  (AI-generated section)
# ---------------------------------------------------------------------------
# Usage:
#     cmake --install build/xd --prefix release      # plain folder
#     cmake --build  build/xd --target package       # zip in build/xd/
#
# Produces a self-contained folder of ~10-20 MB you can hand to anyone on the
# same OS — just unzip and run pcg.exe.

install(TARGETS pcg RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets" DESTINATION .)

if (WIN32 OR MINGW OR MSYS OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    install(CODE "
        set(EXECUTABLE  \"\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:pcg>\")
        set(OUTPUT_DIR  \"\${CMAKE_INSTALL_PREFIX}\")
        set(SEARCH_DIRS \"${PCG_RUNTIME_SEARCH_DIRS}\")
        include(\"${CMAKE_SOURCE_DIR}/cmake/bundle_runtime_deps.cmake\")
    ")
endif ()

set(CPACK_PACKAGE_NAME              "pcg")
set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME         "pcg-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_GENERATOR                 "ZIP")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "pcg")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)


add_executable(meshdemo src/g_meshdemo.cpp)
target_link_libraries(meshdemo
                      PRIVATE SDL3::SDL3
                      PRIVATE SDL3_ttf::SDL3_ttf)
target_include_directories(meshdemo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)