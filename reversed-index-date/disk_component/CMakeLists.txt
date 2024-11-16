add_library(disk_component component.cpp)

target_link_libraries(disk_component PUBLIC roaring)

target_include_directories(disk_component PUBLIC include)

add_subdirectory(ut)
