cmake_minimum_required(VERSION 3.10)
project(wscpp_tests_integration LANGUAGES CXX)

add_executable(wscpp_test_integration
    test_client_server.cpp
    test_rfc6455_protocol.cpp
)

target_link_libraries(wscpp_test_integration
    wscpp
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wscpp_test_integration)

if(WSCPP_ENABLE_STRESS_TESTS)
    add_executable(wscpp_test_stress
        test_stress.cpp
    )
    target_link_libraries(wscpp_test_stress
        wscpp
        GTest::gtest_main
    )
    gtest_discover_tests(wscpp_test_stress)
endif()
