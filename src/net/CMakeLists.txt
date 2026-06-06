cmake_minimum_required(VERSION 3.10)
project(wscpp_net LANGUAGES CXX)

# ASIO source files
set(WSCPP_NET_SOURCES
    asio_socket.cpp
)

# Create wscpp_net library
add_library(wscpp_net STATIC ${WSCPP_NET_SOURCES})

target_include_directories(wscpp_net
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${ASIO_INCLUDE_DIRS}
)

target_link_libraries(wscpp_net
    PUBLIC
        asio::asio
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

# Set target properties
set_target_properties(wscpp_net PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
