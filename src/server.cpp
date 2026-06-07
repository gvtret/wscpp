#include <utility>
#include <wscpp/detail/log.hpp>
#include <wscpp/detail/make_unique.hpp>
#include <wscpp/error.hpp>
#include <wscpp/handshake.hpp>
#include <wscpp/server.hpp>
#if WSCPP_ENABLE_DEFLATE
#include <wscpp/extensions/permessage_deflate.hpp>
#endif
#if !WSCPP_USE_ASIO
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace wscpp {

namespace {

#if !WSCPP_USE_ASIO

class linux_acceptor {
  public:
    linux_acceptor() : fd_(-1) {}

    ~linux_acceptor() {
        close();
    }

    std::error_code listen_on(const std::string &host, std::uint16_t port) {
        close();
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            return std::error_code(errno, std::system_category());
        }

        const int yes = 1;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0) {
            const std::error_code ec(errno, std::system_category());
            close();
            return ec;
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (host == "0.0.0.0" || host.empty()) {
            addr.sin_addr.s_addr = INADDR_ANY;
        } else if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
            close();
            return make_error_code(errc::invalid_address);
        }

        if (::bind(fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
            const std::error_code ec(errno, std::system_category());
            close();
            return ec;
        }
        if (::listen(fd_, SOMAXCONN) != 0) {
            const std::error_code ec(errno, std::system_category());
            close();
            return ec;
        }
        return std::error_code();
    }

    std::error_code accept_one(int &client_fd) {
        client_fd = ::accept(fd_, nullptr, nullptr);
        if (client_fd < 0) {
            return std::error_code(errno, std::system_category());
        }
        return std::error_code();
    }

    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    bool is_open() const {
        return fd_ >= 0;
    }

    int native_fd() const {
        return fd_;
    }

  private:
    int fd_;
};

#endif

} // namespace

class server::impl {
  public:
    impl()
#if WSCPP_USE_ASIO
        : acceptor_(io_context_), is_running_(false), ssl_enabled_(false) {
#else
        : is_running_(false), ssl_enabled_(false) {
#endif
#if WSCPP_USE_ASIO
        ssl_context_ = std::make_shared<net::ssl_context>(asio::ssl::context::tlsv12_server);
        ssl_context_->set_options(asio::ssl::context::default_workarounds |
                                  asio::ssl::context::no_sslv2 | asio::ssl::context::no_sslv3);
#else
        net::ssl_context::make(net::ssl_context::role::server, ssl_context_);
#endif
    }

    ~impl() {
        stop();
    }

    std::error_code listen(uint16_t port) {
        return listen("0.0.0.0", port);
    }

    std::error_code listen(const std::string &host, uint16_t port) {
#if WSCPP_USE_ASIO
        asio::error_code ec;
        const asio::ip::address addr = asio::ip::make_address(host, ec);
        if (ec) {
            return ec;
        }
        const net::tcp::endpoint endpoint(addr, port);
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            return ec;
        }
        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            return ec;
        }
        acceptor_.bind(endpoint, ec);
        if (ec) {
            return ec;
        }
        acceptor_.listen(asio::socket_base::max_connections, ec);
        if (ec) {
            return ec;
        }
#else
        const std::error_code ec = acceptor_.listen_on(host, port);
        if (ec) {
            return ec;
        }
#endif
        is_running_.store(true);
#if WSCPP_USE_ASIO
        start_accept();
#endif
        return std::error_code();
    }

    void set_ssl_context(std::shared_ptr<ssl_context> ctx) {
        if (ctx) {
            ssl_context_ = std::move(ctx);
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

    std::error_code start() {
        if (!is_running_.load()) {
            return make_error_code(errc::server_not_listening);
        }
#if WSCPP_USE_ASIO
        thread_ = std::thread([this]() { io_context_.run(); });
#else
        thread_ = std::thread([this]() { accept_loop(); });
#endif
        return std::error_code();
    }

    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void stop() {
        if (!is_running_.exchange(false)) {
            return;
        }
#if WSCPP_USE_ASIO
        asio::error_code ec;
        if (acceptor_.is_open()) {
            acceptor_.close(ec);
            detail::log_error_ec("server", "acceptor close", ec);
        }
        io_context_.stop();
#else
        acceptor_.close();
#endif
        shutdown_all_sockets();
#if !WSCPP_USE_ASIO
        if (thread_.joinable()) {
            thread_.join();
        }
        join_workers();
#endif
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.clear();
        }
#if WSCPP_USE_ASIO
        if (thread_.joinable()) {
            thread_.join();
        }
#endif
    }

    bool is_running() const {
        return is_running_.load();
    }

  private:
#if WSCPP_USE_ASIO
    void start_accept() {
        std::shared_ptr<net::tcp::socket> socket(new net::tcp::socket(io_context_));
        acceptor_.async_accept(*socket, [this, socket](const asio::error_code &ec) {
            if (ec) {
                detail::log_error_ec("server", "async accept", ec);
            } else {
                handle_accept(std::move(*socket));
            }
            if (is_running_.load()) {
                start_accept();
            }
        });
    }

    void handle_accept(net::tcp::socket socket) {
#else
    void accept_loop() {
        while (is_running_.load()) {
            if (!acceptor_.is_open()) {
                break;
            }

            pollfd pfd;
            pfd.fd = acceptor_.native_fd();
            pfd.events = POLLIN;
            pfd.revents = 0;
            const int poll_rc = ::poll(&pfd, 1, 200);
            if (!is_running_.load()) {
                break;
            }
            if (poll_rc <= 0) {
                continue;
            }
            if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                break;
            }

            int client_fd = -1;
            const std::error_code ec = acceptor_.accept_one(client_fd);
            if (ec) {
                if (!is_running_.load()) {
                    break;
                }
                if (ec == std::errc::operation_canceled || ec == std::errc::bad_file_descriptor ||
                    ec == std::errc::invalid_argument) {
                    break;
                }
                detail::log_error_ec("server", "accept", ec);
                continue;
            }
            std::thread worker([this, client_fd]() { handle_accept(net::tcp_socket(client_fd)); });
            std::lock_guard<std::mutex> lock(workers_mutex_);
            workers_.push_back(std::move(worker));
        }
    }

    void handle_accept(net::tcp_socket socket) {
#endif
        std::shared_ptr<connection> conn(new connection(io_context_));
        register_pending(conn);
        struct pending_guard {
            impl *self;
            connection *conn;
            ~pending_guard() {
                self->unregister_pending(conn);
            }
        } guard{this, conn.get()};

        conn->set_role(connection_role::server);

        conn->set_on_message([this, conn](const std::vector<uint8_t> &data, frame::opcode op) {
            if (on_message_) {
                on_message_(conn, data, op);
            }
        });

        conn->set_on_close([this, conn](uint16_t code, const std::string &reason) {
            if (on_close_) {
                on_close_(conn, code, reason);
            }
            remove_connection(conn);
        });

        conn->set_on_error([this, conn](const std::string &error) {
            if (on_error_) {
                on_error_(conn, error);
            }
        });

        const std::error_code adopt_ec = conn->adopt(std::move(socket));
        if (adopt_ec) {
            detail::log_error_ec("server", "adopt socket", adopt_ec);
            if (on_error_) {
                on_error_(conn, adopt_ec.message());
            }
            return;
        }

        if (ssl_enabled_ && ssl_context_) {
            std::error_code ec = conn->socket().enable_ssl(ssl_context_);
            if (!ec) {
                ec = conn->socket().ssl_handshake(false);
            }
            if (ec) {
                detail::log_error_ec("server", "TLS handshake", ec);
                if (on_error_) {
                    on_error_(conn, ec.message());
                }
                return;
            }
        }

        if (!perform_handshake(conn)) {
            return;
        }

        if (on_connection_) {
            on_connection_(conn);
        }
        conn->activate();
        add_connection(conn);
    }

    bool perform_handshake(const std::shared_ptr<connection> &conn) {
        net::stream_buffer request_buf;
        std::error_code ec;
        conn->socket().read_until(request_buf, "\r\n\r\n", ec);
        if (ec) {
            detail::log_error_ec("server", "read HTTP request", ec);
            if (on_error_) {
                on_error_(conn, ec.message());
            }
            return false;
        }

        const std::string raw = net::stream_buffer_to_string(request_buf);
        std::string request_line;
        std::map<std::string, std::string> headers;
        if (!handshake::parse_http_headers(raw, request_line, headers)) {
            detail::log_error("server", "Invalid HTTP request");
            if (on_error_) {
                on_error_(conn, "Invalid HTTP request");
            }
            return false;
        }

        if (!handshake::validate_client_request(headers)) {
            detail::log_error("server", "Invalid WebSocket handshake");
            if (on_error_) {
                on_error_(conn, "Invalid WebSocket handshake");
            }
            return false;
        }

        const std::string accept = handshake::compute_accept(headers["sec-websocket-key"]);
#if WSCPP_ENABLE_DEFLATE
        std::string extensions;
        const std::map<std::string, std::string>::const_iterator ext =
            headers.find("sec-websocket-extensions");
        if (ext != headers.end() && extensions::header_offers_permessage_deflate(ext->second)) {
            conn->set_permessage_deflate(true);
            extensions = extensions::permessage_deflate_offer();
        }
        const std::string response = handshake::build_server_response(accept, extensions);
#else
        const std::string response = handshake::build_server_response(accept);
#endif
        conn->socket().write(response.data(), response.size(), ec);
        if (ec) {
            detail::log_error_ec("server", "write handshake response", ec);
            if (on_error_) {
                on_error_(conn, ec.message());
            }
            return false;
        }
        return true;
    }

    void add_connection(const std::shared_ptr<connection> &conn) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[conn.get()] = conn;
    }

    void remove_connection(const std::shared_ptr<connection> &conn) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(conn.get());
    }

    void register_pending(const std::shared_ptr<connection> &conn) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_handshakes_.push_back(conn);
    }

    void unregister_pending(connection *conn) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_handshakes_.erase(std::remove_if(pending_handshakes_.begin(),
                                                 pending_handshakes_.end(),
                                                 [conn](const std::shared_ptr<connection> &c) {
                                                     return c.get() == conn;
                                                 }),
                                  pending_handshakes_.end());
    }

    void shutdown_all_sockets() {
        std::vector<std::shared_ptr<connection>> pending;
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending.swap(pending_handshakes_);
        }
        for (std::vector<std::shared_ptr<connection>>::iterator it = pending.begin();
             it != pending.end(); ++it) {
            (*it)->socket().close();
        }

        std::vector<std::shared_ptr<connection>> active;
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (std::map<void *, std::shared_ptr<connection>>::iterator it = connections_.begin();
                 it != connections_.end(); ++it) {
                active.push_back(it->second);
            }
        }
        for (std::vector<std::shared_ptr<connection>>::iterator it = active.begin();
             it != active.end(); ++it) {
            (*it)->socket().close();
        }
    }

#if !WSCPP_USE_ASIO
    void join_workers() {
        std::vector<std::thread> workers;
        {
            std::lock_guard<std::mutex> lock(workers_mutex_);
            workers.swap(workers_);
        }
        for (std::vector<std::thread>::iterator it = workers.begin(); it != workers.end(); ++it) {
            if (it->joinable()) {
                it->join();
            }
        }
    }
#endif

    net::io_context io_context_;
#if WSCPP_USE_ASIO
    net::tcp::acceptor acceptor_;
#else
    linux_acceptor acceptor_;
#endif
    std::atomic<bool> is_running_;
    bool ssl_enabled_;
    std::shared_ptr<ssl_context> ssl_context_;
    std::map<void *, std::shared_ptr<connection>> connections_;
    std::mutex connections_mutex_;
    std::vector<std::shared_ptr<connection>> pending_handshakes_;
    std::mutex pending_mutex_;
#if !WSCPP_USE_ASIO
    std::vector<std::thread> workers_;
    std::mutex workers_mutex_;
#endif
    connection_callback on_connection_;
    message_callback on_message_;
    close_callback on_close_;
    error_callback on_error_;
    std::thread thread_;
};

server::server() : pimpl_(detail::make_unique<impl>()) {}

server::~server() = default;

std::error_code server::listen(uint16_t port) {
    return pimpl_->listen(port);
}

std::error_code server::listen(const std::string &host, uint16_t port) {
    return pimpl_->listen(host, port);
}

void server::set_ssl_context(std::shared_ptr<ssl_context> ctx) {
    pimpl_->set_ssl_context(std::move(ctx));
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

std::error_code server::start() {
    return pimpl_->start();
}

void server::join() {
    pimpl_->join();
}

void server::stop() {
    pimpl_->stop();
}

bool server::is_running() const {
    return pimpl_->is_running();
}

} // namespace wscpp
