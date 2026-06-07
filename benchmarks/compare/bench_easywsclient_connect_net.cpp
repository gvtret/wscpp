// bench_easywsclient_connect_net.cpp — connect latency vs remote wscpp connect server

#include "../bench_net_args.hpp"
#include "../bench_util.hpp"
#include "easywsclient.hpp"
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

int main(int argc, char *argv[]) {
    net_bench_args args;
    if (!parse_net_client_args(argc, argv, args, "bench_easywsclient_connect_net")) {
        return 1;
    }

    print_compare_banner("bench_easywsclient_connect_net");
    std::printf("target ws://%s:%u samples=%d\n", args.host.c_str(), args.port, args.samples);

    const std::string url = "ws://" + args.host + ":" + std::to_string(args.port);
    std::vector<double> latencies_ms;
    latencies_ms.reserve(static_cast<std::size_t>(args.samples));

    for (int i = 0; i < args.samples; ++i) {
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

    std::printf("easywsclient net connect (%zu samples)\n", latencies_ms.size());
    print_latency("connect_latency", latencies_ms);

    return latencies_ms.empty() ? 1 : 0;
}
