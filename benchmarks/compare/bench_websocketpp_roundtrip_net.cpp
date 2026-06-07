// bench_websocketpp_roundtrip_net.cpp — client-only echo latency over LAN

#include "../bench_net_args.hpp"
#include "../bench_util.hpp"
#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

using namespace wscpp::bench;

typedef websocketpp::client<websocketpp::config::asio> ws_client;

int main(int argc, char *argv[]) {
    net_bench_args args;
    if (!parse_net_client_args(argc, argv, args, "bench_websocketpp_roundtrip_net")) {
        return 1;
    }

    print_compare_banner("bench_websocketpp_roundtrip_net");
    std::printf("target ws://%s:%u/ samples=%d\n", args.host.c_str(), args.port, args.samples);

    std::vector<double> latencies_ms;
    std::mutex latency_mutex;
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(args.samples));
    std::atomic<bool> done(false);

    ws_client client;
    client.init_asio();
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);
    int next_send = 0;

    const std::string url = "ws://" + args.host + ":" + std::to_string(args.port) + "/";
    websocketpp::lib::error_code ec;
    ws_client::connection_ptr conn = client.get_connection(url, ec);
    if (ec) {
        std::fprintf(stderr, "connection error: %s\n", ec.message().c_str());
        return 1;
    }

    conn->set_open_handler([&](websocketpp::connection_hdl) {
        send_times[0] = clock::now();
        client.send(conn->get_handle(), "ping-0", websocketpp::frame::opcode::text);
        next_send = 1;
    });

    conn->set_message_handler([&](websocketpp::connection_hdl, ws_client::message_ptr) {
        const int n = next_send - 1;
        if (n >= 0 && n < args.samples) {
            const double ms =
                elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
            std::lock_guard<std::mutex> lock(latency_mutex);
            latencies_ms.push_back(ms);
        }
        if (next_send < args.samples) {
            send_times[static_cast<std::size_t>(next_send)] = clock::now();
            client.send(conn->get_handle(), "ping-" + std::to_string(next_send),
                        websocketpp::frame::opcode::text);
            ++next_send;
        } else {
            websocketpp::lib::error_code close_ec;
            client.close(conn->get_handle(), websocketpp::close::status::normal, "done", close_ec);
            done = true;
        }
    });

    client.connect(conn);

    std::thread client_thread([&]() {
        try {
            client.run();
        } catch (...) {
            done = true;
        }
    });

    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    client_thread.join();

    std::printf("websocketpp net roundtrip (%zu samples)\n", latencies_ms.size());
    print_latency("echo_latency", latencies_ms);

    return latencies_ms.size() == static_cast<std::size_t>(args.samples) ? 0 : 1;
}
