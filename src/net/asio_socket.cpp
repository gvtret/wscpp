#include <wscpp/net/asio_socket.hpp>
#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/buffer.hpp>
#include <asio/error_code.hpp>
#include <stdexcept>

class wscpp::net::asio_socket::impl {
public:
    impl(asio::io_context& io_context)
        : tcp_socket_(io_context),
          io_context_(io_context),
          ssl_stream_(nullptr),
          is_ssl_(false) {}
    
    ~impl() {
        close();
    }
    
    void connect(const std::string& host, const std::string& port) {
        if (is_open()) {
            close();
        }
        
        tcp::resolver resolver(io_context_);
        auto endpoint_iterator = resolver.resolve(host, port);
        asio::connect(tcp_socket_, endpoint_iterator);
    }
    
    void connect(const tcp::endpoint& endpoint) {
        if (is_open()) {
            close();
        }
        
        tcp_socket_.connect(endpoint);
    }
    
    void close() {
        error_code ec;
        tcp_socket_.shutdown(tcp::socket::shutdown_both, ec);
        tcp_socket_.close(ec);
    }
    
    size_t write(const void* data, size_t size) {
        if (is_ssl_ && ssl_stream_) {
            return asio::write(*ssl_stream_, asio::buffer(data, size), ec_);
        } else {
            return asio::write(tcp_socket_, asio::buffer(data, size), ec_);
        }
    }
    
    size_t read(void* data, size_t size) {
        if (is_ssl_ && ssl_stream_) {
            return asio::read(*ssl_stream_, asio::buffer(data, size), ec_);
        } else {
            return asio::read(tcp_socket_, asio::buffer(data, size), ec_);
        }
    }
    
    bool is_open() const {
        return tcp_socket_.is_open();
    }
    
    void set_ssl_context(std::shared_ptr<ssl_context> ctx) {
        if (ctx) {
            ssl_stream_ = std::make_unique<asio::ssl::stream<tcp::socket&>>(tcp_socket_, *ctx);
            is_ssl_ = true;
        }
    }
    
    bool is_ssl() const {
        return is_ssl_;
    }
    
private:
    tcp::socket tcp_socket_;
    asio::io_context& io_context_;
    std::unique_ptr<asio::ssl::stream<tcp::socket&>> ssl_stream_;
    bool is_ssl_;
    error_code ec_;
};

// Public interface
wscpp::net::asio_socket::asio_socket()
    : pimpl_(std::make_unique<impl>(asio::io_context())) {}

wscpp::net::asio_socket::asio_socket(asio::io_context& io_context)
    : pimpl_(std::make_unique<impl>(io_context)) {}

wscpp::net::asio_socket::~asio_socket() = default;

void wscpp::net::asio_socket::connect(const std::string& host, const std::string& port) {
    pimpl_->connect(host, port);
}

void wscpp::net::asio_socket::connect(const tcp::endpoint& endpoint) {
    pimpl_->connect(endpoint);
}

void wscpp::net::asio_socket::close() {
    pimpl_->close();
}

size_t wscpp::net::asio_socket::write(const void* data, size_t size) {
    return pimpl_->write(data, size);
}

size_t wscpp::net::asio_socket::read(void* data, size_t size) {
    return pimpl_->read(data, size);
}

bool wscpp::net::asio_socket::is_open() const {
    return pimpl_->is_open();
}

void wscpp::net::asio_socket::set_ssl_context(std::shared_ptr<ssl_context> ctx) {
    pimpl_->set_ssl_context(ctx);
}

bool wscpp::net::asio_socket::is_ssl() const {
    return pimpl_->is_ssl();
}
