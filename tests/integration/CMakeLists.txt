cmake_minimum_required(VERSION 3.10)
project(wscpp_tests_integration LANGUAGES CXX)

# Create test executable
add_executable(wscpp_test_integration
    test_client_server.cpp
)

target_link_libraries(wscpp_test_integration
    wscpp
    GTest::gtest_main
)

# Add test
include(GoogleTest)
gtest_discover_tests(wscpp_test_integration)
