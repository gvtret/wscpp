cmake_minimum_required(VERSION 3.10)
project(wscpp_examples_server LANGUAGES CXX)

# Create server example
add_executable(wscpp_example_server
    server.cpp
)

target_link_libraries(wscpp_example_server
    wscpp
)
