// bench_sws_roundtrip_net.cpp — client-only echo latency over LAN

#include "../bench_net_args.hpp"
#include "../bench_util.hpp"
#include "client_ws.hpp"
#include <atomic>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

namespace {

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

} // namespace

int main(int argc, char *argv[]) {
    net_bench_args args;
    if (!parse_net_client_args(argc, argv, args, "bench_sws_roundtrip_net")) {
        return 1;
    }

    print_compare_banner("bench_sws_roundtrip_net");
    std::printf("target ws://%s:%u/echo samples=%d\n", args.host.c_str(), args.port, args.samples);

    std::vector<double> latencies_ms;
    latencies_ms.reserve(static_cast<std::size_t>(args.samples));
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(args.samples));
    std::atomic<bool> done(false);
    std::atomic<int> next_send(0);

    WsClient client(args.host + ":" + std::to_string(args.port) + "/echo");

    client.on_open = [&send_times, &next_send](std::shared_ptr<WsClient::Connection> connection) {
        send_times[0] = clock::now();
        connection->send("ping-0");
        next_send = 1;
    };

    client.on_message = [&](std::shared_ptr<WsClient::Connection> connection,
                            std::shared_ptr<WsClient::InMessage>) {
        const int n = next_send.load() - 1;
        if (n >= 0 && n < args.samples) {
            const double ms =
                elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
            latencies_ms.push_back(ms);
        }
        const int send_idx = next_send.load();
        if (send_idx < args.samples) {
            send_times[static_cast<std::size_t>(send_idx)] = clock::now();
            connection->send("ping-" + std::to_string(send_idx));
            next_send = send_idx + 1;
        } else {
            connection->send_close(1000);
        }
    };

    client.on_error = [&done](std::shared_ptr<WsClient::Connection>,
                              const SimpleWeb::error_code &ec) {
        std::fprintf(stderr, "sws client error: %s\n", ec.message().c_str());
        done = true;
    };

    client.on_close = [&done](std::shared_ptr<WsClient::Connection>, int, const std::string &) {
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

    std::printf("simple-websocket-server net roundtrip (%zu samples)\n", latencies_ms.size());
    print_latency("echo_latency", latencies_ms);

    return latencies_ms.size() == static_cast<std::size_t>(args.samples) ? 0 : 1;
}
