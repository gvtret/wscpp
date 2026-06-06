#include <wscpp/connection.hpp>
#include <wscpp/frame/parser.hpp>
#include <wscpp/frame/builder.hpp>
#include <asio/buffer.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/error_code.hpp>
#include <asio/streambuf.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/connect.hpp>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>

namespace wscpp {

class connection::impl {
public:
    impl(asio::io_context& io_context)
        : socket_(io_context),
          io_context_(io_context),
          is_open_(false),
          is_closing_(false),
          expecting_close_(false) {
        // Default SSL context for client
        ssl_context_ = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
        ssl_context_->set_verify_mode(asio::ssl::verify_none);
    }
    
    ~impl() {
        close();
    }
    
    void connect(const std::string& host, const std::string& port) {
        if (is_open_) {
            throw std::runtime_error("Connection already open");
        }
        
        tcp::resolver resolver(io_context_);
        auto endpoint_iterator = resolver.resolve(host, port);
        socket_.connect(endpoint_iterator);
        
        is_open_ = true;
        
        // Start read loop
        async_read_header();
        
        // Call open callback
        if (on_open_) {
            on_open_();
        }
    }
    
    void connect(const tcp::endpoint& endpoint) {
        if (is_open_) {
            throw std::runtime_error("Connection already open");
        }
        
        socket_.connect(endpoint);
        is_open_ = true;
        
        // Start read loop
        async_read_header();
        
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
        
        // Build close frame
        frame::builder b;
        std::vector<uint8_t> close_frame = b.build_close(status_code, reason, false);
        
        // Send close frame
        socket_.write(close_frame.data(), close_frame.size());
        
        // Close socket
        socket_.close();
        is_open_ = false;
        
        // Call close callback
        if (on_close_) {
            on_close_(status_code, reason);
        }
    }
    
    void send_text(const std::string& text, bool fin) {
        if (!is_open_) {
            throw std::runtime_error("Connection not open");
        }
        
        frame::builder b;
        std::vector<uint8_t> frame = b.build_text(text, fin, false);
        socket_.write(frame.data(), frame.size());
    }
    
    void send_binary(const uint8_t* data, size_t size, bool fin) {
        if (!is_open_) {
            throw std::runtime_error("Connection not open");
        }
        
        frame::builder b;
        std::vector<uint8_t> frame = b.build_binary(data, size, fin, false);
        socket_.write(frame.data(), frame.size());
    }
    
    void set_on_open(open_callback cb) {
        on_open_ = std::move(cb);
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
    
    bool is_open() const {
        return is_open_;
    }
    
    bool is_ssl() const {
        return socket_.is_ssl();
    }
    
    socket_type& socket() {
        return socket_;
    }
    
    const socket_type& socket() const {
        return socket_;
    }
    
private:
    void async_read_header() {
        // Read 2 bytes for header
        if (!is_open_) return;
        
        size_t bytes_read = socket_.read(header_buffer_, 2);
        
        if (bytes_read == 2) {
            parse_header();
        }
    }
    
    void parse_header() {
        uint8_t first_byte = header_buffer_[0];
        uint8_t second_byte = header_buffer_[1];
        
        current_header_.fin = (first_byte & 0x80) != 0;
        current_header_.rsv1 = (first_byte & 0x40) != 0;
        current_header_.rsv2 = (first_byte & 0x20) != 0;
        current_header_.rsv3 = (first_byte & 0x10) != 0;
        current_header_.op = static_cast<frame::opcode>(first_byte & 0x0F);
        current_header_.mask = (second_byte & 0x80) != 0;
        current_header_.payload_len = second_byte & 0x7F;
        
        // Read extended payload length if needed
        if (current_header_.payload_len == 126) {
            size_t bytes_read = socket_.read(ext_payload_buffer_, 2);
            if (bytes_read == 2) {
                current_header_.payload_len = 
                    (static_cast<uint64_t>(ext_payload_buffer_[0]) << 8) | ext_payload_buffer_[1];
            }
        } else if (current_header_.payload_len == 127) {
            size_t bytes_read = socket_.read(ext_payload_buffer_, 8);
            if (bytes_read == 8) {
                current_header_.payload_len = 0;
                for (int i = 0; i < 8; ++i) {
                    current_header_.payload_len = 
                        (current_header_.payload_len << 8) | ext_payload_buffer_[i];
                }
            }
        }
        
        // Read masking key if needed
        if (current_header_.mask) {
            size_t bytes_read = socket_.read(masking_key_buffer_, 4);
            if (bytes_read == 4) {
                current_header_.masking_key = {{
                    masking_key_buffer_[0], masking_key_buffer_[1],
                    masking_key_buffer_[2], masking_key_buffer_[3]
                }};
            }
        }
        
        // Read payload
        async_read_payload();
    }
    
    void async_read_payload() {
        if (current_header_.payload_len == 0) {
            handle_frame();
            return;
        }
        
        payload_buffer_.resize(current_header_.payload_len);
        size_t bytes_read = socket_.read(payload_buffer_.data(), current_header_.payload_len);
        
        if (bytes_read == current_header_.payload_len) {
            // Unmask if needed
            if (current_header_.mask) {
                for (size_t i = 0; i < payload_buffer_.size(); ++i) {
                    payload_buffer_[i] ^= current_header_.masking_key[i % 4];
                }
            }
            handle_frame();
        }
    }
    
    void handle_frame() {
        frame::opcode op = current_header_.op;
        
        // Handle control frames
        if (op == frame::opcode::CLOSE) {
            handle_close_frame();
            return;
        } else if (op == frame::opcode::PING) {
            handle_ping_frame();
            return;
        } else if (op == frame::opcode::PONG) {
            // Ignore pong frames
            async_read_header();
            return;
        }
        
        // Handle data frames
        if (on_message_) {
            on_message_(payload_buffer_, op);
        }
        
        // Continue reading
        async_read_header();
    }
    
    void handle_close_frame() {
        expecting_close_ = true;
        uint16_t status_code = 1000;
        std::string reason;
        
        if (payload_buffer_.size() >= 2) {
            status_code = (static_cast<uint16_t>(payload_buffer_[0]) << 8) | payload_buffer_[1];
            reason = std::string(payload_buffer_.begin() + 2, payload_buffer_.end());
        }
        
        // Send close response
        frame::builder b;
        std::vector<uint8_t> close_frame = b.build_close(status_code, "", false);
        socket_.write(close_frame.data(), close_frame.size());
        
        // Close socket
        socket_.close();
        is_open_ = false;
        
        // Call close callback
        if (on_close_) {
            on_close_(status_code, reason);
        }
    }
    
    void handle_ping_frame() {
        // Send pong response
        frame::builder b;
        std::vector<uint8_t> pong_frame = b.build_pong(payload_buffer_.data(), payload_buffer_.size(), false);
        socket_.write(pong_frame.data(), pong_frame.size());
        
        // Continue reading
        async_read_header();
    }
    
private:
    socket_type socket_;
    asio::io_context& io_context_;
    
    // Header buffer
    uint8_t header_buffer_[2];
    uint8_t ext_payload_buffer_[8];
    uint8_t masking_key_buffer_[4];
    
    // Current frame
    frame::frame_header current_header_;
    std::vector<uint8_t> payload_buffer_;
    
    // State
    bool is_open_;
    bool is_closing_;
    bool expecting_close_;
    
    // SSL context
    std::shared_ptr<asio::ssl::context> ssl_context_;
    
    // Callbacks
    open_callback on_open_;
    message_callback on_message_;
    close_callback on_close_;
    error_callback on_error_;
};

// Public interface
connection::connection(asio::io_context& io_context)
    : pimpl_(std::make_unique<impl>(io_context)) {}

connection::~connection() = default;

void connection::connect(const std::string& host, const std::string& port) {
    pimpl_->connect(host, port);
}

void connection::connect(const tcp::endpoint& endpoint) {
    pimpl_->connect(endpoint);
}

void connection::close(uint16_t status_code, const std::string& reason) {
    pimpl_->close(status_code, reason);
}

void connection::send_text(const std::string& text, bool fin) {
    pimpl_->send_text(text, fin);
}

void connection::send_binary(const uint8_t* data, size_t size, bool fin) {
    pimpl_->send_binary(data, size, fin);
}

void connection::set_on_open(open_callback cb) {
    pimpl_->set_on_open(std::move(cb));
}

void connection::set_on_message(message_callback cb) {
    pimpl_->set_on_message(std::move(cb));
}

void connection::set_on_close(close_callback cb) {
    pimpl_->set_on_close(std::move(cb));
}

void connection::set_on_error(error_callback cb) {
    pimpl_->set_on_error(std::move(cb));
}

bool connection::is_open() const {
    return pimpl_->is_open();
}

bool connection::is_ssl() const {
    return pimpl_->is_ssl();
}

connection::socket_type& connection::socket() {
    return pimpl_->socket();
}

const connection::socket_type& connection::socket() const {
    return pimpl_->socket();
}

} // namespace wscpp
