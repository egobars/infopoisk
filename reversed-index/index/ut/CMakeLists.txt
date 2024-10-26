add_executable(
    index_test
    test.cpp
)

target_link_libraries(
    index_test
    GTest::gtest_main
)
target_link_libraries(index_test roaring)

include(GoogleTest)
gtest_discover_tests(index_test)
