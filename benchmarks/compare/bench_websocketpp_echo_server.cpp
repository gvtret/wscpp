// bench_websocketpp_echo_server.cpp — echo server for LAN benchmarks

#include "../bench_net_args.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> ws_server;

int main(int argc, char *argv[]) {
    uint16_t port = 19081;
    std::string bind_host = "0.0.0.0";
    if (!wscpp::bench::parse_echo_server_args(argc, argv, port, bind_host)) {
        return argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") ? 0
                                                                                              : 1;
    }

    ws_server server;
    server.init_asio();
    server.set_reuse_addr(true);
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);
    server.set_message_handler(
        [&server](websocketpp::connection_hdl hdl, ws_server::message_ptr msg) {
            server.send(hdl, msg->get_payload(), msg->get_opcode());
        });

    websocketpp::lib::error_code ec;
    server.listen(bind_host, std::to_string(port), ec);
    if (ec) {
        std::fprintf(stderr, "listen failed: %s\n", ec.message().c_str());
        return 1;
    }
    server.start_accept();
    std::printf("websocketpp echo server listening on %s:%u\n", bind_host.c_str(), port);
    server.run();
    return 0;
}
