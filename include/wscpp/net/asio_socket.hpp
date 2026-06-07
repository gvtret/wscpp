#ifndef WSCPP_NET_ASIO_SOCKET_HPP
#define WSCPP_NET_ASIO_SOCKET_HPP

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <memory>
#include <string>

namespace wscpp {
namespace net {

class asio_socket {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;

    explicit asio_socket(asio::io_context& io_context);
    ~asio_socket();

    asio_socket(const asio_socket&) = delete;
    asio_socket& operator=(const asio_socket&) = delete;

    void connect(const std::string& host, const std::string& port);
    void connect(const tcp::endpoint& endpoint);
    void adopt(tcp::socket socket);
    void close();

    std::size_t write(const void* data, std::size_t size);
    std::size_t read(void* data, std::size_t size);
    std::size_t read_some(void* data, std::size_t size);
    std::size_t read_until(asio::streambuf& buf, const std::string& delim);

    tcp::socket& native_socket();
    const tcp::socket& native_socket() const;

    bool is_open() const;

    void enable_ssl(std::shared_ptr<ssl_context> ctx);
    void set_ssl_hostname(const std::string& host);
    void ssl_handshake(bool as_client);
    bool is_ssl() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace net
} // namespace wscpp

#endif // WSCPP_NET_ASIO_SOCKET_HPP
