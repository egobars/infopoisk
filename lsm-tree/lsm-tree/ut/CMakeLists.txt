add_executable(
    lsm_tree_test
    test.cpp
)

target_link_libraries(
    lsm_tree_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(lsm_tree_test)
