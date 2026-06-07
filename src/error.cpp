#include <wscpp/error.hpp>

namespace wscpp {

namespace {

class wscpp_error_category : public std::error_category {
  public:
    const char *name() const noexcept override {
        return "wscpp";
    }

    std::string message(int ev) const override {
        switch (static_cast<errc>(ev)) {
        case errc::success:
            return "success";
        case errc::already_open:
            return "connection already open";
        case errc::not_open:
            return "connection not open";
        case errc::not_connected:
            return "socket not connected";
        case errc::handshake_failed:
            return "WebSocket handshake failed";
        case errc::invalid_address:
            return "invalid address";
        case errc::ssl_not_enabled:
            return "SSL not enabled";
        case errc::ssl_init_failed:
            return "SSL context initialization failed";
        case errc::ssl_io_error:
            return "SSL I/O error";
        case errc::ssl_would_block:
            return "SSL operation would block";
        case errc::compression_failed:
            return "permessage-deflate compression failed";
        case errc::server_not_listening:
            return "server not listening";
        case errc::host_not_found:
            return "host not found";
        default:
            return "unknown wscpp error";
        }
    }
};

const wscpp_error_category &wscpp_category_instance() {
    static const wscpp_error_category instance;
    return instance;
}

} // namespace

const std::error_category &error_category() {
    return wscpp_category_instance();
}

} // namespace wscpp
