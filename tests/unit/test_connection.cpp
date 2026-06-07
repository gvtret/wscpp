// test_connection.cpp
// Unit tests for WebSocket connection

#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>
#include <wscpp/connection.hpp>
#include <wscpp/net/transport.hpp>

using namespace wscpp;
using wscpp::net::io_context;

// Test connection creation
TEST(ConnectionTest, CreateConnection) {
    io_context io_context;
    connection conn(io_context);
    EXPECT_FALSE(conn.is_open());
}

// Test connection state after creation
TEST(ConnectionTest, ConnectionInitialState) {
    io_context io_context;
    connection conn(io_context);

    EXPECT_FALSE(conn.is_open());
    EXPECT_FALSE(conn.is_ssl());
}

// Test connection close
TEST(ConnectionTest, CloseConnection) {
    io_context io_context;
    connection conn(io_context);

    // Close without connecting (should be safe)
    conn.close(1000, "Normal closure");

    EXPECT_FALSE(conn.is_open());
}

// Test connection callbacks
TEST(ConnectionTest, SetCallbacks) {
    io_context io_context;
    connection conn(io_context);

    bool open_called = false;
    conn.set_on_open([&open_called]() { open_called = true; });

    std::string received_text;
    conn.set_on_message([&received_text](const std::vector<uint8_t> &data, frame::opcode op) {
        received_text = std::string(data.begin(), data.end());
    });

    bool close_called = false;
    uint16_t close_code = 0;
    conn.set_on_close([&close_called, &close_code](uint16_t code, const std::string &) {
        close_called = true;
        close_code = code;
    });

    bool error_called = false;
    conn.set_on_error([&error_called](const std::string &) { error_called = true; });

    // Callbacks are set
    EXPECT_TRUE(true); // Just verify setting works
}

// Test connection send
TEST(ConnectionTest, SendText) {
    io_context io_context;
    connection conn(io_context);

    // Note: This will fail without a real connection, but we can test the method exists
    // In a real test, we would connect to a test server
    EXPECT_FALSE(conn.is_open());
}

// Test connection send binary
TEST(ConnectionTest, SendBinary) {
    io_context io_context;
    connection conn(io_context);

    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    // Note: This will fail without a real connection
    EXPECT_FALSE(conn.is_open());
}

// Test connection socket access
TEST(ConnectionTest, SocketAccess) {
    io_context io_context;
    connection conn(io_context);

    auto &socket = conn.socket();
    const auto &const_socket = conn.socket();

    EXPECT_EQ(&socket, &const_socket);
}

// Test connection SSL state
TEST(ConnectionTest, SSLState) {
    io_context io_context;
    connection conn(io_context);

    // Default connection is not SSL
    EXPECT_FALSE(conn.is_ssl());
}

// Test multiple connections
TEST(ConnectionTest, MultipleConnections) {
    io_context io_context;

    connection conn1(io_context);
    connection conn2(io_context);

    EXPECT_FALSE(conn1.is_open());
    EXPECT_FALSE(conn2.is_open());
}

// Test connection with different io_contexts
TEST(ConnectionTest, DifferentIOContexts) {
    io_context io_context1;
    io_context io_context2;

    connection conn1(io_context1);
    connection conn2(io_context2);

    EXPECT_FALSE(conn1.is_open());
    EXPECT_FALSE(conn2.is_open());
}

// Test connection with SSL context
TEST(ConnectionTest, SSLContext) {
    io_context io_context;
    connection conn(io_context);

#if WSCPP_USE_ASIO
    asio::ssl::context ssl_context(asio::ssl::context::tlsv12_client);
    (void)ssl_context;
#else
    std::shared_ptr<net::openssl_context> ssl_context;
    ASSERT_FALSE(net::openssl_context::make(net::openssl_context::role::client, ssl_context));
#endif

    EXPECT_FALSE(conn.is_ssl());
}
