#include <wscpp/net/asio_socket.hpp>
#include <wscpp/detail/make_unique.hpp>
#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <asio/error_code.hpp>
#include <openssl/ssl.h>
#include <stdexcept>

namespace wscpp {
namespace net {

class asio_socket::impl {
public:
    explicit impl(asio::io_context& io_context)
        : io_context_(io_context),
          socket_(io_context),
          ssl_(nullptr),
          ssl_enabled_(false) {}

    void connect(const std::string& host, const std::string& port) {
        tcp::resolver resolver(io_context_);
        tcp::resolver::results_type endpoints = resolver.resolve(host, port);
        asio::connect(socket_, endpoints);
    }

    void connect(const tcp::endpoint& endpoint) {
        socket_.connect(endpoint);
    }

    void adopt(tcp::socket socket) {
        socket_ = std::move(socket);
    }

    void close() {
        asio::error_code ec;
        if (ssl_) {
            ssl_->shutdown(ec);
        }
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    std::size_t write(const void* data, std::size_t size) {
        asio::error_code ec;
        std::size_t n = 0;
        if (ssl_) {
            n = asio::write(*ssl_, asio::buffer(data, size), ec);
        } else {
            n = asio::write(socket_, asio::buffer(data, size), ec);
        }
        if (ec) {
            throw std::system_error(ec);
        }
        return n;
    }

    std::size_t read(void* data, std::size_t size) {
        asio::error_code ec;
        std::size_t n = 0;
        if (ssl_) {
            n = asio::read(*ssl_, asio::buffer(data, size), ec);
        } else {
            n = asio::read(socket_, asio::buffer(data, size), ec);
        }
        if (ec) {
            throw std::system_error(ec);
        }
        return n;
    }

    std::size_t read_some(void* data, std::size_t size) {
        asio::error_code ec;
        std::size_t n = 0;
        if (ssl_) {
            n = ssl_->read_some(asio::buffer(data, size), ec);
        } else {
            n = socket_.read_some(asio::buffer(data, size), ec);
        }
        if (ec && ec != asio::error::would_block) {
            throw std::system_error(ec);
        }
        return n;
    }

    std::size_t read_until(asio::streambuf& buf, const std::string& delim) {
        asio::error_code ec;
        std::size_t n = 0;
        if (ssl_) {
            n = asio::read_until(*ssl_, buf, delim, ec);
        } else {
            n = asio::read_until(socket_, buf, delim, ec);
        }
        if (ec) {
            throw std::system_error(ec);
        }
        return n;
    }

    tcp::socket& native_socket() { return socket_; }
    const tcp::socket& native_socket() const { return socket_; }

    bool is_open() const { return socket_.is_open(); }

    void enable_ssl(std::shared_ptr<ssl_context> ctx) {
        if (!ctx) {
            return;
        }
        ssl_context_ = ctx;
        ssl_.reset(new asio::ssl::stream<tcp::socket&>(socket_, *ctx));
        ssl_enabled_ = true;
    }

    void set_ssl_hostname(const std::string& host) {
        if (!ssl_) {
            throw std::runtime_error("SSL not enabled");
        }
        if (SSL_set_tlsext_host_name(ssl_->native_handle(), host.c_str()) != 1) {
            throw std::runtime_error("Failed to set TLS SNI hostname");
        }
    }

    void ssl_handshake(bool as_client) {
        if (!ssl_) {
            throw std::runtime_error("SSL not enabled");
        }
        asio::error_code ec;
        if (as_client) {
            ssl_->handshake(asio::ssl::stream_base::client, ec);
        } else {
            ssl_->handshake(asio::ssl::stream_base::server, ec);
        }
        if (ec) {
            throw std::system_error(ec);
        }
    }

    bool is_ssl() const { return ssl_enabled_; }

private:
    asio::io_context& io_context_;
    tcp::socket socket_;
    std::shared_ptr<ssl_context> ssl_context_;
    std::unique_ptr<asio::ssl::stream<tcp::socket&>> ssl_;
    bool ssl_enabled_;
};

asio_socket::asio_socket(asio::io_context& io_context)
    : pimpl_(detail::make_unique<impl>(io_context)) {}

asio_socket::~asio_socket() = default;

void asio_socket::connect(const std::string& host, const std::string& port) {
    pimpl_->connect(host, port);
}

void asio_socket::connect(const tcp::endpoint& endpoint) {
    pimpl_->connect(endpoint);
}

void asio_socket::adopt(tcp::socket socket) {
    pimpl_->adopt(std::move(socket));
}

void asio_socket::close() {
    pimpl_->close();
}

std::size_t asio_socket::write(const void* data, std::size_t size) {
    return pimpl_->write(data, size);
}

std::size_t asio_socket::read(void* data, std::size_t size) {
    return pimpl_->read(data, size);
}

std::size_t asio_socket::read_some(void* data, std::size_t size) {
    return pimpl_->read_some(data, size);
}

std::size_t asio_socket::read_until(asio::streambuf& buf, const std::string& delim) {
    return pimpl_->read_until(buf, delim);
}

asio_socket::tcp::socket& asio_socket::native_socket() {
    return pimpl_->native_socket();
}

const asio_socket::tcp::socket& asio_socket::native_socket() const {
    return pimpl_->native_socket();
}

bool asio_socket::is_open() const {
    return pimpl_->is_open();
}

void asio_socket::enable_ssl(std::shared_ptr<ssl_context> ctx) {
    pimpl_->enable_ssl(ctx);
}

void asio_socket::set_ssl_hostname(const std::string& host) {
    pimpl_->set_ssl_hostname(host);
}

void asio_socket::ssl_handshake(bool as_client) {
    pimpl_->ssl_handshake(as_client);
}

bool asio_socket::is_ssl() const {
    return pimpl_->is_ssl();
}

} // namespace net
} // namespace wscpp
