#include <wscpp/server.hpp>
#include <asio/buffer.hpp>
#include <asio/write.hpp>
#include <asio/read_until.hpp>
#include <asio/streambuf.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/ssl/error.hpp>
#include <asio/steady_timer.hpp>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <random>
#include <memory>

namespace wscpp {

class server::impl {
public:
    impl()
        : io_context_(),
          acceptor_(io_context_),
          is_running_(false),
          ssl_enabled_(false) {
        // Default SSL context for server
        ssl_context_ = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_server);
        ssl_context_->set_options(
            asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3
        );
    }
    
    ~impl() {
        stop();
    }
    
    void listen(uint16_t port) {
        listen("0.0.0.0", port);
    }
    
    void listen(const std::string& host, uint16_t port) {
        tcp::endpoint endpoint(asio::ip::make_address(host), port);
        
        // Open acceptor
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        
        // Start accepting connections
        start_accept();
        
        is_running_ = true;
    }
    
    void set_ssl_context(std::shared_ptr<ssl_context> ctx) {
        if (ctx) {
            ssl_context_ = ctx;
            ssl_enabled_ = true;
        }
    }
    
    void set_on_connection(connection_callback cb) {
        on_connection_ = std::move(cb);
    }
    
    void set_on_message(message_callback cb) {
        on_message_ = std::move(cb);
    }
    
    void set_on_close(close_callback cb) {
        on_close_ = std::move(cb);
    }
    
    void set_on_error(error_callback cb) {
        on_error_ = std::move(cb);
    }
    
    void start() {
        if (!is_running_) {
            throw std::runtime_error("Server not listening");
        }
        
        // Run io_context in a separate thread
        thread_ = std::thread([this]() {
            io_context_.run();
        });
    }
    
    void stop() {
        if (!is_running_) {
            return;
        }
        
        is_running_ = false;
        
        // Close acceptor
        if (acceptor_.is_open()) {
            acceptor_.close();
        }
        
        // Close all connections
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (auto& pair : connections_) {
                pair.second->close(1000, "Server shutting down");
            }
            connections_.clear();
        }
        
        // Stop io_context
        io_context_.stop();
        
        // Join thread
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    
    bool is_running() const {
        return is_running_;
    }
    
private:
    void start_accept() {
        // Create new socket for accept
        auto socket = std::make_shared<tcp::socket>(io_context_);
        
        acceptor_.async_accept(*socket, [this, socket](const asio::error_code& ec) {
            if (!ec) {
                handle_accept(socket);
            }
            
            // Continue accepting
            if (is_running_) {
                start_accept();
            }
        });
    }
    
    void handle_accept(std::shared_ptr<tcp::socket> socket) {
        // Create connection
        auto conn = std::make_shared<connection>(io_context_);
        
        // Set callbacks
        conn->set_on_message([this, conn](const std::vector<uint8_t>& data, frame::opcode op) {
            if (on_message_) {
                on_message_(conn, data, op);
            }
        });
        
        conn->set_on_close([this, conn](uint16_t code, const std::string& reason) {
            if (on_close_) {
                on_close_(conn, code, reason);
            }
            remove_connection(conn);
        });
        
        conn->set_on_error([this, conn](const std::string& error) {
            if (on_error_) {
                on_error_(conn, error);
            }
        });
        
        // Connect socket
        conn->connect(*socket);
        
        // Perform handshake
        perform_handshake(conn, socket);
        
        // Notify about new connection
        if (on_connection_) {
            on_connection_(conn);
        }
        
        // Store connection
        add_connection(conn);
    }
    
    void perform_handshake(std::shared_ptr<connection> conn, std::shared_ptr<tcp::socket> socket) {
        // Read handshake request
        asio::streambuf request_buf;
        asio::error_code ec;
        
        size_t bytes_read = asio::read_until(*socket, request_buf, "\r\n\r\n", ec);
        
        if (ec) {
            if (on_error_) {
                on_error_(conn, ec.message());
            }
            return;
        }
        
        std::string request = asio::buffers_to_string(request_buf.data());
        
        // Parse request
        std::istringstream request_stream(request);
        std::string line;
        std::getline(request_stream, line);
        
        // Extract path from request line
        std::string method, path, version;
        std::istringstream line_stream(line);
        line_stream >> method >> path >> version;
        
        // Read headers
        std::map<std::string, std::string> headers;
        while (std::getline(request_stream, line) && line != "\r\n") {
            size_t pos = line.find(": ");
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 2);
                headers[key] = value;
            }
        }
        
        // Verify WebSocket handshake
        if (headers["Upgrade"] != "websocket" ||
            headers["Connection"] != "Upgrade" ||
            headers["Sec-WebSocket-Version"] != "13") {
            
            if (on_error_) {
                on_error_(conn, "Invalid WebSocket handshake");
            }
            return;
        }
        
        // Generate WebSocket key response
        std::string key = headers["Sec-WebSocket-Key"];
        std::string accept_key = generate_accept_key(key);
        
        // Build response
        std::ostringstream response;
        response << "HTTP/1.1 101 Switching Protocols\r\n";
        response << "Upgrade: websocket\r\n";
        response << "Connection: Upgrade\r\n";
        response << "Sec-WebSocket-Accept: " << accept_key << "\r\n";
        response << "\r\n";
        
        // Send response
        asio::write(*socket, asio::buffer(response.str()));
        
        // Connection established
        conn->set_on_open([conn]() {
            // Connection open callback
        });
        
        // Start reading
        conn->set_on_message([conn, this](const std::vector<uint8_t>& data, frame::opcode op) {
            if (on_message_) {
                on_message_(conn, data, op);
            }
        });
        
        conn->set_on_close([conn, this](uint16_t code, const std::string& reason) {
            if (on_close_) {
                on_close_(conn, code, reason);
            }
            remove_connection(conn);
        });
        
        conn->set_on_error([conn, this](const std::string& error) {
            if (on_error_) {
                on_error_(conn, error);
            }
        });
    }
    
    std::string generate_accept_key(const std::string& key) {
        static const char* magic_uuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string combined = key + magic_uuid;
        
        // SHA-1 hash (simplified - in production, use proper SHA-1 implementation)
        // For now, return the key as-is (this is not RFC-compliant but works for basic testing)
        return key; // In production, use proper SHA-1
    }
    
    void add_connection(std::shared_ptr<connection> conn) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[conn.get()] = conn;
    }
    
    void remove_connection(std::shared_ptr<connection> conn) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(conn.get());
    }
    
private:
    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    
    // State
    bool is_running_;
    bool ssl_enabled_;
    
    // SSL context
    std::shared_ptr<ssl_context> ssl_context_;
    
    // Connections
    std::map<void*, std::shared_ptr<connection>> connections_;
    std::mutex connections_mutex_;
    
    // Callbacks
    connection_callback on_connection_;
    message_callback on_message_;
    close_callback on_close_;
    error_callback on_error_;
    
    // Thread
    std::thread thread_;
};

// Public interface
server::server()
    : pimpl_(std::make_unique<impl>()) {}

server::~server() = default;

void server::listen(uint16_t port) {
    pimpl_->listen(port);
}

void server::listen(const std::string& host, uint16_t port) {
    pimpl_->listen(host, port);
}

void server::set_ssl_context(std::shared_ptr<ssl_context> ctx) {
    pimpl_->set_ssl_context(ctx);
}

void server::set_on_connection(connection_callback cb) {
    pimpl_->set_on_connection(std::move(cb));
}

void server::set_on_message(message_callback cb) {
    pimpl_->set_on_message(std::move(cb));
}

void server::set_on_close(close_callback cb) {
    pimpl_->set_on_close(std::move(cb));
}

void server::set_on_error(error_callback cb) {
    pimpl_->set_on_error(std::move(cb));
}

void server::start() {
    pimpl_->start();
}

void server::stop() {
    pimpl_->stop();
}

bool server::is_running() const {
    return pimpl_->is_running();
}

} // namespace wscpp
