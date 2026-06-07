// bench_beast_roundtrip.cpp — echo latency comparison (Boost.Beast)

#include "../bench_common.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <atomic>
#include <cstdio>
#include <future>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

void run_echo_server(std::promise<uint16_t> port_promise, std::atomic<bool>& running) {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        port_promise.set_value(acceptor.local_endpoint().port());

        tcp::socket socket(ioc);
        acceptor.accept(socket);

        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();

        beast::flat_buffer buffer;
        while (running.load()) {
            ws.read(buffer);
            ws.text(ws.got_text());
            ws.write(buffer.data());
            buffer.consume(buffer.size());
        }
    } catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed &&
            se.code() != net::error::operation_aborted &&
            se.code() != net::error::connection_reset) {
            std::fprintf(stderr, "beast server: %s\n", se.code().message().c_str());
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "beast server: %s\n", ex.what());
    }
}

bool run_echo_client(uint16_t port, int samples, std::vector<double>& latencies_ms) {
    try {
        net::io_context ioc;
        websocket::stream<tcp::socket> ws(ioc);

        const std::string host = "127.0.0.1";
        const std::string port_str = std::to_string(port);
        tcp::resolver resolver(ioc);
        net::connect(beast::get_lowest_layer(ws), resolver.resolve(host, port_str));
        ws.handshake(host, "/");

        beast::flat_buffer buffer;
        latencies_ms.reserve(static_cast<std::size_t>(samples));

        for (int i = 0; i < samples; ++i) {
            const std::string msg = "ping-" + std::to_string(i);
            const clock::time_point t0 = clock::now();
            ws.write(net::buffer(msg));
            ws.read(buffer);
            latencies_ms.push_back(elapsed_sec(t0, clock::now()) * 1000.0);
            buffer.consume(buffer.size());
        }

        ws.close(websocket::close_code::normal);
        return true;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "beast client: %s\n", ex.what());
        return false;
    }
}

bool run_throughput(uint16_t port, std::size_t payload_bytes, int iterations, double& seconds_out) {
    try {
        net::io_context ioc;
        websocket::stream<tcp::socket> ws(ioc);

        const std::string host = "127.0.0.1";
        const std::string port_str = std::to_string(port);
        tcp::resolver resolver(ioc);
        net::connect(beast::get_lowest_layer(ws), resolver.resolve(host, port_str));
        ws.handshake(host, "/");

        const std::string blob(payload_bytes, static_cast<char>(0xAB));
        beast::flat_buffer buffer;
        const clock::time_point t0 = clock::now();
        for (int i = 0; i < iterations; ++i) {
            ws.binary(true);
            ws.write(net::buffer(blob));
            ws.read(buffer);
            buffer.consume(buffer.size());
        }
        seconds_out = elapsed_sec(t0, clock::now());
        ws.close(websocket::close_code::normal);
        return true;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "beast throughput: %s\n", ex.what());
        return false;
    }
}

uint16_t start_echo_server(std::thread& server_thread, std::atomic<bool>& running) {
    std::promise<uint16_t> port_promise;
    std::future<uint16_t> port_future = port_promise.get_future();
    server_thread = std::thread(run_echo_server, std::move(port_promise), std::ref(running));
    return port_future.get();
}

} // namespace

int main() {
    const int samples = 100;

    std::atomic<bool> running(true);
    std::thread server_thread;
    const uint16_t port = start_echo_server(server_thread, running);

    std::vector<double> latencies_ms;
    const bool ok = run_echo_client(port, samples, latencies_ms);
    running = false;
    if (server_thread.joinable()) {
        server_thread.join();
    }

    if (!ok) {
        return 1;
    }

    std::printf("beast roundtrip (%zu samples, port %u)\n", latencies_ms.size(), port);
    print_latency("echo_latency", latencies_ms);

    const std::size_t throughput_bytes = 64 * 1024;
    running = true;
    const uint16_t tp_port = start_echo_server(server_thread, running);
    double tp_seconds = 0.0;
    if (run_throughput(tp_port, throughput_bytes, 100, tp_seconds)) {
        print_throughput("echo_throughput_64k",
                         static_cast<double>(throughput_bytes * 100),
                         tp_seconds,
                         100);
    }
    running = false;
    if (server_thread.joinable()) {
        server_thread.join();
    }

    return 0;
}
