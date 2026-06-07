// bench_easywsclient_connect.cpp — WebSocket connect latency (easywsclient, client-only)

#include "../bench_common.hpp"
#include "easywsclient.hpp"
#include <wscpp/server.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdio>
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
    srv.set_on_connection([](std::shared_ptr<connection> conn) {
        conn->set_on_open([conn]() { conn->close(1000, "bench"); });
    });
    srv.listen(port);
    srv.start();

    // easywsclient URL parser requires no trailing slash when path is empty.
    const std::string url = "ws://127.0.0.1:" + std::to_string(port);
    std::vector<double> latencies_ms;
    latencies_ms.reserve(static_cast<std::size_t>(samples));

    for (int i = 0; i < samples; ++i) {
        const clock::time_point start = clock::now();
        easywsclient::WebSocket::pointer ws = easywsclient::WebSocket::from_url(url);
        if (!ws) {
            std::fprintf(stderr, "easywsclient connect failed on iteration %d\n", i);
            break;
        }
        const double ms = elapsed_sec(start, clock::now()) * 1000.0;
        latencies_ms.push_back(ms);
        delete ws;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    srv.stop();

    std::printf("easywsclient connect (%zu samples, port %u)\n", latencies_ms.size(), port);
    print_latency("connect_latency", latencies_ms);

    return latencies_ms.empty() ? 1 : 0;
}
