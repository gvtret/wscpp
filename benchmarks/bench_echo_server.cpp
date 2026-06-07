// bench_echo_server.cpp — WebSocket echo server for remote network benchmarks

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <wscpp/server.hpp>

using namespace wscpp;

namespace {

void usage(const char *argv0) {
    std::fprintf(stderr, "Usage: %s [port] [bind_host]\n", argv0);
    std::fprintf(stderr, "  Defaults: port=19081 bind=0.0.0.0\n");
}

} // namespace

int main(int argc, char *argv[]) {
    uint16_t port = 19081;
    std::string bind_host = "0.0.0.0";

    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            usage(argv[0]);
            return 0;
        }
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }
    if (argc > 2) {
        bind_host = argv[2];
    }

    server srv;
    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
        conn->set_on_message([conn](const std::vector<uint8_t> &data, frame::opcode op) {
            if (op == frame::opcode::BINARY) {
                conn->send_binary(data.data(), data.size());
            } else {
                conn->send_text(std::string(data.begin(), data.end()));
            }
        });
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

    std::printf("wscpp echo server listening on %s:%u\n", bind_host.c_str(), port);
    srv.join();
    return 0;
}
