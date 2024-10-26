add_executable(
    component_test
    test.cpp
)

target_link_libraries(
    component_test
    GTest::gtest_main
)
target_link_libraries(component_test roaring)

include(GoogleTest)
gtest_discover_tests(component_test)
