#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wscpp/detail/make_unique.hpp>
#include <wscpp/error.hpp>
#include <wscpp/net/linux_socket.hpp>

namespace wscpp {
namespace net {

namespace {

void close_fd(int &fd) {
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
        fd = -1;
    }
}

std::error_code connect_host(const std::string &host, const std::string &port, int &out_fd) {
    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *result = nullptr;
    const int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (rc != 0) {
        return make_error_code(errc::host_not_found);
    }

    int fd = -1;
    std::error_code last_ec = make_error_code(errc::host_not_found);
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
        fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            last_ec = std::error_code(errno, std::system_category());
            continue;
        }
        if (::connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            out_fd = fd;
            freeaddrinfo(result);
            return std::error_code();
        }
        last_ec = std::error_code(errno, std::system_category());
        close_fd(fd);
    }
    freeaddrinfo(result);
    return last_ec;
}

} // namespace

linux_socket::linux_socket(io_context &) : fd_(-1), ssl_(nullptr), ssl_enabled_(false) {}

linux_socket::~linux_socket() {
    close();
}

std::error_code linux_socket::connect(const std::string &host, const std::string &port) {
    if (is_open()) {
        close();
    }
    return connect_host(host, port, fd_);
}

std::error_code linux_socket::connect(const tcp_endpoint &endpoint) {
    return connect(endpoint.address, std::to_string(endpoint.port));
}

std::error_code linux_socket::adopt(int fd) {
    if (is_open()) {
        close();
    }
    if (fd < 0) {
        return std::error_code(errno, std::system_category());
    }
    fd_ = fd;
    return std::error_code();
}

void linux_socket::close() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    ssl_enabled_ = false;
    ssl_context_.reset();
    close_fd(fd_);
}

std::size_t linux_socket::write(const void *data, std::size_t size, std::error_code &ec) {
    ec.clear();
    if (!is_open()) {
        ec = make_error_code(errc::not_connected);
        return 0;
    }

    const char *ptr = static_cast<const char *>(data);
    std::size_t total = 0;
    while (total < size) {
        int n = 0;
        if (ssl_) {
            n = SSL_write(ssl_, ptr + total, static_cast<int>(size - total));
            if (n <= 0) {
                ec = ssl_error(n);
                return total;
            }
        } else {
            n = static_cast<int>(::send(fd_, ptr + total, size - total, MSG_NOSIGNAL));
            if (n < 0) {
                ec = std::error_code(errno, std::system_category());
                return total;
            }
            if (n == 0) {
                ec = std::make_error_code(std::errc::broken_pipe);
                return total;
            }
        }
        total += static_cast<std::size_t>(n);
    }
    return total;
}

std::size_t linux_socket::read_raw(void *data, std::size_t size, bool exact, std::error_code &ec) {
    ec.clear();
    char *ptr = static_cast<char *>(data);
    std::size_t total = 0;
    while (total < size) {
        int n = 0;
        if (ssl_) {
            n = SSL_read(ssl_, ptr + total, static_cast<int>(size - total));
            if (n <= 0) {
                ec = ssl_error(n);
                return total;
            }
        } else {
            n = static_cast<int>(::recv(fd_, ptr + total, size - total, 0));
            if (n < 0) {
                ec = std::error_code(errno, std::system_category());
                return total;
            }
            if (n == 0) {
                ec = std::make_error_code(std::errc::connection_reset);
                return total;
            }
        }
        total += static_cast<std::size_t>(n);
        if (!exact) {
            break;
        }
    }
    return total;
}

std::size_t linux_socket::read(void *data, std::size_t size, std::error_code &ec) {
    if (!is_open()) {
        ec = make_error_code(errc::not_connected);
        return 0;
    }
    return read_raw(data, size, true, ec);
}

std::size_t linux_socket::read_some_raw(void *data, std::size_t size, std::error_code &ec) {
    if (!is_open()) {
        ec = make_error_code(errc::not_connected);
        return 0;
    }
    return read_raw(data, size, false, ec);
}

std::size_t linux_socket::read_some(void *data, std::size_t size, std::error_code &ec) {
    return read_some_raw(data, size, ec);
}

std::size_t linux_socket::read_until(stream_buffer &buf, const std::string &delim,
                                     std::error_code &ec) {
    ec.clear();
    if (!is_open()) {
        ec = make_error_code(errc::not_connected);
        return 0;
    }
    if (delim.empty()) {
        return 0;
    }

    const std::size_t start_size = buf.size();
    char byte = 0;
    while (true) {
        read_raw(&byte, 1, true, ec);
        if (ec) {
            return buf.size() - start_size;
        }
        buf.append(byte);

        const std::string current = buf.to_string();
        if (current.size() >= delim.size() &&
            current.compare(current.size() - delim.size(), delim.size(), delim) == 0) {
            break;
        }
    }
    return buf.size() - start_size;
}

int linux_socket::native_fd() const {
    return fd_;
}

bool linux_socket::is_open() const {
    return fd_ >= 0;
}

std::error_code linux_socket::enable_ssl(const std::shared_ptr<ssl_context> &ctx) {
    if (!ctx) {
        return std::error_code();
    }
    if (!is_open()) {
        return make_error_code(errc::not_connected);
    }
    ssl_context_ = ctx;
    ssl_ = SSL_new(ctx->native());
    if (!ssl_) {
        return make_error_code(errc::ssl_init_failed);
    }
    if (SSL_set_fd(ssl_, fd_) != 1) {
        SSL_free(ssl_);
        ssl_ = nullptr;
        return make_error_code(errc::ssl_init_failed);
    }
    ssl_enabled_ = true;
    return std::error_code();
}

std::error_code linux_socket::set_ssl_hostname(const std::string &host) {
    if (!ssl_) {
        return make_error_code(errc::ssl_not_enabled);
    }
    if (SSL_set_tlsext_host_name(ssl_, host.c_str()) != 1) {
        return make_error_code(errc::ssl_io_error);
    }
    return std::error_code();
}

std::error_code linux_socket::ssl_handshake(bool as_client) {
    if (!ssl_) {
        return make_error_code(errc::ssl_not_enabled);
    }
    const int rc = as_client ? SSL_connect(ssl_) : SSL_accept(ssl_);
    if (rc != 1) {
        return ssl_error(rc);
    }
    return std::error_code();
}

bool linux_socket::is_ssl() const {
    return ssl_enabled_;
}

std::error_code linux_socket::ssl_error(int result) {
    const int err = SSL_get_error(ssl_, result);
    if (err == SSL_ERROR_ZERO_RETURN) {
        return std::make_error_code(std::errc::connection_reset);
    }
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        return make_error_code(errc::ssl_would_block);
    }
    return make_error_code(errc::ssl_io_error);
}

} // namespace net
} // namespace wscpp
