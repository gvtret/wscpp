#ifndef WSCPP_NET_IO_CONTEXT_HPP
#define WSCPP_NET_IO_CONTEXT_HPP

namespace wscpp {
namespace net {

/** @brief Placeholder event loop when transport does not use ASIO. */
class io_context {
  public:
    void run() {}

    void stop() {}
};

} // namespace net
} // namespace wscpp

#endif
