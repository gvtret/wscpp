// bench_ixwebsocket_roundtrip_net.cpp — client-only echo + throughput over LAN

#include "../bench_net_args.hpp"
#include "../bench_util.hpp"
#include <atomic>
#include <cstdio>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

int main(int argc, char *argv[]) {
    net_bench_args args;
    if (!parse_net_client_args(argc, argv, args, "bench_ixwebsocket_roundtrip_net")) {
        return 1;
    }

    print_compare_banner("bench_ixwebsocket_roundtrip_net");
    std::printf("target ws://%s:%u/ samples=%d\n", args.host.c_str(), args.port, args.samples);

    ix::initNetSystem();

    const std::string url = "ws://" + args.host + ":" + std::to_string(args.port) + "/";

    std::vector<double> latencies_ms(static_cast<std::size_t>(args.samples), 0.0);
    std::atomic<int> latency_count(0);
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(args.samples));
    std::atomic<bool> done(false);
    std::atomic<int> next_send(0);

    ix::WebSocket client;
    client.setUrl(url);
    client.setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            send_times[0] = clock::now();
            client.send("ping-0");
            next_send = 1;
        } else if (msg->type == ix::WebSocketMessageType::Message) {
            const int n = next_send.load() - 1;
            if (n >= 0 && n < args.samples) {
                const double ms =
                    elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
                latencies_ms[static_cast<std::size_t>(n)] = ms;
                latency_count.fetch_add(1);
            }
            const int send_idx = next_send.load();
            if (send_idx < args.samples) {
                send_times[static_cast<std::size_t>(send_idx)] = clock::now();
                client.send("ping-" + std::to_string(send_idx));
                next_send = send_idx + 1;
            } else {
                done = true;
            }
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            std::fprintf(stderr, "ixwebsocket client error: %s\n", msg->errorInfo.reason.c_str());
            done = true;
        }
    });

    client.start();
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    client.stop();

    std::vector<double> latency_report(latencies_ms.begin(),
                                       latencies_ms.begin() + latency_count.load());
    std::printf("ixwebsocket net roundtrip (%zu samples)\n", latency_report.size());
    print_latency("echo_latency", latency_report);

    const std::size_t throughput_bytes = 64 * 1024;
    const std::string blob(throughput_bytes, static_cast<char>(0xAB));
    std::atomic<bool> tp_done(false);
    std::atomic<int> tp_sent(0);
    int tp_iterations = 0;

    ix::WebSocket tp_client;
    tp_client.setUrl(url);
    tp_client.setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
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

    print_throughput("echo_throughput_64k", static_cast<double>(throughput_bytes * tp_iterations),
                     elapsed_sec(tp_start, tp_end), tp_iterations);

    ix::uninitNetSystem();
    return latency_report.size() == static_cast<std::size_t>(args.samples) ? 0 : 1;
}
