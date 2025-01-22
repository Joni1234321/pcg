cmake_minimum_required(VERSION 3.25)

project(pcg VERSION 1.0)

option(DOWNLOAD_DEPENDENCIES "Download dependencies" TRUE)
option(_SET_WARNINGS_ENABLED "Enable warnings" FALSE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (_SET_WARNINGS_ENABLED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wall -Wextra -Wpedantic -Werror")
endif ()

set(imgui_dir ${CMAKE_SOURCE_DIR}/lib/imgui)
set(resources_dir ${CMAKE_SOURCE_DIR}/resources)
add_executable(${PROJECT_NAME}
               source/main.cpp
               source/g_factory.cpp
               source/g_game.cpp
               source/m_frame.cpp
               source/r_ui.cpp
               source/r_ui_node.cpp
               source/r_ui_element.cpp
               ${imgui_dir}/imgui.cpp
               ${imgui_dir}/imgui_demo.cpp
               ${imgui_dir}/imgui_draw.cpp
               ${imgui_dir}/imgui_tables.cpp
               ${imgui_dir}/imgui_widgets.cpp
               ${imgui_dir}/imgui_impl_sdl3.cpp
               ${imgui_dir}/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

if (DOWNLOAD_DEPENDENCIES)
	include(FetchContent)
	fetchcontent_declare(
			SDL3
			GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
			GIT_TAG "preview-3.1.6"
			EXCLUDE_FROM_ALL
	)
	fetchcontent_declare(
			SDL3_ttf
			GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
			GIT_TAG "main"
			EXCLUDE_FROM_ALL
	)
	fetchcontent_makeavailable(SDL3)
	fetchcontent_makeavailable(SDL3_ttf)

else ()
	find_package(SDL3 CONFIG REQUIRED)
	find_package(SDL3_ttf CONFIG REQUIRED)
endif ()

target_link_libraries(${PROJECT_NAME}
                      PRIVATE SDL3::SDL3
                      PRIVATE SDL3_ttf::SDL3_ttf
)

add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E echo "Copying files..."
		COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${PROJECT_NAME}>
		COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3_ttf::SDL3_ttf-shared> $<TARGET_FILE_DIR:${PROJECT_NAME}>
		COMMAND "${CMAKE_COMMAND}" -E echo "Finished copying files"
		VERBATIM
)