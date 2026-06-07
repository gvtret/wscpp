// bench_beast_roundtrip_net.cpp — client-only echo + throughput over LAN

#include "../bench_net_args.hpp"
#include "../bench_util.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdio>
#include <string>
#include <vector>

using namespace wscpp::bench;

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

bool run_echo_client(const net_bench_args &args, std::vector<double> &latencies_ms) {
    try {
        net::io_context ioc;
        websocket::stream<tcp::socket> ws(ioc);

        const std::string port_str = std::to_string(args.port);
        tcp::resolver resolver(ioc);
        net::connect(beast::get_lowest_layer(ws), resolver.resolve(args.host, port_str));
        ws.handshake(args.host, "/");

        beast::flat_buffer buffer;
        latencies_ms.reserve(static_cast<std::size_t>(args.samples));

        for (int i = 0; i < args.samples; ++i) {
            const std::string msg = "ping-" + std::to_string(i);
            const clock::time_point t0 = clock::now();
            ws.write(net::buffer(msg));
            ws.read(buffer);
            latencies_ms.push_back(elapsed_sec(t0, clock::now()) * 1000.0);
            buffer.consume(buffer.size());
        }

        ws.close(websocket::close_code::normal);
        return true;
    } catch (const std::exception &ex) {
        std::fprintf(stderr, "beast client: %s\n", ex.what());
        return false;
    }
}

bool run_throughput(const net_bench_args &args, std::size_t payload_bytes, int iterations,
                    double &seconds_out) {
    try {
        net::io_context ioc;
        websocket::stream<tcp::socket> ws(ioc);

        const std::string port_str = std::to_string(args.port);
        tcp::resolver resolver(ioc);
        net::connect(beast::get_lowest_layer(ws), resolver.resolve(args.host, port_str));
        ws.handshake(args.host, "/");

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
    } catch (const std::exception &ex) {
        std::fprintf(stderr, "beast throughput: %s\n", ex.what());
        return false;
    }
}

} // namespace

int main(int argc, char *argv[]) {
    net_bench_args args;
    if (!parse_net_client_args(argc, argv, args, "bench_beast_roundtrip_net")) {
        return 1;
    }

    print_compare_banner("bench_beast_roundtrip_net");
    std::printf("target ws://%s:%u/ samples=%d\n", args.host.c_str(), args.port, args.samples);

    std::vector<double> latencies_ms;
    if (!run_echo_client(args, latencies_ms)) {
        return 1;
    }

    std::printf("beast net roundtrip (%zu samples)\n", latencies_ms.size());
    print_latency("echo_latency", latencies_ms);

    const std::size_t throughput_bytes = 64 * 1024;
    double tp_seconds = 0.0;
    if (run_throughput(args, throughput_bytes, 100, tp_seconds)) {
        print_throughput("echo_throughput_64k", static_cast<double>(throughput_bytes * 100),
                         tp_seconds, 100);
    }

    return latencies_ms.size() == static_cast<std::size_t>(args.samples) ? 0 : 1;
}
