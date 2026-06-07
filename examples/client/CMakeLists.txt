cmake_minimum_required(VERSION 3.10)
project(wscpp_examples_client LANGUAGES CXX)

# Create client example
add_executable(wscpp_example_client
    client.cpp
)

target_link_libraries(wscpp_example_client
    wscpp
)

wscpp_target_strict_warnings(wscpp_example_client)
