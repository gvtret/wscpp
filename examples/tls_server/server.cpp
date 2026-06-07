// TLS WebSocket server example (wss://)

#include <wscpp/server.hpp>
#include <asio/ssl/context.hpp>
#include <iostream>
#include <memory>
#include <string>

using namespace wscpp;

namespace {

void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [--port PORT] [--cert CERT.pem] [--key KEY.pem]\n"
              << "  Defaults: port=8443, cert=cert.pem, key=key.pem\n";
}

} // namespace

int main(int argc, char* argv[]) {
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

    try {
        auto ssl_ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_server);
        ssl_ctx->set_options(
            asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3);
        ssl_ctx->use_certificate_chain_file(cert_path);
        ssl_ctx->use_private_key_file(key_path, asio::ssl::context::pem);

        server srv;
        srv.set_ssl_context(ssl_ctx);

        srv.set_on_connection([&](std::shared_ptr<connection> conn) {
            std::cout << "Secure connection from "
                      << conn->socket().native_socket().remote_endpoint() << std::endl;
            conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode) {
                std::string message(data.begin(), data.end());
                std::cout << "Received: " << message << std::endl;
                conn->send_text("Echo: " + message);
            });
        });

        srv.listen(port);
        std::cout << "WSS server listening on port " << port << std::endl;
        srv.start();
        srv.join();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
