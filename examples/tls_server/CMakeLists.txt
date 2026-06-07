cmake_minimum_required(VERSION 3.10)

add_executable(wscpp_example_tls_server server.cpp)
target_link_libraries(wscpp_example_tls_server PRIVATE wscpp)
