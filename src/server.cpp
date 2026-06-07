#include <wscpp/server.hpp>
#include <wscpp/handshake.hpp>
#include <wscpp/detail/make_unique.hpp>
#include <asio/read_until.hpp>
#include <asio/streambuf.hpp>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace wscpp {

namespace {

std::string streambuf_to_string(const asio::streambuf& buf) {
    const asio::streambuf::const_buffers_type bufs = buf.data();
    return std::string(asio::buffers_begin(bufs), asio::buffers_end(bufs));
}

} // namespace

class server::impl {
public:
    impl()
        : io_context_(),
          acceptor_(io_context_),
          is_running_(false),
          ssl_enabled_(false) {
        ssl_context_ = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_server);
        ssl_context_->set_options(
            asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3);
    }

    ~impl() {
        stop();
    }

    void listen(uint16_t port) {
        listen("0.0.0.0", port);
    }

    void listen(const std::string& host, uint16_t port) {
        tcp::endpoint endpoint(asio::ip::make_address(host), port);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        is_running_ = true;
        start_accept();
    }

    void set_ssl_context(std::shared_ptr<ssl_context> ctx) {
        if (ctx) {
            ssl_context_ = ctx;
            ssl_enabled_ = true;
        }
    }

    void set_on_connection(connection_callback cb) { on_connection_ = cb; }
    void set_on_message(message_callback cb) { on_message_ = cb; }
    void set_on_close(close_callback cb) { on_close_ = cb; }
    void set_on_error(error_callback cb) { on_error_ = cb; }

    void start() {
        if (!is_running_) {
            throw std::runtime_error("Server not listening");
        }
        thread_ = std::thread([this]() { io_context_.run(); });
    }

    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void stop() {
        if (!is_running_) {
            return;
        }
        is_running_ = false;
        asio::error_code ec;
        if (acceptor_.is_open()) {
            acceptor_.close(ec);
        }
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (std::map<void*, std::shared_ptr<connection>>::iterator it =
                     connections_.begin();
                 it != connections_.end(); ++it) {
                it->second->close(1000, "Server shutting down");
            }
            connections_.clear();
        }
        io_context_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    bool is_running() const { return is_running_; }

private:
    void start_accept() {
        std::shared_ptr<tcp::socket> socket(new tcp::socket(io_context_));
        acceptor_.async_accept(*socket, [this, socket](const asio::error_code& ec) {
            if (!ec) {
                handle_accept(socket);
            }
            if (is_running_) {
                start_accept();
            }
        });
    }

    void handle_accept(std::shared_ptr<tcp::socket> socket) {
        std::shared_ptr<connection> conn(new connection(io_context_));
        conn->set_role(connection_role::server);

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

        conn->adopt(std::move(*socket));

        if (ssl_enabled_ && ssl_context_) {
            try {
                conn->socket().enable_ssl(ssl_context_);
                conn->socket().ssl_handshake(false);
            } catch (const std::exception& ex) {
                if (on_error_) {
                    on_error_(conn, ex.what());
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

    bool perform_handshake(std::shared_ptr<connection> conn) {
        try {
            asio::streambuf request_buf;
            conn->socket().read_until(request_buf, "\r\n\r\n");

            const std::string raw = streambuf_to_string(request_buf);
            std::string request_line;
            std::map<std::string, std::string> headers;
            if (!handshake::parse_http_headers(raw, request_line, headers)) {
                if (on_error_) {
                    on_error_(conn, "Invalid HTTP request");
                }
                return false;
            }

            if (!handshake::validate_client_request(headers)) {
                if (on_error_) {
                    on_error_(conn, "Invalid WebSocket handshake");
                }
                return false;
            }

            const std::string accept =
                handshake::compute_accept(headers["sec-websocket-key"]);
            const std::string response = handshake::build_server_response(accept);
            conn->socket().write(response.data(), response.size());
            return true;
        } catch (const std::exception& ex) {
            if (on_error_) {
                on_error_(conn, ex.what());
            }
            return false;
        }
    }

    void add_connection(std::shared_ptr<connection> conn) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[conn.get()] = conn;
    }

    void remove_connection(std::shared_ptr<connection> conn) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(conn.get());
    }

    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    bool is_running_;
    bool ssl_enabled_;
    std::shared_ptr<ssl_context> ssl_context_;
    std::map<void*, std::shared_ptr<connection>> connections_;
    std::mutex connections_mutex_;
    connection_callback on_connection_;
    message_callback on_message_;
    close_callback on_close_;
    error_callback on_error_;
    std::thread thread_;
};

server::server()
    : pimpl_(detail::make_unique<impl>()) {}

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
    pimpl_->set_on_connection(cb);
}

void server::set_on_message(message_callback cb) {
    pimpl_->set_on_message(cb);
}

void server::set_on_close(close_callback cb) {
    pimpl_->set_on_close(cb);
}

void server::set_on_error(error_callback cb) {
    pimpl_->set_on_error(cb);
}

void server::start() {
    pimpl_->start();
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
