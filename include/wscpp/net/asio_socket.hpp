#ifndef WSCPP_NET_ASIO_SOCKET_HPP
#define WSCPP_NET_ASIO_SOCKET_HPP

/**
 * @file asio_socket.hpp
 * @brief TCP socket wrapper with optional TLS (ASIO).
 */

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <memory>
#include <string>
#include <system_error>

namespace wscpp {
namespace net {

/**
 * @brief Sync TCP/TLS stream used by @ref wscpp::connection.
 */
class asio_socket {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;

    explicit asio_socket(asio::io_context& io_context);
    ~asio_socket();

    asio_socket(const asio_socket&) = delete;
    asio_socket& operator=(const asio_socket&) = delete;

    std::error_code connect(const std::string& host, const std::string& port);
    std::error_code connect(const tcp::endpoint& endpoint);
    std::error_code adopt(tcp::socket socket);
    void close();

    std::size_t write(const void* data, std::size_t size, std::error_code& ec);
    std::size_t read(void* data, std::size_t size, std::error_code& ec);
    std::size_t read_some(void* data, std::size_t size, std::error_code& ec);
    std::size_t read_until(asio::streambuf& buf, const std::string& delim, std::error_code& ec);

    tcp::socket& native_socket();
    const tcp::socket& native_socket() const;

    bool is_open() const;

    /** @brief Wrap socket with TLS using @p ctx. */
    std::error_code enable_ssl(std::shared_ptr<ssl_context> ctx);

    /** @brief Set TLS SNI hostname (client, before handshake). */
    std::error_code set_ssl_hostname(const std::string& host);

    /** @brief Perform TLS handshake (@p as_client true for client role). */
    std::error_code ssl_handshake(bool as_client);

    bool is_ssl() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace net
} // namespace wscpp

#endif // WSCPP_NET_ASIO_SOCKET_HPP
