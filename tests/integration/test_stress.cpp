#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>
#include <wscpp/client.hpp>
#include <wscpp/server.hpp>

using namespace wscpp;

namespace {

uint16_t pick_free_port() {
    asio::io_context io;
    asio::ip::tcp::acceptor acc(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    return acc.local_endpoint().port();
}

bool wait_for(std::atomic<bool> &flag, int timeout_ms) {
    for (int elapsed = 0; elapsed < timeout_ms; elapsed += 10) {
        if (flag.load()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return flag.load();
}

void start_echo_server(server &srv, uint16_t port) {
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op == frame::opcode::TEXT) {
                conn->send_text(std::string(data.begin(), data.end()));
            } else if (op == frame::opcode::BINARY) {
                conn->send_binary(data.data(), data.size());
            }
        });
    });
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());
}

} // namespace

TEST(StressIntegration, ParallelClientsEcho) {
    const uint16_t port = pick_free_port();
    server srv;
    start_echo_server(srv, port);

    const int client_count = 10;
    std::atomic<int> completed(0);

    std::vector<std::thread> threads;
    for (int i = 0; i < client_count; ++i) {
        threads.emplace_back([&, i]() {
            client cli;
            const std::string msg = "client-" + std::to_string(i);
            std::atomic<bool> done(false);
            cli.set_on_open([&]() { cli.send_text(msg); });
            cli.set_on_message([&](const std::vector<uint8_t> &data, frame::opcode) {
                if (std::string(data.begin(), data.end()) == msg) {
                    cli.close();
                    done = true;
                }
            });
            const std::error_code ec = cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
            if (ec) {
                done = true;
            }
            if (wait_for(done, 10000)) {
                completed.fetch_add(1);
            }
        });
    }

    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    srv.stop();

    EXPECT_EQ(completed.load(), client_count);
}

TEST(StressIntegration, LargeBinaryMessage) {
    const uint16_t port = pick_free_port();
    server srv;
    start_echo_server(srv, port);

    const std::size_t payload_size = 1024 * 1024;
    std::vector<uint8_t> payload(payload_size);
    for (std::size_t i = 0; i < payload_size; ++i) {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }

    std::atomic<bool> done(false);
    std::thread client_thread([&]() {
        client cli;
        cli.set_on_open([&]() { cli.send_binary(payload.data(), payload.size()); });
        cli.set_on_message([&](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op == frame::opcode::BINARY && data.size() == payload_size) {
                cli.close();
                done = true;
            }
        });
        const std::error_code ec = cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            done = true;
        }
        wait_for(done, 30000);
    });

    ASSERT_TRUE(wait_for(done, 30000));
    client_thread.join();
    srv.stop();
}

TEST(StressIntegration, RapidSmallMessages) {
    const uint16_t port = pick_free_port();
    server srv;
    start_echo_server(srv, port);

    const int message_count = 100;
    std::atomic<bool> done(false);
    std::atomic<int> received(0);

    std::thread client_thread([&]() {
        client cli;
        cli.set_on_open([&]() { cli.send_text("start"); });
        cli.set_on_message([&](const std::vector<uint8_t> &data, frame::opcode) {
            const int count = received.fetch_add(1) + 1;
            if (count < message_count) {
                cli.send_text("msg-" + std::to_string(count));
            } else {
                cli.close();
                done = true;
            }
        });
        const std::error_code ec = cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            done = true;
        }
        wait_for(done, 15000);
    });

    ASSERT_TRUE(wait_for(done, 15000));
    client_thread.join();
    srv.stop();
    EXPECT_GE(received.load(), message_count);
}
