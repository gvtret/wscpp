#include <gtest/gtest.h>
#include <wscpp/server.hpp>
#include <wscpp/client.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>

using namespace wscpp;

namespace {

uint16_t pick_free_port() {
    asio::io_context io;
    asio::ip::tcp::acceptor acc(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    return acc.local_endpoint().port();
}

bool wait_for(std::atomic<bool>& flag, int timeout_ms) {
    for (int elapsed = 0; elapsed < timeout_ms; elapsed += 10) {
        if (flag.load()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return flag.load();
}

} // namespace

TEST(ClientServerIntegration, EchoTextMessage) {
    const uint16_t port = pick_free_port();
    std::atomic<bool> client_done(false);
    std::string received;

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode) {
            conn->send_text(std::string(data.begin(), data.end()));
        });
    });
    srv.listen(port);
    srv.start();

    std::thread client_thread([&]() {
        client cli;
        cli.set_on_open([&]() { cli.send_text("ping"); });
        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode) {
            received = std::string(data.begin(), data.end());
            cli.close();
            client_done = true;
        });
        try {
            cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
        } catch (const std::exception& ex) {
            ADD_FAILURE() << ex.what();
            client_done = true;
            return;
        }
        wait_for(client_done, 5000);
    });

    ASSERT_TRUE(wait_for(client_done, 5000)) << "integration test timed out";
    client_thread.join();
    srv.stop();

    EXPECT_EQ(received, "ping");
}
