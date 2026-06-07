#ifndef WSCPP_TESTS_TLS_FIXTURES_HPP
#define WSCPP_TESTS_TLS_FIXTURES_HPP

#include <gtest/gtest.h>
#include <wscpp/client.hpp>
#include <wscpp/server.hpp>
#if WSCPP_USE_ASIO
#include <asio/ssl/context.hpp>
#else
#include <wscpp/net/openssl_context.hpp>
#endif
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>

namespace wscpp {
namespace test {
namespace tls {

struct paths {
    std::string dir;
    std::string ca_crt;
    std::string ca_key;
    std::string server_crt;
    std::string server_key;
    std::string client_crt;
    std::string client_key;
};

inline bool file_exists(const std::string &path) {
    std::ifstream in(path.c_str());
    return in.good();
}

inline std::string join_path(const std::string &dir, const char *name) {
    if (dir.empty()) {
        return name;
    }
    if (dir[dir.size() - 1] == '/') {
        return dir + name;
    }
    return dir + "/" + name;
}

inline paths fixture_paths() {
    paths p;
    const char *env = std::getenv("WSCPP_TLS_FIXTURES_DIR");
#ifdef WSCPP_TLS_FIXTURES_DIR
    p.dir = env ? env : WSCPP_TLS_FIXTURES_DIR;
#else
    p.dir = env ? env : "tests/tls/fixtures";
#endif
    p.ca_crt = join_path(p.dir, "ca.crt");
    p.ca_key = join_path(p.dir, "ca.key");
    p.server_crt = join_path(p.dir, "server.crt");
    p.server_key = join_path(p.dir, "server.key");
    p.client_crt = join_path(p.dir, "client.crt");
    p.client_key = join_path(p.dir, "client.key");
    return p;
}

inline bool fixtures_available() {
    const paths p = fixture_paths();
    return file_exists(p.ca_crt) && file_exists(p.ca_key) && file_exists(p.server_crt) &&
           file_exists(p.server_key) && file_exists(p.client_crt) && file_exists(p.client_key);
}

inline void skip_unless_fixtures() {
    if (!fixtures_available()) {
        GTEST_SKIP() << "TLS fixtures missing in " << fixture_paths().dir
                     << " — run: bash scripts/gen-test-tls-certs.sh";
    }
}

#define WSCPP_REQUIRE_TLS_FIXTURES()                                                               \
    do {                                                                                           \
        if (!::wscpp::test::tls::fixtures_available()) {                                           \
            GTEST_SKIP() << "TLS fixtures missing in " << ::wscpp::test::tls::fixture_paths().dir  \
                         << " — run: bash scripts/gen-test-tls-certs.sh";                          \
        }                                                                                          \
    } while (0)

inline std::shared_ptr<server::ssl_context> make_server_ssl_context(const paths &p,
                                                                    bool require_client_cert) {
#if WSCPP_USE_ASIO
    std::shared_ptr<asio::ssl::context> ctx(
        new asio::ssl::context(asio::ssl::context::tlsv12_server));
    ctx->set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                     asio::ssl::context::no_sslv3);
    asio::error_code ec;
    ctx->use_certificate_chain_file(p.server_crt, ec);
    if (ec) {
        return std::shared_ptr<server::ssl_context>();
    }
    ctx->use_private_key_file(p.server_key, asio::ssl::context::pem, ec);
    if (ec) {
        return std::shared_ptr<server::ssl_context>();
    }
    if (require_client_cert) {
        ctx->load_verify_file(p.ca_crt, ec);
        if (ec) {
            return std::shared_ptr<server::ssl_context>();
        }
        ctx->set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
    }
    return ctx;
#else
    std::shared_ptr<net::openssl_context> ctx;
    if (net::openssl_context::make(net::openssl_context::role::server, ctx)) {
        return std::shared_ptr<server::ssl_context>();
    }
    if (!ctx->use_certificate_chain_file(p.server_crt) ||
        !ctx->use_private_key_file(p.server_key)) {
        return std::shared_ptr<server::ssl_context>();
    }
    if (require_client_cert) {
        if (!ctx->load_verify_file(p.ca_crt)) {
            return std::shared_ptr<server::ssl_context>();
        }
        ctx->set_verify_peer(true);
    }
    return ctx;
#endif
}

inline std::shared_ptr<client::ssl_context>
make_client_ssl_context(const paths &p, bool verify_server, bool present_client_cert) {
#if WSCPP_USE_ASIO
    std::shared_ptr<asio::ssl::context> ctx(
        new asio::ssl::context(asio::ssl::context::tlsv12_client));
    asio::error_code ec;
    if (present_client_cert) {
        ctx->use_certificate_chain_file(p.client_crt, ec);
        if (ec) {
            return std::shared_ptr<client::ssl_context>();
        }
        ctx->use_private_key_file(p.client_key, asio::ssl::context::pem, ec);
        if (ec) {
            return std::shared_ptr<client::ssl_context>();
        }
    }
    if (verify_server) {
        ctx->load_verify_file(p.ca_crt, ec);
        if (ec) {
            return std::shared_ptr<client::ssl_context>();
        }
        ctx->set_verify_mode(asio::ssl::verify_peer);
    } else {
        ctx->set_verify_mode(asio::ssl::verify_none);
    }
    return ctx;
#else
    std::shared_ptr<net::openssl_context> ctx;
    if (net::openssl_context::make(net::openssl_context::role::client, ctx)) {
        return std::shared_ptr<client::ssl_context>();
    }
    if (present_client_cert) {
        if (!ctx->use_certificate_chain_file(p.client_crt) ||
            !ctx->use_private_key_file(p.client_key)) {
            return std::shared_ptr<client::ssl_context>();
        }
    }
    if (verify_server) {
        if (!ctx->load_verify_file(p.ca_crt)) {
            return std::shared_ptr<client::ssl_context>();
        }
        ctx->set_verify_peer(false);
    } else {
        ctx->set_verify_none();
    }
    return ctx;
#endif
}

} // namespace tls
} // namespace test
} // namespace wscpp

#endif
