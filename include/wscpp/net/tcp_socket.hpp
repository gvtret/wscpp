#ifndef WSCPP_NET_TCP_SOCKET_HPP
#define WSCPP_NET_TCP_SOCKET_HPP

#if !WSCPP_USE_ASIO

namespace wscpp {
namespace net {

/** @brief Adopted connected TCP socket (file descriptor) for server accept path. */
class tcp_socket {
public:
    explicit tcp_socket(int fd) : fd_(fd) {}

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;

    tcp_socket(tcp_socket&& other) : fd_(other.fd_) { other.fd_ = -1; }

    tcp_socket& operator=(tcp_socket&& other) {
        if (this != &other) {
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int release() {
        const int fd = fd_;
        fd_ = -1;
        return fd;
    }

    int native_fd() const { return fd_; }

private:
    int fd_;
};

} // namespace net
} // namespace wscpp

#endif

#endif
