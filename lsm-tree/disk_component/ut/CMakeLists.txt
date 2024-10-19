add_executable(
    component_test
    test.cpp
)

target_link_libraries(
    component_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(component_test)
