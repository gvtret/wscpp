// bench_roundtrip_net.cpp — WebSocket echo client against a remote host

#include "bench_util.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <wscpp/client.hpp>

using namespace wscpp;
using namespace wscpp::bench;

namespace {

void usage(const char *argv0) {
    std::fprintf(stderr, "Usage: %s <host> [port] [samples]\n", argv0);
    std::fprintf(stderr, "  Defaults: port=19081 samples=100\n");
}

} // namespace

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const std::string host = argv[1];
    const uint16_t port =
        argc > 2 ? static_cast<uint16_t>(std::atoi(argv[2])) : static_cast<uint16_t>(19081);
    const int samples = argc > 3 ? std::atoi(argv[3]) : 100;

    print_banner("bench_roundtrip_net");
    std::printf("target ws://%s:%u/ samples=%d\n", host.c_str(), port, samples);

    const std::string url = "ws://" + host + ":" + std::to_string(port) + "/";

    std::vector<double> latencies_ms;
    latencies_ms.reserve(static_cast<std::size_t>(samples));
    std::mutex latency_mutex;
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(samples));
    std::atomic<bool> done(false);
    std::atomic<bool> connect_failed(false);

    std::thread client_thread([&]() {
        client cli;
        int next_send = 0;

        cli.set_on_open([&]() {
            send_times[0] = clock::now();
            cli.send_text("ping-0");
            next_send = 1;
        });

        cli.set_on_message([&](const std::vector<uint8_t> &, frame::opcode) {
            const int n = next_send - 1;
            if (n >= 0 && n < samples) {
                const double ms =
                    elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
                {
                    std::lock_guard<std::mutex> lock(latency_mutex);
                    latencies_ms.push_back(ms);
                }
            }
            if (next_send < samples) {
                send_times[static_cast<std::size_t>(next_send)] = clock::now();
                cli.send_text("ping-" + std::to_string(next_send));
                ++next_send;
            } else {
                cli.close();
                done = true;
            }
        });

        const std::error_code ec = cli.connect(url);
        if (ec) {
            std::fprintf(stderr, "connect failed: %s\n", ec.message().c_str());
            connect_failed = true;
            done = true;
        } else {
            while (!done.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });

    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    client_thread.join();

    if (connect_failed.load()) {
        return 1;
    }

    std::printf("wscpp net roundtrip (%zu samples)\n", latencies_ms.size());
    print_latency("echo_latency", latencies_ms);

    const std::size_t throughput_bytes = 64 * 1024;
    std::vector<uint8_t> blob(throughput_bytes, 0xAB);
    std::atomic<bool> tp_done(false);
    std::atomic<bool> tp_failed(false);
    int tp_iterations = 0;

    std::thread tp_thread([&]() {
        client cli;
        int sent = 0;
        cli.set_on_open([&]() { cli.send_binary(blob.data(), blob.size()); });
        cli.set_on_message([&](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op != frame::opcode::BINARY || data.size() != throughput_bytes) {
                return;
            }
            ++sent;
            if (sent < 100) {
                cli.send_binary(blob.data(), blob.size());
            } else {
                tp_iterations = sent;
                cli.close();
                tp_done = true;
            }
        });
        const std::error_code ec = cli.connect(url);
        if (ec) {
            std::fprintf(stderr, "throughput connect failed: %s\n", ec.message().c_str());
            tp_failed = true;
            tp_done = true;
        } else {
            while (!tp_done.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });

    const clock::time_point tp_start = clock::now();
    while (!tp_done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const clock::time_point tp_end = clock::now();
    tp_thread.join();

    if (tp_failed.load()) {
        return 1;
    }

    print_throughput("echo_throughput_64k", static_cast<double>(throughput_bytes * tp_iterations),
                     elapsed_sec(tp_start, tp_end), tp_iterations);

    return latencies_ms.size() == static_cast<std::size_t>(samples) ? 0 : 1;
}
