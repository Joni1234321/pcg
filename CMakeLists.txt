cmake_minimum_required(VERSION 3.20)

project(pcg VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME}
        source/main.cpp
        source/factory.cpp
        source/game.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source)

option(DOWNLOAD_DEPENDENCIES "Download dependencies" TRUE)
if(DOWNLOAD_DEPENDENCIES)
    include(FetchContent)
    FetchContent_Declare(
            SDL3
            GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
            GIT_TAG "preview-3.1.6"
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3)
    FetchContent_Declare(
            SDL3_ttf
            GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
            GIT_TAG "main"
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3_ttf)
else()
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_ttf CONFIG REQUIRED)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3_ttf::SDL3_ttf)
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3_ttf::SDL3_ttf-shared> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM
)

