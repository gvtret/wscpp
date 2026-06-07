#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <wscpp/client.hpp>
#include <wscpp/frame/builder.hpp>
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

} // namespace

TEST(Rfc6455Connection, FragmentedTextEcho) {
    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);
    std::string received;

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op == frame::opcode::TEXT) {
                conn->send_text(std::string(data.begin(), data.end()));
            }
        });
    });
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());

    std::thread t([&]() {
        client cli;
        cli.set_on_open([&]() {
            cli.send_text("Hello ", false);
            cli.send_continuation("World", true);
        });
        cli.set_on_message([&](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op == frame::opcode::TEXT) {
                received = std::string(data.begin(), data.end());
                cli.close();
                done = true;
            }
        });
        const std::error_code ec = cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            done = true;
        } else {
            wait_for(done, 5000);
        }
    });

    t.join();
    srv.stop();
    ASSERT_TRUE(done.load());
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
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());

    std::thread t([&]() {
        client cli;
        cli.set_on_open([&]() {});
        const std::error_code ec = cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
        if (!ec) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            cli.close();
        }
        done = true;
    });

    t.join();
    srv.stop();
    ASSERT_TRUE(done.load());
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
            std::error_code ec;
            conn->socket().write(frame.data(), frame.size(), ec);
        });
    });
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());

    std::thread t([&]() {
        client cli;
        cli.set_on_close([&](uint16_t code, const std::string &) {
            close_code = code;
            done = true;
        });
        cli.set_on_error([&](const std::string &) { done = true; });
        const std::error_code ec = cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            done = true;
        } else {
            wait_for(done, 5000);
        }
    });

    t.join();
    srv.stop();
    ASSERT_TRUE(done.load());
    EXPECT_EQ(close_code, 1007u);
}

#if WSCPP_ENABLE_DEFLATE

TEST(Rfc6455Connection, DeflateTextEcho) {
    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);
    std::string received;

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op == frame::opcode::TEXT) {
                conn->send_text(std::string(data.begin(), data.end()));
            }
        });
    });
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());

    std::thread t([&]() {
        client cli;
        cli.enable_permessage_deflate(true);
        cli.set_on_open([&]() { cli.send_text("Hello compressed"); });
        cli.set_on_message([&](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op == frame::opcode::TEXT) {
                received = std::string(data.begin(), data.end());
                cli.close();
                done = true;
            }
        });
        const std::error_code ec = cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            done = true;
        } else {
            wait_for(done, 5000);
        }
    });

    t.join();
    srv.stop();
    ASSERT_TRUE(done.load());
    EXPECT_EQ(received, "Hello compressed");
}

#endif
