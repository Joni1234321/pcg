cmake_minimum_required(VERSION 3.20)

project(PCG VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCE_FILES
        "source/*.cpp"
        "source/*.hpp"
)

add_executable(main ${SOURCE_FILES})