cmake_minimum_required(VERSION 3.10)
project(wscpp_tests_unit LANGUAGES CXX)

# Create test executable
add_executable(wscpp_test_unit
    test_version.cpp
    test_asio.cpp
    test_openssl.cpp
    test_frame.cpp
)

target_link_libraries(wscpp_test_unit
    wscpp
    GTest::gtest_main
)

# Add test
include(GoogleTest)
gtest_discover_tests(wscpp_test_unit)
