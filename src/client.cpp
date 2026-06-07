#include <wscpp/client.hpp>
#include <wscpp/handshake.hpp>
#include <wscpp/detail/make_unique.hpp>
#include <asio/streambuf.hpp>
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

class client::impl {
public:
    impl()
        : io_context_(),
          connection_(io_context_),
          is_open_(false),
          is_closing_(false) {
        connection_.set_role(connection_role::client);
        ssl_context_ = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
        ssl_context_->set_verify_mode(asio::ssl::verify_none);
    }

    ~impl() {
        stop();
    }

    void connect(const std::string& url) {
        const url_info info = parse_url(url);

        if (info.secure) {
            connection_.socket().enable_ssl(ssl_context_);
            connection_.socket().set_ssl_hostname(info.host);
            connection_.socket().connect(info.host, info.port);
            connection_.socket().ssl_handshake(true);
        } else {
            connection_.socket().connect(info.host, info.port);
        }

        perform_handshake(info);
        connection_.activate();
        is_open_ = true;
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

    void send_continuation(const std::string& data, bool fin) {
        connection_.send_continuation(
            reinterpret_cast<const uint8_t*>(data.data()), data.size(), fin);
    }

    void set_on_open(open_callback cb) { connection_.set_on_open(cb); }
    void set_on_message(message_callback cb) { connection_.set_on_message(cb); }
    void set_on_close(close_callback cb) { connection_.set_on_close(cb); }
    void set_on_error(error_callback cb) { connection_.set_on_error(cb); }

    bool is_open() const { return is_open_; }
    bool is_ssl() const { return connection_.is_ssl(); }

    void run() {
        io_context_.run();
    }

    void stop() {
        io_context_.stop();
    }

private:
    url_info parse_url(const std::string& url) {
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

    void perform_handshake(const url_info& info) {
        const std::string key = handshake::generate_key();
        const std::string request =
            handshake::build_client_request(info.host, info.port, info.path, key);

        connection_.socket().write(request.data(), request.size());

        asio::streambuf response;
        connection_.socket().read_until(response, "\r\n\r\n");
        const std::string response_str = streambuf_to_string(response);

        if (response_str.find("101") == std::string::npos) {
            throw std::runtime_error("WebSocket handshake failed");
        }
    }

    asio::io_context io_context_;
    connection_type connection_;
    bool is_open_;
    bool is_closing_;
    std::shared_ptr<asio::ssl::context> ssl_context_;
};

client::client()
    : pimpl_(detail::make_unique<impl>()) {}

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

void client::send_continuation(const std::string& data, bool fin) {
    pimpl_->send_continuation(data, fin);
}

void client::set_on_open(open_callback cb) {
    pimpl_->set_on_open(cb);
}

void client::set_on_message(message_callback cb) {
    pimpl_->set_on_message(cb);
}

void client::set_on_close(close_callback cb) {
    pimpl_->set_on_close(cb);
}

void client::set_on_error(error_callback cb) {
    pimpl_->set_on_error(cb);
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
