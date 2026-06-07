#ifndef WSCPP_NET_LINUX_SOCKET_HPP
#define WSCPP_NET_LINUX_SOCKET_HPP

#include <wscpp/net/io_context.hpp>
#include <wscpp/net/openssl_context.hpp>
#include <wscpp/net/stream_buffer.hpp>
#include <wscpp/net/tcp_endpoint.hpp>
#include <memory>
#include <string>
#include <system_error>

struct ssl_st;

namespace wscpp {
namespace net {

/**
 * @brief Sync TCP/TLS stream using POSIX sockets and OpenSSL.
 */
class linux_socket {
public:
    explicit linux_socket(io_context& io_context);
    ~linux_socket();

    linux_socket(const linux_socket&) = delete;
    linux_socket& operator=(const linux_socket&) = delete;

    std::error_code connect(const std::string& host, const std::string& port);
    std::error_code connect(const tcp_endpoint& endpoint);
    std::error_code adopt(int fd);
    void close();

    std::size_t write(const void* data, std::size_t size, std::error_code& ec);
    std::size_t read(void* data, std::size_t size, std::error_code& ec);
    std::size_t read_some(void* data, std::size_t size, std::error_code& ec);
    std::size_t read_until(stream_buffer& buf, const std::string& delim, std::error_code& ec);

    int native_fd() const;
    bool is_open() const;

    std::error_code enable_ssl(std::shared_ptr<ssl_context> ctx);
    std::error_code set_ssl_hostname(const std::string& host);
    std::error_code ssl_handshake(bool as_client);
    bool is_ssl() const;

private:
    std::size_t read_raw(void* data, std::size_t size, bool exact, std::error_code& ec);
    std::size_t read_some_raw(void* data, std::size_t size, std::error_code& ec);
    std::error_code ssl_error(int result);

    io_context& io_context_;
    int fd_;
    std::shared_ptr<openssl_context> ssl_context_;
    ssl_st* ssl_;
    bool ssl_enabled_;
};

using stream_socket = linux_socket;

} // namespace net
} // namespace wscpp

#endif
