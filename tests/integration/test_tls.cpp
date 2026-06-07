#include <gtest/gtest.h>
#include <wscpp/client.hpp>
#include <wscpp/server.hpp>
#include <tls_fixtures.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace wscpp;
using wscpp::test::tls::fixture_paths;
using wscpp::test::tls::make_client_ssl_context;
using wscpp::test::tls::make_server_ssl_context;

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

TEST(TlsIntegration, WssEchoWithServerCertificate) {
    WSCPP_REQUIRE_TLS_FIXTURES();
    const test::tls::paths paths = fixture_paths();

    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);
    std::string received;

    server srv;
    const std::shared_ptr<server::ssl_context> ssl_ctx =
        make_server_ssl_context(paths, false);
    ASSERT_TRUE(ssl_ctx);
    srv.set_ssl_context(ssl_ctx);
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode) {
            conn->send_text(std::string(data.begin(), data.end()));
        });
    });
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());

    std::thread t([&]() {
        client cli;
        cli.set_on_open([&]() { cli.send_text("wss-ping"); });
        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode) {
            received = std::string(data.begin(), data.end());
            cli.close();
            done = true;
        });
        const std::error_code ec =
            cli.connect("wss://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            ADD_FAILURE() << ec.message();
            done = true;
        } else {
            wait_for(done, 5000);
        }
    });

    ASSERT_TRUE(wait_for(done, 5000));
    t.join();
    srv.stop();
    EXPECT_EQ(received, "wss-ping");
}

TEST(TlsIntegration, WssEchoWithCaVerification) {
    WSCPP_REQUIRE_TLS_FIXTURES();
    const test::tls::paths paths = fixture_paths();

    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);
    std::string received;

    server srv;
    const std::shared_ptr<server::ssl_context> ssl_ctx =
        make_server_ssl_context(paths, false);
    ASSERT_TRUE(ssl_ctx);
    srv.set_ssl_context(ssl_ctx);
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode) {
            conn->send_text(std::string(data.begin(), data.end()));
        });
    });
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());

    std::thread t([&]() {
        client cli;
        const std::shared_ptr<client::ssl_context> client_ctx =
            make_client_ssl_context(paths, true, false);
        if (!client_ctx) {
            ADD_FAILURE() << "client SSL context";
            done = true;
            return;
        }
        cli.set_ssl_context(client_ctx);
        cli.set_on_open([&]() { cli.send_text("verified"); });
        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode) {
            received = std::string(data.begin(), data.end());
            cli.close();
            done = true;
        });
        const std::error_code ec =
            cli.connect("wss://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            ADD_FAILURE() << ec.message();
            done = true;
        } else {
            wait_for(done, 5000);
        }
    });

    ASSERT_TRUE(wait_for(done, 5000));
    t.join();
    srv.stop();
    EXPECT_EQ(received, "verified");
}

TEST(TlsIntegration, WssMutualTlsEcho) {
    WSCPP_REQUIRE_TLS_FIXTURES();
    const test::tls::paths paths = fixture_paths();

    const uint16_t port = pick_free_port();
    std::atomic<bool> done(false);
    std::string received;

    server srv;
    const std::shared_ptr<server::ssl_context> server_ctx =
        make_server_ssl_context(paths, true);
    ASSERT_TRUE(server_ctx);
    srv.set_ssl_context(server_ctx);
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode) {
            conn->send_text(std::string(data.begin(), data.end()));
        });
    });
    ASSERT_FALSE(srv.listen(port));
    ASSERT_FALSE(srv.start());

    std::thread t([&]() {
        client cli;
        const std::shared_ptr<client::ssl_context> client_ctx =
            make_client_ssl_context(paths, true, true);
        if (!client_ctx) {
            ADD_FAILURE() << "client SSL context";
            done = true;
            return;
        }
        cli.set_ssl_context(client_ctx);
        cli.set_on_open([&]() { cli.send_text("mtls"); });
        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode) {
            received = std::string(data.begin(), data.end());
            cli.close();
            done = true;
        });
        const std::error_code ec =
            cli.connect("wss://127.0.0.1:" + std::to_string(port) + "/");
        if (ec) {
            ADD_FAILURE() << ec.message();
            done = true;
        } else {
            wait_for(done, 5000);
        }
    });

    ASSERT_TRUE(wait_for(done, 5000));
    t.join();
    srv.stop();
    EXPECT_EQ(received, "mtls");
}
