#include <wscpp/client.hpp>
#include <asio/connect.hpp>
#include <asio/buffer.hpp>
#include <asio/write.hpp>
#include <asio/read_until.hpp>
#include <asio/streambuf.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/ssl/error.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <random>

namespace wscpp {

class client::impl {
public:
    impl()
        : io_context_(),
          connection_(io_context_),
          is_open_(false),
          is_closing_(false) {
        // Default SSL context
        ssl_context_ = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
        ssl_context_->set_verify_mode(asio::ssl::verify_none);
    }
    
    ~impl() {
        stop();
    }
    
    void connect(const std::string& url) {
        url_info info = parse_url(url);
        
        // Connect
        if (info.secure) {
            connect_ssl(info);
        } else {
            connect_tcp(info);
        }
        
        is_open_ = true;
        
        // Call open callback
        if (on_open_) {
            on_open_();
        }
    }
    
    void close(uint16_t status_code, const std::string& reason) {
        if (!is_open_ || is_closing_) {
            return;
        }
        
        is_closing_ = true;
        connection_.close(status_code, reason);
        is_open_ = false;
    }
    
    void send_text(const std::string& text, bool fin) {
        connection_.send_text(text, fin);
    }
    
    void send_binary(const uint8_t* data, size_t size, bool fin) {
        connection_.send_binary(data, size, fin);
    }
    
    void set_on_open(open_callback cb) {
        on_open_ = std::move(cb);
    }
    
    void set_on_message(message_callback cb) {
        connection_.set_on_message(std::move(cb));
    }
    
    void set_on_close(close_callback cb) {
        connection_.set_on_close(std::move(cb));
    }
    
    void set_on_error(error_callback cb) {
        on_error_ = std::move(cb);
    }
    
    bool is_open() const {
        return is_open_;
    }
    
    bool is_ssl() const {
        return connection_.is_ssl();
    }
    
    void run() {
        asio::io_context::work work(io_context_);
        io_context_.run();
    }
    
    void stop() {
        io_context_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    
private:
    url_info parse_url(const std::string& url) {
        url_info info;
        
        // Parse scheme
        size_t pos = url.find("://");
        if (pos != std::string::npos) {
            info.scheme = url.substr(0, pos);
            info.secure = (info.scheme == "wss");
        } else {
            info.scheme = "ws";
            info.secure = false;
        }
        
        // Parse host and port
        std::string remaining = url.substr(pos + 3);
        
        // Check for path
        pos = remaining.find('/');
        if (pos != std::string::npos) {
            info.path = remaining.substr(pos);
            remaining = remaining.substr(0, pos);
        } else {
            info.path = "/";
        }
        
        // Parse host:port
        pos = remaining.find(':');
        if (pos != std::string::npos) {
            info.host = remaining.substr(0, pos);
            info.port = remaining.substr(pos + 1);
        } else {
            info.host = remaining;
            info.port = info.secure ? "443" : "80";
        }
        
        return info;
    }
    
    void connect_tcp(const url_info& info) {
        tcp::resolver resolver(io_context_);
        auto endpoint_iterator = resolver.resolve(info.host, info.port);
        
        // Connect
        asio::connect(connection_.socket().native_handle(), endpoint_iterator);
        
        // WebSocket handshake
        perform_handshake(info);
    }
    
    void connect_ssl(const url_info& info) {
        // Create SSL stream
        asio::ssl::stream<tcp::socket&> ssl_stream(
            connection_.socket().native_handle().native_handle(), 
            *ssl_context_
        );
        
        // Perform SSL handshake
        ssl_stream.handshake(asio::ssl::stream_base::client);
        
        // WebSocket handshake
        perform_handshake(info);
    }
    
    void perform_handshake(const url_info& info) {
        // Generate WebSocket key
        std::string key = generate_key();
        
        // Build handshake request
        std::ostringstream request;
        request << "GET " << info.path << " HTTP/1.1\r\n";
        request << "Host: " << info.host << ":" << info.port << "\r\n";
        request << "Upgrade: websocket\r\n";
        request << "Connection: Upgrade\r\n";
        request << "Sec-WebSocket-Key: " << key << "\r\n";
        request << "Sec-WebSocket-Version: 13\r\n";
        
        if (!info.secure) {
            request << "\r\n";
        }
        
        std::string handshake = request.str();
        
        // Send handshake
        asio::write(connection_.socket().native_handle(), asio::buffer(handshake));
        
        // Read response (simplified - in production, parse HTTP response properly)
        asio::streambuf response;
        asio::read_until(connection_.socket().native_handle(), response, "\r\n\r\n");
        
        std::string response_str = asio::buffer_cast<const char*>(response.data());
        
        // Verify response
        if (response_str.find("101 Switching Protocols") == std::string::npos) {
            throw std::runtime_error("WebSocket handshake failed");
        }
    }
    
    std::string generate_key() {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string key;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 63);
        
        for (int i = 0; i < 16; ++i) {
            key += chars[dis(gen)];
        }
        
        return key;
    }
    
private:
    asio::io_context io_context_;
    connection_type connection_;
    
    // State
    bool is_open_;
    bool is_closing_;
    
    // SSL context
    std::shared_ptr<asio::ssl::context> ssl_context_;
    
    // Callbacks
    open_callback on_open_;
    error_callback on_error_;
    
    // Thread
    std::thread thread_;
};

// Public interface
client::client()
    : pimpl_(std::make_unique<impl>()) {}

client::~client() = default;

void client::connect(const std::string& url) {
    pimpl_->connect(url);
}

void client::close(uint16_t status_code, const std::string& reason) {
    pimpl_->close(status_code, reason);
}

void client::send_text(const std::string& text, bool fin) {
    pimpl_->send_text(text, fin);
}

void client::send_binary(const uint8_t* data, size_t size, bool fin) {
    pimpl_->send_binary(data, size, fin);
}

void client::set_on_open(open_callback cb) {
    pimpl_->set_on_open(std::move(cb));
}

void client::set_on_message(message_callback cb) {
    pimpl_->set_on_message(std::move(cb));
}

void client::set_on_close(close_callback cb) {
    pimpl_->set_on_close(std::move(cb));
}

void client::set_on_error(error_callback cb) {
    pimpl_->set_on_error(std::move(cb));
}

bool client::is_open() const {
    return pimpl_->is_open();
}

bool client::is_ssl() const {
    return pimpl_->is_ssl();
}

void client::run() {
    pimpl_->run();
}

void client::stop() {
    pimpl_->stop();
}

} // namespace wscpp
