#ifndef WSCPP_BENCH_NET_ARGS_HPP
#define WSCPP_BENCH_NET_ARGS_HPP

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace wscpp {
namespace bench {

struct net_bench_args {
    std::string host;
    uint16_t port;
    int samples;
};

inline void print_net_client_usage(const char *argv0, const char *name) {
    std::fprintf(stderr, "Usage: %s <host> [port] [samples]\n", argv0);
    std::fprintf(stderr, "  %s — remote echo server benchmark\n", name);
    std::fprintf(stderr, "  Defaults: port=19081 samples=100\n");
}

inline void print_echo_server_usage(const char *argv0) {
    std::fprintf(stderr, "Usage: %s [port] [bind_host]\n", argv0);
    std::fprintf(stderr, "  Defaults: port=19081 bind=0.0.0.0\n");
}

inline bool parse_net_client_args(int argc, char *argv[], net_bench_args &out, const char *name) {
    if (argc < 2) {
        print_net_client_usage(argv[0], name);
        return false;
    }
    out.host = argv[1];
    out.port = argc > 2 ? static_cast<uint16_t>(std::atoi(argv[2])) : static_cast<uint16_t>(19081);
    out.samples = argc > 3 ? std::atoi(argv[3]) : 100;
    return out.samples > 0;
}

inline bool parse_echo_server_args(int argc, char *argv[], uint16_t &port, std::string &bind_host) {
    port = 19081;
    bind_host = "0.0.0.0";
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            print_echo_server_usage(argv[0]);
            return false;
        }
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }
    if (argc > 2) {
        bind_host = argv[2];
    }
    return true;
}

} // namespace bench
} // namespace wscpp

#endif
