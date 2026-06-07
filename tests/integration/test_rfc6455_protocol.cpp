#include <gtest/gtest.h>
#include <wscpp/server.hpp>
#include <wscpp/client.hpp>
#include <wscpp/frame/builder.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

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

TEST(Rfc6455Connection, FragmentedTextEcho) {
    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);
    std::string received;

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode op) {
            if (op == frame::opcode::TEXT) {
                conn->send_text(std::string(data.begin(), data.end()));
            }
        });
    });
    srv.listen(port);
    srv.start();

    std::thread t([&]() {
        client cli;
        cli.set_on_open([&]() {
            cli.send_text("Hello ", false);
            cli.send_continuation("World", true);
        });
        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode op) {
            if (op == frame::opcode::TEXT) {
                received = std::string(data.begin(), data.end());
                cli.close();
                done = true;
            }
        });
        try {
            cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
            wait_for(done, 5000);
        } catch (...) {
            done = true;
        }
    });

    ASSERT_TRUE(wait_for(done, 5000));
    t.join();
    srv.stop();
    EXPECT_EQ(received, "Hello World");
}

TEST(Rfc6455Connection, ServerPingGetsPong) {
    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_open([conn]() {
            const uint8_t body[] = {'p', 'i', 'n', 'g'};
            conn->send_ping(body, 4);
        });
    });
    srv.listen(port);
    srv.start();

    std::thread t([&]() {
        client cli;
        cli.set_on_open([&]() {});
        try {
            cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            cli.close();
        } catch (...) {
        }
        done = true;
    });

    ASSERT_TRUE(wait_for(done, 5000));
    t.join();
    srv.stop();
}

TEST(Rfc6455Connection, InvalidUtf8TextClosesWith1007) {
    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);
    uint16_t close_code = 0;

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_open([conn]() {
            frame::builder b;
            const uint8_t invalid[] = {0xC0, 0x80};
            const std::vector<uint8_t> frame =
                b.build(frame::opcode::TEXT, invalid, 2, true, false);
            conn->socket().write(frame.data(), frame.size());
        });
    });
    srv.listen(port);
    srv.start();

    std::thread t([&]() {
        client cli;
        cli.set_on_close([&](uint16_t code, const std::string&) {
            close_code = code;
            done = true;
        });
        cli.set_on_error([&](const std::string&) { done = true; });
        try {
            cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
            wait_for(done, 5000);
        } catch (...) {
            done = true;
        }
    });

    ASSERT_TRUE(wait_for(done, 5000));
    t.join();
    srv.stop();
    EXPECT_EQ(close_code, 1007u);
}
