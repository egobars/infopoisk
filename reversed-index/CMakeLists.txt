# Copy this file as CMakeLists.txt in
# some directory. With a recent CMake
# (at least 3.15), in this directory do
#
# cmake -B build .
# cmake --build build
# ./build/repro
#
#
cmake_minimum_required(VERSION 3.15)


project(ReversedIndex VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  roaring
  GIT_REPOSITORY https://github.com/RoaringBitmap/CRoaring.git
  GIT_TAG v2.0.4
  GIT_SHALLOW TRUE)

set(ENABLE_ROARING_TESTS OFF CACHE INTERNAL "")

set(ROARING_BUILD_STATIC ON CACHE INTERNAL "")
FetchContent_MakeAvailable(roaring)

FetchContent_GetProperties(roaring)
SET(CPP_ROARING_HEADERS ${roaring_SOURCE_DIR}/cpp/roaring64map.hh  ${roaring_SOURCE_DIR}/cpp/roaring.hh)
file(COPY  ${CPP_ROARING_HEADERS} DESTINATION ${roaring_SOURCE_DIR}/include/roaring)

enable_testing()

add_subdirectory(common)
add_subdirectory(b-tree)
add_subdirectory(disk_component)
add_subdirectory(index)
