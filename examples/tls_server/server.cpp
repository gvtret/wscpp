// TLS WebSocket server example (wss://)

#include <wscpp/server.hpp>
#if WSCPP_USE_ASIO
#include <asio/ssl/context.hpp>
#else
#include <wscpp/net/openssl_context.hpp>
#endif
#include <iostream>
#include <memory>
#include <string>

using namespace wscpp;

namespace {

void print_usage(const char *prog) {
    std::cerr << "Usage: " << prog << " [--port PORT] [--cert CERT.pem] [--key KEY.pem]\n"
              << "  Defaults: port=8443, cert=cert.pem, key=key.pem\n";
}

} // namespace

int main(int argc, char *argv[]) {
    uint16_t port = 8443;
    std::string cert_path = "cert.pem";
    std::string key_path = "key.pem";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if (arg == "--cert" && i + 1 < argc) {
            cert_path = argv[++i];
        } else if (arg == "--key" && i + 1 < argc) {
            key_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

#if WSCPP_USE_ASIO
    auto ssl_ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_server);
    ssl_ctx->set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                         asio::ssl::context::no_sslv3);
    ssl_ctx->use_certificate_chain_file(cert_path);
    ssl_ctx->use_private_key_file(key_path, asio::ssl::context::pem);
#else
    std::shared_ptr<wscpp::net::openssl_context> ssl_ctx;
    const std::error_code ctx_ec =
        wscpp::net::openssl_context::make(wscpp::net::openssl_context::role::server, ssl_ctx);
    if (ctx_ec) {
        std::cerr << "SSL context failed: " << ctx_ec.message() << std::endl;
        return 1;
    }
    if (!ssl_ctx->use_certificate_chain_file(cert_path) ||
        !ssl_ctx->use_private_key_file(key_path)) {
        std::cerr << "Failed to load TLS certificate or key" << std::endl;
        return 1;
    }
#endif

    server srv;
    srv.set_ssl_context(ssl_ctx);

    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
#if WSCPP_USE_ASIO
        std::cout << "Secure connection from " << conn->socket().native_socket().remote_endpoint()
                  << std::endl;
#else
        std::cout << "Secure connection accepted" << std::endl;
#endif
        conn->set_on_message([conn](const std::vector<uint8_t> &data, frame::opcode) {
            const std::string message(data.begin(), data.end());
            std::cout << "Received: " << message << std::endl;
            conn->send_text("Echo: " + message);
        });
    });

    const std::error_code listen_ec = srv.listen(port);
    if (listen_ec) {
        std::cerr << "Listen failed: " << listen_ec.message() << std::endl;
        return 1;
    }
    std::cout << "WSS server listening on port " << port << std::endl;

    const std::error_code start_ec = srv.start();
    if (start_ec) {
        std::cerr << "Start failed: " << start_ec.message() << std::endl;
        return 1;
    }
    srv.join();
    return 0;
}
