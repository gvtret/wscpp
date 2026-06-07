#include <wscpp/net/openssl_context.hpp>
#include <wscpp/error.hpp>

namespace wscpp {
namespace net {

namespace {

int verify_callback(int preverify_ok, X509_STORE_CTX*) {
    return preverify_ok;
}

} // namespace

openssl_context::openssl_context(role r) : ctx_(nullptr) {
    const SSL_METHOD* method = (r == role::client) ? TLS_client_method() : TLS_server_method();
    if (!method) {
        return;
    }
    ctx_ = SSL_CTX_new(method);
    if (ctx_) {
        SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);
    }
}

openssl_context::~openssl_context() {
    if (ctx_) {
        SSL_CTX_free(ctx_);
        ctx_ = nullptr;
    }
}

std::error_code openssl_context::make(role r, std::shared_ptr<openssl_context>& out) {
    out.reset(new openssl_context(r));
    if (!out->valid()) {
        out.reset();
        return make_error_code(errc::ssl_init_failed);
    }
    return std::error_code();
}

void openssl_context::set_verify_none() {
    if (ctx_) {
        SSL_CTX_set_verify(ctx_, SSL_VERIFY_NONE, nullptr);
    }
}

bool openssl_context::load_verify_file(const std::string& path) {
    return ctx_ && SSL_CTX_load_verify_locations(ctx_, path.c_str(), nullptr) == 1;
}

void openssl_context::set_verify_peer(bool fail_if_no_peer_cert) {
    if (!ctx_) {
        return;
    }
    int mode = SSL_VERIFY_PEER;
    if (fail_if_no_peer_cert) {
        mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
    SSL_CTX_set_verify(ctx_, mode, verify_callback);
}

bool openssl_context::use_certificate_chain_file(const std::string& path) {
    return ctx_ && SSL_CTX_use_certificate_chain_file(ctx_, path.c_str()) == 1;
}

bool openssl_context::use_private_key_file(const std::string& path) {
    return ctx_ && SSL_CTX_use_PrivateKey_file(ctx_, path.c_str(), SSL_FILETYPE_PEM) == 1;
}

} // namespace net
} // namespace wscpp
