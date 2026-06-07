#ifndef WSCPP_CLIENT_HPP
#define WSCPP_CLIENT_HPP

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include "connection.hpp"
#include "frame/parser.hpp"
#include "frame/builder.hpp"

namespace wscpp {

class client {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;
    using connection_type = connection;
    
    // Callback types
    using message_callback = std::function<void(const std::vector<uint8_t>&, frame::opcode)>;
    using close_callback = std::function<void(uint16_t, const std::string&)>;
    using error_callback = std::function<void(const std::string&)>;
    using open_callback = std::function<void()>;
    
    client();
    ~client();
    
    // Disable copy
    client(const client&) = delete;
    client& operator=(const client&) = delete;
    
    // Connection management
    void connect(const std::string& url);
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
    
    // Run io_context (blocking)
    void run();
    
    // Stop io_context
    void stop();
    
private:
    struct url_info {
        std::string scheme;
        std::string host;
        std::string port;
        std::string path;
        bool secure;
    };
    
    url_info parse_url(const std::string& url);
    
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace wscpp

#endif // WSCPP_CLIENT_HPP
