// bench_sws_roundtrip.cpp — echo latency comparison (Simple-WebSocket-Server)

#include "../bench_util.hpp"
#include "client_ws.hpp"
#include "server_ws.hpp"
#include <atomic>
#include <cstdio>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

namespace {

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using Connection = WsServer::Connection;
using InMessage = WsServer::InMessage;

void configure_echo_server(WsServer& server) {
    server.config.port = 0;
    server.config.address = "127.0.0.1";
    server.config.reuse_address = true;
    server.config.thread_pool_size = 1;

    auto& echo = server.endpoint["^/echo/?$"];
    echo.on_message = [](std::shared_ptr<Connection> connection, std::shared_ptr<InMessage> message) {
        connection->send(message->string());
    };
}

} // namespace

int main() {
    print_compare_banner("bench_sws_roundtrip");
    const int samples = 100;

    WsServer server;
    configure_echo_server(server);

    std::promise<uint16_t> listen_port;
    std::thread server_thread([&]() {
        server.start([&listen_port](unsigned short bound_port) {
            listen_port.set_value(bound_port);
        });
    });

    const uint16_t actual_port = listen_port.get_future().get();

    std::vector<double> latencies_ms;
    latencies_ms.reserve(static_cast<std::size_t>(samples));
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(samples));
    std::atomic<bool> done(false);
    std::atomic<int> next_send(0);

    WsClient client("127.0.0.1:" + std::to_string(actual_port) + "/echo");

    client.on_open = [&send_times, &next_send](std::shared_ptr<WsClient::Connection> connection) {
        send_times[0] = clock::now();
        connection->send("ping-0");
        next_send = 1;
    };

    client.on_message = [&](std::shared_ptr<WsClient::Connection> connection,
                            std::shared_ptr<WsClient::InMessage> message) {
        (void)message;
        const int n = next_send.load() - 1;
        if (n >= 0 && n < samples) {
            const double ms =
                elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
            latencies_ms.push_back(ms);
        }
        const int send_idx = next_send.load();
        if (send_idx < samples) {
            send_times[static_cast<std::size_t>(send_idx)] = clock::now();
            connection->send("ping-" + std::to_string(send_idx));
            next_send = send_idx + 1;
        } else {
            connection->send_close(1000);
        }
    };

    client.on_error = [&done](std::shared_ptr<WsClient::Connection>, const SimpleWeb::error_code& ec) {
        std::fprintf(stderr, "sws client error: %s\n", ec.message().c_str());
        done = true;
    };

    client.on_close = [&done](std::shared_ptr<WsClient::Connection>, int, const std::string&) {
        done = true;
    };

    std::thread client_thread([&]() { client.start(); });

    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    client.stop();
    if (client_thread.joinable()) {
        client_thread.join();
    }

    server.stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }

    std::printf("simple-websocket-server roundtrip (%zu samples, port %u)\n",
                latencies_ms.size(),
                actual_port);
    print_latency("echo_latency", latencies_ms);

    return latencies_ms.size() == static_cast<std::size_t>(samples) ? 0 : 1;
}
