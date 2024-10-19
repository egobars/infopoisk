add_library(disk_component component.cpp)

target_include_directories(disk_component PUBLIC include)

add_subdirectory(ut)
