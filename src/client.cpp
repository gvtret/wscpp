#include <utility>
#include <wscpp/client.hpp>
#include <wscpp/detail/make_unique.hpp>
#include <wscpp/error.hpp>
#include <wscpp/handshake.hpp>
#if WSCPP_ENABLE_DEFLATE
#include <wscpp/extensions/permessage_deflate.hpp>
#endif
#include <map>
#include <sstream>

namespace wscpp {

class client::impl {
  public:
    impl()
        : io_context_(), connection_(io_context_), is_open_(false), is_closing_(false)
#if WSCPP_ENABLE_DEFLATE
          ,
          request_deflate_(false)
#endif
    {
        connection_.set_role(connection_role::client);
    }

    ~impl() {
        stop();
    }

    std::error_code connect(const std::string &url) {
        const url_info info = parse_url(url);
        std::error_code ec;

        if (info.secure) {
            ec = ensure_ssl_context();
            if (ec) {
                return ec;
            }
            ec = connection_.socket().connect(info.host, info.port);
            if (ec) {
                return ec;
            }
            ec = connection_.socket().enable_ssl(ssl_context_);
            if (ec) {
                return ec;
            }
            ec = connection_.socket().set_ssl_hostname(info.host);
            if (ec) {
                return ec;
            }
            ec = connection_.socket().ssl_handshake(true);
            if (ec) {
                return ec;
            }
        } else {
            ec = connection_.socket().connect(info.host, info.port);
            if (ec) {
                return ec;
            }
        }

        ec = perform_handshake(info);
        if (ec) {
            return ec;
        }
        connection_.activate();
        is_open_ = true;
        return std::error_code();
    }

    void close(uint16_t status_code, const std::string &reason) {
        if (!is_open_ || is_closing_) {
            return;
        }
        is_closing_ = true;
        connection_.close(status_code, reason);
        is_open_ = false;
    }

    std::error_code send_text(const std::string &text, bool fin) {
        return connection_.send_text(text, fin);
    }

    std::error_code send_binary(const uint8_t *data, size_t size, bool fin) {
        return connection_.send_binary(data, size, fin);
    }

    std::error_code send_continuation(const std::string &data, bool fin) {
        return connection_.send_continuation(reinterpret_cast<const uint8_t *>(data.data()),
                                             data.size(), fin);
    }

    void set_on_open(open_callback cb) {
        connection_.set_on_open(std::move(cb));
    }
    void set_on_message(message_callback cb) {
        connection_.set_on_message(std::move(cb));
    }
    void set_on_close(close_callback cb) {
        connection_.set_on_close(std::move(cb));
    }
    void set_on_error(error_callback cb) {
        connection_.set_on_error(std::move(cb));
    }

    bool is_open() const {
        return is_open_;
    }
    bool is_ssl() const {
        return connection_.is_ssl();
    }

    void run() {
        io_context_.run();
    }

    void stop() {
        io_context_.stop();
    }

    void set_ssl_context(std::shared_ptr<net::ssl_context> ctx) {
        ssl_context_ = std::move(ctx);
    }

#if WSCPP_ENABLE_DEFLATE
    void enable_permessage_deflate(bool enable) {
        request_deflate_ = enable;
    }
#endif

  private:
    std::error_code ensure_ssl_context() {
        if (ssl_context_) {
            return std::error_code();
        }
#if WSCPP_USE_ASIO
        ssl_context_ = std::make_shared<net::ssl_context>(asio::ssl::context::tlsv12_client);
        ssl_context_->set_verify_mode(asio::ssl::verify_none);
        return std::error_code();
#else
        return net::ssl_context::make(net::ssl_context::role::client, ssl_context_);
#endif
    }

    url_info parse_url(const std::string &url) {
        url_info info;
        info.path = "/";
        info.secure = false;
        info.scheme = "ws";

        std::string work = url;
        std::size_t pos = work.find("://");
        if (pos != std::string::npos) {
            info.scheme = work.substr(0, pos);
            info.secure = (info.scheme == "wss");
            work = work.substr(pos + 3);
        }

        pos = work.find('/');
        if (pos != std::string::npos) {
            info.path = work.substr(pos);
            work = work.substr(0, pos);
        }

        pos = work.find(':');
        if (pos != std::string::npos) {
            info.host = work.substr(0, pos);
            info.port = work.substr(pos + 1);
        } else {
            info.host = work;
            info.port = info.secure ? "443" : "80";
        }
        return info;
    }

    std::error_code perform_handshake(const url_info &info) {
        const std::string key = handshake::generate_key();
#if WSCPP_ENABLE_DEFLATE
        const std::string extensions =
            request_deflate_ ? extensions::permessage_deflate_offer() : std::string();
        const std::string request =
            handshake::build_client_request(info.host, info.port, info.path, key, extensions);
#else
        const std::string request =
            handshake::build_client_request(info.host, info.port, info.path, key);
#endif

        std::error_code ec;
        connection_.socket().write(request.data(), request.size(), ec);
        if (ec) {
            return ec;
        }

        net::stream_buffer response;
        connection_.socket().read_until(response, "\r\n\r\n", ec);
        if (ec) {
            return ec;
        }
        const std::string response_str = net::stream_buffer_to_string(response);

        std::string status_line;
        std::map<std::string, std::string> headers;
        if (!handshake::parse_http_headers(response_str, status_line, headers) ||
            status_line.find("101") == std::string::npos) {
            return make_error_code(errc::handshake_failed);
        }

#if WSCPP_ENABLE_DEFLATE
        if (request_deflate_) {
            const std::map<std::string, std::string>::const_iterator ext =
                headers.find("sec-websocket-extensions");
            if (ext != headers.end() && extensions::header_offers_permessage_deflate(ext->second)) {
                connection_.set_permessage_deflate(true);
            }
        }
#endif
        return std::error_code();
    }

    net::io_context io_context_;
    connection_type connection_;
    bool is_open_;
    bool is_closing_;
#if WSCPP_ENABLE_DEFLATE
    bool request_deflate_;
#endif
    std::shared_ptr<net::ssl_context> ssl_context_;
};

client::client() : pimpl_(detail::make_unique<impl>()) {}

client::~client() = default;

std::error_code client::connect(const std::string &url) {
    return pimpl_->connect(url);
}

void client::close(uint16_t status_code, const std::string &reason) {
    pimpl_->close(status_code, reason);
}

std::error_code client::send_text(const std::string &text, bool fin) {
    return pimpl_->send_text(text, fin);
}

std::error_code client::send_binary(const uint8_t *data, size_t size, bool fin) {
    return pimpl_->send_binary(data, size, fin);
}

std::error_code client::send_continuation(const std::string &data, bool fin) {
    return pimpl_->send_continuation(data, fin);
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

void client::set_ssl_context(std::shared_ptr<ssl_context> ctx) {
    pimpl_->set_ssl_context(std::move(ctx));
}

#if WSCPP_ENABLE_DEFLATE
void client::enable_permessage_deflate(bool enable) {
    pimpl_->enable_permessage_deflate(enable);
}
#endif

} // namespace wscpp
