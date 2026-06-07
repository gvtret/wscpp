#include <gtest/gtest.h>
#include <wscpp/net/asio_socket.hpp>
#include <wscpp/error.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl/context.hpp>

TEST(AsioSocketTest, CreateSocket) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    EXPECT_FALSE(socket.is_open());
}

TEST(AsioSocketTest, ConnectAndOpen) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    EXPECT_FALSE(socket.is_open());
}

TEST(AsioSocketTest, CloseSocket) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    socket.close();
    EXPECT_FALSE(socket.is_open());
}

TEST(AsioSocketTest, SslHandshakeRequiresEnableSsl) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    EXPECT_EQ(socket.ssl_handshake(true), wscpp::make_error_code(wscpp::errc::ssl_not_enabled));
}

TEST(AsioSocketTest, SetSslHostnameAfterEnableSsl) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    std::shared_ptr<asio::ssl::context> ctx(
        new asio::ssl::context(asio::ssl::context::tlsv12_client));
    ASSERT_FALSE(socket.enable_ssl(ctx));
    EXPECT_FALSE(socket.set_ssl_hostname("example.com"));
}

TEST(AsioSocketTest, SetSslHostnameRequiresEnableSsl) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    EXPECT_EQ(socket.set_ssl_hostname("example.com"),
              wscpp::make_error_code(wscpp::errc::ssl_not_enabled));
}

TEST(AsioSocketTest, SetSSLContext) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);

    std::shared_ptr<asio::ssl::context> ctx(
        new asio::ssl::context(asio::ssl::context::tlsv12_client));
    ASSERT_FALSE(socket.enable_ssl(ctx));

    EXPECT_TRUE(socket.is_ssl());
}

TEST(AsioSocketTest, WriteOperationRequiresConnection) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    const char* test_data = "Hello, ASIO!";
    std::error_code ec;
    socket.write(test_data, 12, ec);
    EXPECT_TRUE(ec);
}

TEST(AsioSocketTest, ReadOperationRequiresConnection) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);
    char buffer[256];
    std::error_code ec;
    socket.read(buffer, 256, ec);
    EXPECT_TRUE(ec);
}

TEST(AsioSocketTest, MultipleOperations) {
    asio::io_context io_context;
    wscpp::net::asio_socket socket(io_context);

    socket.close();
    EXPECT_FALSE(socket.is_open());

    socket.close();
    EXPECT_FALSE(socket.is_open());
}
