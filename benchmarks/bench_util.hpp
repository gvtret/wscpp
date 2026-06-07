#ifndef WSCPP_BENCH_UTIL_HPP
#define WSCPP_BENCH_UTIL_HPP

#include "bench_common.hpp"
#include <cstdint>
#include <cstdio>

#ifndef WSCPP_USE_ASIO
#define WSCPP_USE_ASIO 1
#endif

#if defined(WSCPP_BENCH_HAVE_ASIO)
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#endif

namespace wscpp {
namespace bench {

inline const char *transport_label() {
#if WSCPP_USE_ASIO
    return "asio";
#else
    return "linux";
#endif
}

inline void print_banner(const char *target) {
    std::printf("=== %s (wscpp transport: %s) ===\n", target, transport_label());
}

inline void print_compare_banner(const char *target) {
    std::printf("=== %s ===\n", target);
}

#if defined(WSCPP_BENCH_HAVE_ASIO)

inline uint16_t pick_free_port() {
    asio::io_context io;
    asio::ip::tcp::acceptor acc(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    return acc.local_endpoint().port();
}

#else

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

inline uint16_t pick_free_port() {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return 0;
    }
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return 0;
    }
    socklen_t len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len) != 0) {
        ::close(fd);
        return 0;
    }
    ::close(fd);
    return ntohs(addr.sin_port);
}

#endif

} // namespace bench
} // namespace wscpp

#endif
