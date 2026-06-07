// bench_websocketpp_roundtrip.cpp — echo latency comparison baseline

#include "../bench_common.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

typedef websocketpp::server<websocketpp::config::asio> ws_server;
typedef websocketpp::client<websocketpp::config::asio> ws_client;

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

    ws_server server;
    server.init_asio();
    server.set_reuse_addr(true);
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);
    server.set_message_handler([&server](websocketpp::connection_hdl hdl, ws_server::message_ptr msg) {
        server.send(hdl, msg->get_payload(), msg->get_opcode());
    });
    server.listen(port);
    server.start_accept();

    std::atomic<bool> server_done(false);
    std::thread server_thread([&]() {
        try {
            server.run();
        } catch (...) {
        }
        server_done = true;
    });

    std::vector<double> latencies_ms;
    std::mutex latency_mutex;
    std::vector<clock::time_point> send_times(static_cast<std::size_t>(samples));
    std::atomic<bool> done(false);

    ws_client client;
    client.init_asio();
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);
    int next_send = 0;

    websocketpp::lib::error_code ec;
    ws_client::connection_ptr conn =
        client.get_connection("ws://127.0.0.1:" + std::to_string(port) + "/", ec);
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
        if (n >= 0 && n < samples) {
            const double ms = elapsed_sec(send_times[static_cast<std::size_t>(n)], clock::now()) * 1000.0;
            std::lock_guard<std::mutex> lock(latency_mutex);
            latencies_ms.push_back(ms);
        }
        if (next_send < samples) {
            send_times[static_cast<std::size_t>(next_send)] = clock::now();
            client.send(conn->get_handle(),
                          "ping-" + std::to_string(next_send),
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
    server.stop_listening();
    server.stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }

    std::printf("websocketpp roundtrip (%zu samples, port %u)\n", latencies_ms.size(), port);
    print_latency("echo_latency", latencies_ms);

    return 0;
}
