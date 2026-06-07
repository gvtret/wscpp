cmake_minimum_required(VERSION 3.10)
project(wscpp_tests_regression LANGUAGES CXX)

# Create test executable
add_executable(wscpp_test_regression
    test_rfc6455_compliance.cpp
)

target_link_libraries(wscpp_test_regression
    wscpp
    GTest::gtest_main
)

wscpp_target_strict_warnings(wscpp_test_regression)

# Add test
include(GoogleTest)
gtest_discover_tests(wscpp_test_regression)
