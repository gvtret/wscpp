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
    
    asio_socket();
    explicit asio_socket(asio::io_context& io_context);
    virtual ~asio_socket();
    
    // Connection
    void connect(const std::string& host, const std::string& port);
    void connect(const tcp::endpoint& endpoint);
    void close();
    
    // I/O operations
    size_t write(const void* data, size_t size);
    size_t read(void* data, size_t size);
    
    // State
    bool is_open() const;
    
    // SSL support
    void set_ssl_context(std::shared_ptr<ssl_context> ctx);
    bool is_ssl() const;
    
private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace net
} // namespace wscpp

#endif // WSCPP_NET_ASIO_SOCKET_HPP
