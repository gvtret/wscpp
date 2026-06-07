// bench_wscpp_connect_server.cpp — accept and close (easywsclient connect latency over LAN)

#include "bench_net_args.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <wscpp/server.hpp>

using namespace wscpp;
using namespace wscpp::bench;

int main(int argc, char *argv[]) {
    uint16_t port = 19081;
    std::string bind_host = "0.0.0.0";
    if (!parse_echo_server_args(argc, argv, port, bind_host)) {
        return argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") ? 0
                                                                                             : 1;
    }

    server srv;
    srv.set_on_connection([](std::shared_ptr<connection> conn) {
        conn->set_on_open([conn]() { conn->close(1000, "bench"); });
    });

    const std::error_code listen_ec = srv.listen(bind_host, port);
    if (listen_ec) {
        std::fprintf(stderr, "listen %s:%u failed: %s\n", bind_host.c_str(), port,
                     listen_ec.message().c_str());
        return 1;
    }
    const std::error_code start_ec = srv.start();
    if (start_ec) {
        std::fprintf(stderr, "start failed: %s\n", start_ec.message().c_str());
        return 1;
    }

    std::printf("wscpp connect server listening on %s:%u\n", bind_host.c_str(), port);
    srv.join();
    return 0;
}
