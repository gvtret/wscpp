// bench_roundtrip.cpp — localhost WebSocket echo latency (wscpp)

#include "bench_common.hpp"
#include <wscpp/client.hpp>
#include <wscpp/server.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp;
using namespace wscpp::bench;

namespace {

uint16_t pick_free_port() {
    asio::io_context io;
    asio::ip::tcp::acceptor acc(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    return acc.local_endpoint().port();
}

} // namespace

int main() {
    const int samples = 100;
    const uint16_t port = pick_free_port();

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode) {
            conn->send_text(std::string(data.begin(), data.end()));
        });
    });
    srv.listen(port);
    srv.start();

    std::vector<double> latencies_ms;
    latencies_ms.reserve(static_cast<std::size_t>(samples));
    std::mutex latency_mutex;
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(samples));
    std::atomic<bool> done(false);

    std::thread client_thread([&]() {
        client cli;
        int next_send = 0;

        cli.set_on_open([&]() {
            send_times[0] = clock::now();
            cli.send_text("ping-0");
            next_send = 1;
        });

        cli.set_on_message([&](const std::vector<uint8_t>&, frame::opcode) {
            const int n = next_send - 1;
            if (n >= 0 && n < samples) {
                const double ms = elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
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

        try {
            cli.connect("ws://127.0.0.1:" + std::to_string(port) + "/");
            while (!done.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } catch (...) {
            done = true;
        }
    });

    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    client_thread.join();
    srv.stop();

    std::printf("wscpp roundtrip (%zu samples, port %u)\n", latencies_ms.size(), port);
    print_latency("echo_latency", latencies_ms);

    const std::size_t throughput_bytes = 64 * 1024;
    std::vector<uint8_t> blob(throughput_bytes, 0xAB);
    std::atomic<bool> tp_done(false);
    int tp_iterations = 0;

    const uint16_t tp_port = pick_free_port();
    server tp_srv;
    tp_srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode op) {
            if (op == frame::opcode::BINARY) {
                conn->send_binary(data.data(), data.size());
            }
        });
    });
    tp_srv.listen(tp_port);
    tp_srv.start();

    std::thread tp_thread([&]() {
        client cli;
        int sent = 0;
        cli.set_on_open([&]() { cli.send_binary(blob.data(), blob.size()); });
        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode op) {
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
        try {
            cli.connect("ws://127.0.0.1:" + std::to_string(tp_port) + "/");
            while (!tp_done.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } catch (...) {
            tp_done = true;
        }
    });

    const clock::time_point tp_start = clock::now();
    while (!tp_done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const clock::time_point tp_end = clock::now();
    tp_thread.join();
    tp_srv.stop();

    print_throughput("echo_throughput_64k",
                     static_cast<double>(throughput_bytes * tp_iterations),
                     elapsed_sec(tp_start, tp_end), tp_iterations);

    return 0;
}
