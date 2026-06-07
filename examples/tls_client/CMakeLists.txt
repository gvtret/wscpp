cmake_minimum_required(VERSION 3.10)

add_executable(wscpp_example_tls_client client.cpp)
target_link_libraries(wscpp_example_tls_client PRIVATE wscpp)
