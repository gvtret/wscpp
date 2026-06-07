#include <wscpp/crypto/ssl_context.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/error.hpp>
#include <asio/ssl/verify_mode.hpp>
#include <asio/error_code.hpp>
#include <asio/error_code.hpp>

namespace wscpp {
namespace crypto {

// Helper function to convert method to asio::ssl::context method
static asio::ssl::context::method to_asio_method(ssl_context::method m) {
    switch (m) {
        case ssl_context::method::tlsv12:
            return asio::ssl::context::tlsv12;
        case ssl_context::method::tlsv11:
            return asio::ssl::context::tlsv11;
        case ssl_context::method::tlsv13:
            return asio::ssl::context::tlsv13;
        case ssl_context::method::sslv2:
            return asio::ssl::context::sslv2;
        case ssl_context::method::sslv3:
            return asio::ssl::context::sslv3;
        case ssl_context::method::tls_client:
            return asio::ssl::context::tls_client;
        case ssl_context::method::tls_server:
            return asio::ssl::context::tls_server;
        default:
            return asio::ssl::context::tlsv12;
    }
}

ssl_context::ssl_context(method m) {
    context_ = std::make_shared<asio::ssl::context>(to_asio_method(m));
}

ssl_context::~ssl_context() = default;

ssl_context::ssl_context(ssl_context&&) noexcept = default;
ssl_context& ssl_context::operator=(ssl_context&&) noexcept = default;

asio::ssl::context& ssl_context::native_handle() {
    return *context_;
}

const asio::ssl::context& ssl_context::native_handle() const {
    return *context_;
}

void ssl_context::set_options(uint32_t options) {
    context_->set_options(options);
}

void ssl_context::set_verify_mode(asio::ssl::verify_mode mode) {
    context_->set_verify_mode(mode);
}

void ssl_context::set_verify_depth(int depth) {
    context_->set_verify_depth(depth);
}

bool ssl_context::load_verify_file(const std::string& path) {
    asio::error_code ec;
    context_->load_verify_file(path, ec);
    return !ec;
}

bool ssl_context::load_cert_chain(const std::string& path) {
    asio::error_code ec;
    context_->use_certificate_chain_file(path, ec);
    return !ec;
}

bool ssl_context::load_private_key(const std::string& path) {
    asio::error_code ec;
    context_->use_private_key_file(path, asio::ssl::context::pem, ec);
    return !ec;
}

std::shared_ptr<asio::ssl::context> ssl_context::shared_context() {
    return context_;
}

} // namespace crypto
} // namespace wscpp
