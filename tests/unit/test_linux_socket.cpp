// test_linux_socket.cpp — unit tests for POSIX socket transport

#include <gtest/gtest.h>
#include <wscpp/net/linux_socket.hpp>
#include <wscpp/net/openssl_context.hpp>
#include <wscpp/error.hpp>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <chrono>
#include <future>
#include <thread>
#include <unistd.h>

namespace {

uint16_t pick_free_port() {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return 0;
    }
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return 0;
    }
    socklen_t len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        ::close(fd);
        return 0;
    }
    ::close(fd);
    return ntohs(addr.sin_port);
}

} // namespace

TEST(LinuxSocketTest, CreateSocket) {
    wscpp::net::io_context io;
    wscpp::net::linux_socket socket(io);
    EXPECT_FALSE(socket.is_open());
}

TEST(LinuxSocketTest, CloseSocket) {
    wscpp::net::io_context io;
    wscpp::net::linux_socket socket(io);
    socket.close();
    EXPECT_FALSE(socket.is_open());
}

TEST(LinuxSocketTest, SslHandshakeRequiresEnableSsl) {
    wscpp::net::io_context io;
    wscpp::net::linux_socket socket(io);
    EXPECT_EQ(socket.ssl_handshake(true),
              wscpp::make_error_code(wscpp::errc::ssl_not_enabled));
}

TEST(LinuxSocketTest, SetSslHostnameRequiresEnableSsl) {
    wscpp::net::io_context io;
    wscpp::net::linux_socket socket(io);
    EXPECT_EQ(socket.set_ssl_hostname("example.com"),
              wscpp::make_error_code(wscpp::errc::ssl_not_enabled));
}

TEST(LinuxSocketTest, TcpEchoRoundtrip) {
    const uint16_t port = pick_free_port();
    ASSERT_NE(port, 0u);

    std::promise<void> server_ready;
    std::future<void> ready = server_ready.get_future();

    std::thread server_thread([&, port]() {
        const int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
            return;
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            ::close(listen_fd);
            return;
        }
        if (::listen(listen_fd, 1) != 0) {
            ::close(listen_fd);
            return;
        }
        server_ready.set_value();

        const int client_fd = ::accept(listen_fd, nullptr, nullptr);
        if (client_fd < 0) {
            ::close(listen_fd);
            return;
        }

        char buf[32] = {};
        if (::recv(client_fd, buf, sizeof(buf), 0) == 5) {
            ::send(client_fd, buf, 5, MSG_NOSIGNAL);
        }

        ::close(client_fd);
        ::close(listen_fd);
    });

    ready.wait();

    wscpp::net::io_context io;
    wscpp::net::linux_socket socket(io);
    ASSERT_FALSE(socket.connect("127.0.0.1", std::to_string(port)));
    ASSERT_TRUE(socket.is_open());

    const char msg[] = "hello";
    std::error_code ec;
    ASSERT_EQ(socket.write(msg, 5, ec), 5u);
    ASSERT_FALSE(ec);
    char out[32] = {};
    ASSERT_EQ(socket.read(out, 5, ec), 5u);
    ASSERT_FALSE(ec);
    EXPECT_STREQ(out, "hello");

    socket.close();
    server_thread.join();
}

TEST(LinuxSocketTest, OpenSslContextCreation) {
    std::shared_ptr<wscpp::net::openssl_context> ctx;
    ASSERT_FALSE(wscpp::net::openssl_context::make(
        wscpp::net::openssl_context::role::client, ctx));
    EXPECT_NE(ctx->native(), nullptr);
}
