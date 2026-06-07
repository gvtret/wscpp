#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <gtest/gtest.h>
#include <wscpp/error.hpp>
#include <wscpp/server.hpp>

using namespace wscpp;

TEST(ServerTest, InitialState) {
    server srv;
    EXPECT_FALSE(srv.is_running());
}

TEST(ServerTest, StopWithoutListenIsSafe) {
    server srv;
    srv.stop();
    EXPECT_FALSE(srv.is_running());
}

TEST(ServerTest, ListenBindsPort) {
    asio::io_context io;
    asio::ip::tcp::acceptor probe(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    const uint16_t port = probe.local_endpoint().port();
    probe.close();

    server srv;
    EXPECT_FALSE(srv.listen(port));
    EXPECT_TRUE(srv.is_running());
    srv.stop();
    EXPECT_FALSE(srv.is_running());
}

TEST(ServerTest, StartRequiresListen) {
    server srv;
    EXPECT_EQ(srv.start(), make_error_code(errc::server_not_listening));
}
