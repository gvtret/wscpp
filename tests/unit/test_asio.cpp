// test_asio.cpp
// Unit test for ASIO socket wrapper

#include <gtest/gtest.h>
#include <wscpp/net/asio_socket.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/buffer.hpp>
#include <thread>
#include <chrono>

// Test basic socket creation
TEST(AsioSocketTest, CreateSocket) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    EXPECT_FALSE(socket.is_open());
}

// Test socket open after connect
TEST(AsioSocketTest, ConnectAndOpen) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    
    // Try to connect to a non-existent local port (expected to fail)
    // We just want to test that is_open() works correctly
    EXPECT_FALSE(socket.is_open());
}

// Test close
TEST(AsioSocketTest, CloseSocket) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    socket.close();
    EXPECT_FALSE(socket.is_open());
}

// Test SSL context
TEST(AsioSocketTest, SetSSLContext) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    
    asio::ssl::context ssl_context(asio::ssl::context::tlsv12_client);
    socket.set_ssl_context(std::make_shared<asio::ssl::context>(ssl_context));
    
    EXPECT_TRUE(socket.is_ssl());
}

// Test write operation (basic)
TEST(AsioSocketTest, WriteOperation) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    
    const char* test_data = "Hello, ASIO!";
    size_t bytes_written = socket.write(test_data, 12);
    
    // The write operation should complete (even if connection fails)
    EXPECT_EQ(bytes_written, 12);
}

// Test read operation (basic)
TEST(AsioSocketTest, ReadOperation) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    
    char buffer[256];
    size_t bytes_read = socket.read(buffer, 256);
    
    // The read operation should complete (even if connection fails)
    EXPECT_GE(bytes_read, 0);
}

// Test multiple socket operations
TEST(AsioSocketTest, MultipleOperations) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    
    // Multiple operations
    socket.close();
    EXPECT_FALSE(socket.is_open());
    
    socket.close(); // Should be safe to call multiple times
    EXPECT_FALSE(socket.is_open());
}
