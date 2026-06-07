cmake_minimum_required(VERSION 3.10)
project(wscpp_tests_integration LANGUAGES CXX)

add_executable(wscpp_test_integration
    test_client_server.cpp
    test_rfc6455_protocol.cpp
    test_tls.cpp
)

target_include_directories(wscpp_test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/tls
)

target_compile_definitions(wscpp_test_integration PRIVATE
    WSCPP_TLS_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/tls/fixtures"
)

target_link_libraries(wscpp_test_integration PRIVATE
    wscpp
    GTest::gtest_main
)

if(TARGET asio::asio)
    target_link_libraries(wscpp_test_integration PRIVATE asio::asio)
    target_compile_definitions(wscpp_test_integration PRIVATE ASIO_STANDALONE)
endif()

include(GoogleTest)
gtest_discover_tests(wscpp_test_integration PROPERTIES TIMEOUT 30)

if(WSCPP_ENABLE_STRESS_TESTS)
    add_executable(wscpp_test_stress
        test_stress.cpp
    )
    target_link_libraries(wscpp_test_stress PRIVATE
        wscpp
        GTest::gtest_main
    )
    if(TARGET asio::asio)
        target_link_libraries(wscpp_test_stress PRIVATE asio::asio)
        target_compile_definitions(wscpp_test_stress PRIVATE ASIO_STANDALONE)
    endif()
    gtest_discover_tests(wscpp_test_stress)
endif()
