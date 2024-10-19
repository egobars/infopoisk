add_library(lsm_tree tree.cpp)

target_include_directories(lsm_tree PUBLIC include)

add_subdirectory(ut)
