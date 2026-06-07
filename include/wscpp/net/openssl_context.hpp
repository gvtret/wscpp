#ifndef WSCPP_NET_OPENSSL_CONTEXT_HPP
#define WSCPP_NET_OPENSSL_CONTEXT_HPP

#include <memory>
#include <openssl/ssl.h>
#include <string>
#include <system_error>

namespace wscpp {
namespace net {

/** @brief OpenSSL TLS context for native socket transport. */
class openssl_context {
public:
    enum class role { client, server };

    static std::error_code make(role r, std::shared_ptr<openssl_context>& out);

    ~openssl_context();

    openssl_context(const openssl_context&) = delete;
    openssl_context& operator=(const openssl_context&) = delete;

    SSL_CTX* native() const { return ctx_; }
    bool valid() const { return ctx_ != nullptr; }

    void set_verify_none();

    /** @brief Load trusted CA bundle (PEM) for peer verification. */
    bool load_verify_file(const std::string& path);

    /** @brief Require valid peer certificate (server: client cert / client: server cert). */
    void set_verify_peer(bool fail_if_no_peer_cert = false);

    bool use_certificate_chain_file(const std::string& path);
    bool use_private_key_file(const std::string& path);

private:
    explicit openssl_context(role r);

    SSL_CTX* ctx_;
};

using ssl_context = openssl_context;

} // namespace net
} // namespace wscpp

#endif
