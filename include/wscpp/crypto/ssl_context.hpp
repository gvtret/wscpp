#ifndef WSCPP_CRYPTO_SSL_CONTEXT_HPP
#define WSCPP_CRYPTO_SSL_CONTEXT_HPP

/**
 * @file ssl_context.hpp
 * @brief Thin wrapper around @c asio::ssl::context.
 */

#include <asio/ssl/context.hpp>
#include <memory>
#include <string>

namespace wscpp {
namespace crypto {

/**
 * @brief OpenSSL/TLS context configuration helper.
 */
class ssl_context {
  public:
    /** @brief TLS method selection. */
    enum class method {
        tls_client, ///< Generic TLS client
        tls_server, ///< Generic TLS server
        tlsv1,
        tlsv11,
        tlsv12,
        tlsv13,
        sslv2,
        sslv3
    };

    explicit ssl_context(method m = method::tlsv12);
    ~ssl_context();

    ssl_context(const ssl_context &) = delete;
    ssl_context &operator=(const ssl_context &) = delete;

    ssl_context(ssl_context &&) noexcept;
    ssl_context &operator=(ssl_context &&) noexcept;

    asio::ssl::context &native_handle();
    const asio::ssl::context &native_handle() const;

    void set_options(uint32_t options);
    void set_verify_mode(asio::ssl::verify_mode mode);
    void set_verify_depth(int depth);

    bool load_verify_file(const std::string &path);
    bool load_cert_chain(const std::string &path);
    bool load_private_key(const std::string &path);

    std::shared_ptr<asio::ssl::context> shared_context();

  private:
    std::shared_ptr<asio::ssl::context> context_;
};

} // namespace crypto
} // namespace wscpp

#endif // WSCPP_CRYPTO_SSL_CONTEXT_HPP
