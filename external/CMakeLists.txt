cmake_minimum_required(VERSION 4.2.2)

# Enable experimental `import std` support. The UUID is specific to the CMake
# version and MUST be set before the first project() call.
# https://gitlab.kitware.com/cmake/cmake/-/blob/master/Help/dev/experimental.rst
# https://gitlab.kitware.com/cmake/cmake/-/blob/v4.2.2/Help/dev/experimental.rstmaster repo/help/dev/experimental.rst
if(CMAKE_VERSION VERSION_EQUAL 4.2.2)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
else()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
endif()

project(abc VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)

add_executable(abc)
target_sources(abc PRIVATE abc.cpp)
set_property(TARGET abc PROPERTY CXX_MODULE_STD ON)