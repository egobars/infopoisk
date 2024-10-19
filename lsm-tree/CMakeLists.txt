cmake_minimum_required(VERSION 3.16)
project(lsm_tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_subdirectory(b-tree)
add_subdirectory(common)
add_subdirectory(disk_component)
add_subdirectory(lsm-tree)

add_executable(
    main
    main.cpp
)
