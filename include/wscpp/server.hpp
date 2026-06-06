#ifndef WSCPP_SERVER_HPP
#define WSCPP_SERVER_HPP

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include "connection.hpp"
#include "frame/parser.hpp"
#include "frame/builder.hpp"

namespace wscpp {

class server {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;
    using connection_type = connection;
    
    // Callback types
    using connection_callback = std::function<void(std::shared_ptr<connection_type>)>;
    using message_callback = std::function<void(std::shared_ptr<connection_type>, const std::vector<uint8_t>&, frame::opcode)>;
    using close_callback = std::function<void(std::shared_ptr<connection_type>, uint16_t, const std::string&)>;
    using error_callback = std::function<void(std::shared_ptr<connection_type>, const std::string&)>;
    
    server();
    ~server();
    
    // Disable copy
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    
    // Server management
    void listen(uint16_t port);
    void listen(const std::string& host, uint16_t port);
    
    // SSL support
    void set_ssl_context(std::shared_ptr<ssl_context> ctx);
    
    // Callbacks
    void set_on_connection(connection_callback cb);
    void set_on_message(message_callback cb);
    void set_on_close(close_callback cb);
    void set_on_error(error_callback cb);
    
    // Run server
    void start();
    void stop();
    
    // State
    bool is_running() const;
    
private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace wscpp

#endif // WSCPP_SERVER_HPP
