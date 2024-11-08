add_library(common common.cpp)

target_link_libraries(common PUBLIC roaring)

target_include_directories(common PUBLIC include)
