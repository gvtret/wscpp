#ifndef WSCPP_CONNECTION_HPP
#define WSCPP_CONNECTION_HPP

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include "net/asio_socket.hpp"
#include "frame/parser.hpp"
#include "frame/builder.hpp"

namespace wscpp {

class connection {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;
    using socket_type = net::asio_socket;
    
    // Callback types
    using message_callback = std::function<void(const std::vector<uint8_t>&, opcode)>;
    using close_callback = std::function<void(uint16_t, const std::string&)>;
    using error_callback = std::function<void(const std::string&)>;
    using open_callback = std::function<void()>;
    
    connection(asio::io_context& io_context);
    ~connection();
    
    // Disable copy
    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;
    
    // Connection management
    void connect(const std::string& host, const std::string& port);
    void connect(const tcp::endpoint& endpoint);
    void close(uint16_t status_code = 1000, const std::string& reason = "");
    
    // I/O operations
    void send_text(const std::string& text, bool fin = true);
    void send_binary(const uint8_t* data, size_t size, bool fin = true);
    
    // Callbacks
    void set_on_open(open_callback cb);
    void set_on_message(message_callback cb);
    void set_on_close(close_callback cb);
    void set_on_error(error_callback cb);
    
    // State
    bool is_open() const;
    bool is_ssl() const;
    
    // Get underlying socket
    socket_type& socket();
    const socket_type& socket() const;
    
private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace wscpp

#endif // WSCPP_CONNECTION_HPP
