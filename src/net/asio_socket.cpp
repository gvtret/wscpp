#include <asio/connect.hpp>
#include <asio/error_code.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <openssl/ssl.h>
#include <wscpp/detail/make_unique.hpp>
#include <wscpp/error.hpp>
#include <wscpp/net/asio_socket.hpp>

namespace wscpp {
namespace net {

class asio_socket::impl {
  public:
    explicit impl(asio::io_context &io_context)
        : io_context_(io_context), socket_(io_context), ssl_(nullptr), ssl_enabled_(false) {}

    std::error_code connect(const std::string &host, const std::string &port) {
        asio::error_code ec;
        tcp::resolver resolver(io_context_);
        const tcp::resolver::results_type endpoints = resolver.resolve(host, port, ec);
        if (ec) {
            return ec;
        }
        asio::connect(socket_, endpoints, ec);
        return ec;
    }

    std::error_code connect(const tcp::endpoint &endpoint) {
        asio::error_code ec;
        socket_.connect(endpoint, ec);
        return ec;
    }

    std::error_code adopt(tcp::socket socket) {
        socket_ = std::move(socket);
        return std::error_code();
    }

    void close() {
        asio::error_code ec;
        if (ssl_) {
            ssl_->shutdown(ec);
        }
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    std::size_t write(const void *data, std::size_t size, std::error_code &ec) {
        ec.clear();
        std::size_t n = 0;
        if (ssl_) {
            n = asio::write(*ssl_, asio::buffer(data, size), ec);
        } else {
            n = asio::write(socket_, asio::buffer(data, size), ec);
        }
        return n;
    }

    std::size_t read(void *data, std::size_t size, std::error_code &ec) {
        ec.clear();
        std::size_t n = 0;
        if (ssl_) {
            n = asio::read(*ssl_, asio::buffer(data, size), ec);
        } else {
            n = asio::read(socket_, asio::buffer(data, size), ec);
        }
        return n;
    }

    std::size_t read_some(void *data, std::size_t size, std::error_code &ec) {
        ec.clear();
        std::size_t n = 0;
        if (ssl_) {
            n = ssl_->read_some(asio::buffer(data, size), ec);
        } else {
            n = socket_.read_some(asio::buffer(data, size), ec);
        }
        if (ec == asio::error::would_block) {
            ec.clear();
        }
        return n;
    }

    std::size_t read_until(asio::streambuf &buf, const std::string &delim, std::error_code &ec) {
        ec.clear();
        std::size_t n = 0;
        if (ssl_) {
            n = asio::read_until(*ssl_, buf, delim, ec);
        } else {
            n = asio::read_until(socket_, buf, delim, ec);
        }
        return n;
    }

    tcp::socket &native_socket() {
        return socket_;
    }
    const tcp::socket &native_socket() const {
        return socket_;
    }

    bool is_open() const {
        return socket_.is_open();
    }

    std::error_code enable_ssl(std::shared_ptr<ssl_context> ctx) {
        if (!ctx) {
            return std::error_code();
        }
        ssl_context_ = ctx;
        ssl_.reset(new asio::ssl::stream<tcp::socket &>(socket_, *ctx));
        ssl_enabled_ = true;
        return std::error_code();
    }

    std::error_code set_ssl_hostname(const std::string &host) {
        if (!ssl_) {
            return make_error_code(errc::ssl_not_enabled);
        }
        if (SSL_set_tlsext_host_name(ssl_->native_handle(), host.c_str()) != 1) {
            return make_error_code(errc::ssl_io_error);
        }
        return std::error_code();
    }

    std::error_code ssl_handshake(bool as_client) {
        if (!ssl_) {
            return make_error_code(errc::ssl_not_enabled);
        }
        asio::error_code ec;
        if (as_client) {
            ssl_->handshake(asio::ssl::stream_base::client, ec);
        } else {
            ssl_->handshake(asio::ssl::stream_base::server, ec);
        }
        return ec;
    }

    bool is_ssl() const {
        return ssl_enabled_;
    }

  private:
    asio::io_context &io_context_;
    tcp::socket socket_;
    std::shared_ptr<ssl_context> ssl_context_;
    std::unique_ptr<asio::ssl::stream<tcp::socket &>> ssl_;
    bool ssl_enabled_;
};

asio_socket::asio_socket(asio::io_context &io_context)
    : pimpl_(detail::make_unique<impl>(io_context)) {}

asio_socket::~asio_socket() = default;

std::error_code asio_socket::connect(const std::string &host, const std::string &port) {
    return pimpl_->connect(host, port);
}

std::error_code asio_socket::connect(const tcp::endpoint &endpoint) {
    return pimpl_->connect(endpoint);
}

std::error_code asio_socket::adopt(tcp::socket socket) {
    return pimpl_->adopt(std::move(socket));
}

void asio_socket::close() {
    pimpl_->close();
}

std::size_t asio_socket::write(const void *data, std::size_t size, std::error_code &ec) {
    return pimpl_->write(data, size, ec);
}

std::size_t asio_socket::read(void *data, std::size_t size, std::error_code &ec) {
    return pimpl_->read(data, size, ec);
}

std::size_t asio_socket::read_some(void *data, std::size_t size, std::error_code &ec) {
    return pimpl_->read_some(data, size, ec);
}

std::size_t asio_socket::read_until(asio::streambuf &buf, const std::string &delim,
                                    std::error_code &ec) {
    return pimpl_->read_until(buf, delim, ec);
}

asio_socket::tcp::socket &asio_socket::native_socket() {
    return pimpl_->native_socket();
}

const asio_socket::tcp::socket &asio_socket::native_socket() const {
    return pimpl_->native_socket();
}

bool asio_socket::is_open() const {
    return pimpl_->is_open();
}

std::error_code asio_socket::enable_ssl(std::shared_ptr<ssl_context> ctx) {
    return pimpl_->enable_ssl(ctx);
}

std::error_code asio_socket::set_ssl_hostname(const std::string &host) {
    return pimpl_->set_ssl_hostname(host);
}

std::error_code asio_socket::ssl_handshake(bool as_client) {
    return pimpl_->ssl_handshake(as_client);
}

bool asio_socket::is_ssl() const {
    return pimpl_->is_ssl();
}

} // namespace net
} // namespace wscpp
