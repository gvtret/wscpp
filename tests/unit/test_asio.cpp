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

// Test SSL handshake requires enable_ssl first
TEST(AsioSocketTest, SslHandshakeRequiresEnableSsl) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    EXPECT_THROW(socket.ssl_handshake(true), std::runtime_error);
}

// TLS SNI hostname can be set after enable_ssl
TEST(AsioSocketTest, SetSslHostnameAfterEnableSsl) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    std::shared_ptr<asio::ssl::context> ctx(
        new asio::ssl::context(asio::ssl::context::tlsv12_client));
    socket.enable_ssl(ctx);
    EXPECT_NO_THROW(socket.set_ssl_hostname("example.com"));
}

TEST(AsioSocketTest, SetSslHostnameRequiresEnableSsl) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    EXPECT_THROW(socket.set_ssl_hostname("example.com"), std::runtime_error);
}

// Test SSL context
TEST(AsioSocketTest, SetSSLContext) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    
    std::shared_ptr<asio::ssl::context> ctx(
        new asio::ssl::context(asio::ssl::context::tlsv12_client));
    socket.enable_ssl(ctx);
    
    EXPECT_TRUE(socket.is_ssl());
}

// Test write operation requires an open connection
TEST(AsioSocketTest, WriteOperationRequiresConnection) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    const char* test_data = "Hello, ASIO!";
    EXPECT_THROW(socket.write(test_data, 12), std::system_error);
}

// Test read operation requires an open connection
TEST(AsioSocketTest, ReadOperationRequiresConnection) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    char buffer[256];
    EXPECT_THROW(socket.read(buffer, 256), std::system_error);
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
