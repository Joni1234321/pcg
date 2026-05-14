cmake_minimum_required(VERSION 3.25)

project(pcg VERSION 1.0)

option(DOWNLOAD_DEPENDENCIES "Download dependencies" TRUE)
option(_SET_WARNINGS_ENABLED "Enable warnings" FALSE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (_SET_WARNINGS_ENABLED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wall -Wextra -Wpedantic -Werror")
endif ()


add_subdirectory(3rdparty/SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/SDL3_ttf EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/SDL3_image EXCLUDE_FROM_ALL)

add_executable(pcg
               source/main.cpp
               source/g_clickcore.cpp
               source/g_cosmoclick.cpp
               source/g_factory.cpp
               source/g_game.cpp
               source/1_systems/t_debug_system.cpp
               source/1_systems/r_ui_node.cpp
)

target_link_libraries(pcg
                      PRIVATE SDL3::SDL3
                      PRIVATE SDL3_ttf::SDL3_ttf
                      PRIVATE SDL3_image::SDL3_image
)

set(resources_dir ${CMAKE_SOURCE_DIR}/resources)


target_include_directories(pcg
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

#add_custom_command(
#		TARGET pcg POST_BUILD
#		COMMAND "${CMAKE_COMMAND}" -E echo "Copying files..."
#		COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:pcg>
#		COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3_ttf::SDL3_ttf-shared> $<TARGET_FILE_DIR:pcg>
#		COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3_image::SDL3_image-shared> $<TARGET_FILE_DIR:pcg>
#		COMMAND "${CMAKE_COMMAND}" -E echo "Finished copying files"
#		VERBATIM
#)
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/build)