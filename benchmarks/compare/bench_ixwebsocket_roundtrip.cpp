// bench_ixwebsocket_roundtrip.cpp — echo latency comparison (IXWebSocket 11.4.x)

#include "../bench_common.hpp"
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

namespace {

uint16_t pick_free_port() {
    asio::io_context io;
    asio::ip::tcp::acceptor acc(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    return acc.local_endpoint().port();
}

bool start_echo_server(ix::WebSocketServer& server) {
    server.setOnClientMessageCallback(
        [](std::shared_ptr<ix::ConnectionState>,
           ix::WebSocket& webSocket,
           const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Message) {
                webSocket.send(msg->str, msg->binary);
            }
        });
    return server.listenAndStart();
}

} // namespace

int main() {
    ix::initNetSystem();

    const int samples = 100;
    const uint16_t port = pick_free_port();

    ix::WebSocketServer server(static_cast<int>(port), "127.0.0.1");
    if (!start_echo_server(server)) {
        std::fprintf(stderr, "ixwebsocket server failed to start on port %u\n", port);
        return 1;
    }

    std::vector<double> latencies_ms(static_cast<std::size_t>(samples), 0.0);
    std::atomic<int> latency_count(0);
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(samples));
    std::atomic<bool> done(false);
    std::atomic<int> next_send(0);

    ix::WebSocket client;
    client.setUrl("ws://127.0.0.1:" + std::to_string(port) + "/");
    client.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            send_times[0] = clock::now();
            client.send("ping-0");
            next_send = 1;
        } else if (msg->type == ix::WebSocketMessageType::Message) {
            const int n = next_send.load() - 1;
            if (n >= 0 && n < samples) {
                const double ms =
                    elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
                latencies_ms[static_cast<std::size_t>(n)] = ms;
                latency_count.fetch_add(1);
            }
            const int send_idx = next_send.load();
            if (send_idx < samples) {
                send_times[static_cast<std::size_t>(send_idx)] = clock::now();
                client.send("ping-" + std::to_string(send_idx));
                next_send = send_idx + 1;
            } else {
                done = true;
            }
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            std::fprintf(stderr, "ixwebsocket client error: %s\n",
                         msg->errorInfo.reason.c_str());
            done = true;
        }
    });

    client.start();
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    client.stop();
    server.stop();

    std::vector<double> latency_report(latencies_ms.begin(),
                                       latencies_ms.begin() + latency_count.load());
    std::printf("ixwebsocket roundtrip (%zu samples, port %u)\n", latency_report.size(), port);
    print_latency("echo_latency", latency_report);

    const std::size_t throughput_bytes = 64 * 1024;
    const std::string blob(throughput_bytes, static_cast<char>(0xAB));
    const uint16_t tp_port = pick_free_port();

    ix::WebSocketServer tp_server(static_cast<int>(tp_port), "127.0.0.1");
    if (!start_echo_server(tp_server)) {
        ix::uninitNetSystem();
        return 1;
    }

    std::atomic<bool> tp_done(false);
    std::atomic<int> tp_sent(0);
    int tp_iterations = 0;

    ix::WebSocket tp_client;
    tp_client.setUrl("ws://127.0.0.1:" + std::to_string(tp_port) + "/");
    tp_client.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            tp_client.sendBinary(blob);
        } else if (msg->type == ix::WebSocketMessageType::Message) {
            if (msg->str.size() != throughput_bytes) {
                return;
            }
            const int sent = tp_sent.fetch_add(1) + 1;
            if (sent < 100) {
                tp_client.sendBinary(blob);
            } else {
                tp_iterations = sent;
                tp_done = true;
            }
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            tp_done = true;
        }
    });

    const clock::time_point tp_start = clock::now();
    tp_client.start();
    while (!tp_done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    tp_client.stop();
    const clock::time_point tp_end = clock::now();
    tp_server.stop();

    print_throughput("echo_throughput_64k",
                     static_cast<double>(throughput_bytes * tp_iterations),
                     elapsed_sec(tp_start, tp_end), tp_iterations);

    ix::uninitNetSystem();
    return 0;
}
