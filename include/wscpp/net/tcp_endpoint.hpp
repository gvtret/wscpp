#ifndef WSCPP_NET_TCP_ENDPOINT_HPP
#define WSCPP_NET_TCP_ENDPOINT_HPP

#include <cstdint>
#include <string>

namespace wscpp {
namespace net {

/** @brief TCP endpoint for native socket transport. */
struct tcp_endpoint {
    std::string address;
    std::uint16_t port;
};

} // namespace net
} // namespace wscpp

#endif
