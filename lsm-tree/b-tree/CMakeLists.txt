add_library(b_tree b_tree.cpp)

target_include_directories(b_tree PUBLIC include)

add_subdirectory(ut)
