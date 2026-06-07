cmake_minimum_required(VERSION 3.10)
project(wscpp_tests_unit LANGUAGES CXX)

set(WSCPP_UNIT_TEST_SOURCES
    test_version.cpp
    test_frame.cpp
    test_handshake.cpp
    test_connection.cpp
    test_client.cpp
    test_server.cpp
    test_utf8.cpp
    test_permessage_deflate.cpp
)

if(WSCPP_USE_ASIO)
    list(APPEND WSCPP_UNIT_TEST_SOURCES test_asio.cpp test_openssl.cpp)
else()
    list(APPEND WSCPP_UNIT_TEST_SOURCES test_linux_socket.cpp)
endif()

add_executable(wscpp_test_unit ${WSCPP_UNIT_TEST_SOURCES})

target_link_libraries(wscpp_test_unit PRIVATE
    wscpp
    GTest::gtest_main
)

if(TARGET asio::asio)
    target_link_libraries(wscpp_test_unit PRIVATE asio::asio)
    target_compile_definitions(wscpp_test_unit PRIVATE ASIO_STANDALONE)
endif()

wscpp_target_strict_warnings(wscpp_test_unit)

include(GoogleTest)
gtest_discover_tests(wscpp_test_unit)
