#ifndef WSCPP_CRYPTO_SSL_CONTEXT_HPP
#define WSCPP_CRYPTO_SSL_CONTEXT_HPP

#include <asio/ssl/context.hpp>
#include <memory>
#include <string>

namespace wscpp {
namespace crypto {

class ssl_context {
public:
    enum class method {
        tls_client,
        tls_server,
        tlsv1,
        tlsv11,
        tlsv12,
        tlsv13,
        sslv2,
        sslv3
    };
    
    ssl_context(method m = method::tlsv12);
    ~ssl_context();
    
    // Disable copy
    ssl_context(const ssl_context&) = delete;
    ssl_context& operator=(const ssl_context&) = delete;
    
    // Enable move
    ssl_context(ssl_context&&) noexcept;
    ssl_context& operator=(ssl_context&&) noexcept;
    
    // Get underlying context
    asio::ssl::context& native_handle();
    const asio::ssl::context& native_handle() const;
    
    // Configuration methods
    void set_options(uint32_t options);
    void set_verify_mode(asio::ssl::verify_mode mode);
    void set_verify_depth(int depth);
    
    // Load certificates
    bool load_verify_file(const std::string& path);
    bool load_cert_chain(const std::string& path);
    bool load_private_key(const std::string& path);
    
    // Get context pointer
    std::shared_ptr<asio::ssl::context> shared_context();
    
private:
    std::shared_ptr<asio::ssl::context> context_;
};

} // namespace crypto
} // namespace wscpp

#endif // WSCPP_CRYPTO_SSL_CONTEXT_HPP
