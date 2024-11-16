add_library(index index.cpp)

target_include_directories(index PUBLIC include)

target_link_libraries(index PUBLIC roaring)

add_subdirectory(ut)
