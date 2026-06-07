#include <gtest/gtest.h>
#include <wscpp/server.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

using namespace wscpp;

TEST(ServerTest, InitialState) {
    server srv;
    EXPECT_FALSE(srv.is_running());
}

TEST(ServerTest, StopWithoutListenIsSafe) {
    server srv;
    EXPECT_NO_THROW(srv.stop());
    EXPECT_FALSE(srv.is_running());
}

TEST(ServerTest, ListenBindsPort) {
    asio::io_context io;
    asio::ip::tcp::acceptor probe(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    const uint16_t port = probe.local_endpoint().port();
    probe.close();

    server srv;
    EXPECT_NO_THROW(srv.listen(port));
    EXPECT_TRUE(srv.is_running());
    srv.stop();
    EXPECT_FALSE(srv.is_running());
}

TEST(ServerTest, StartRequiresListen) {
    server srv;
    EXPECT_THROW(srv.start(), std::runtime_error);
}
