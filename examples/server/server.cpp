// Server example for wscpp

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <wscpp/server.hpp>

using namespace wscpp;

int main(int argc, char *argv[]) {
    uint16_t port = 8080;

    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }

    server srv;

    srv.set_on_connection([&](std::shared_ptr<connection> conn) {
#if WSCPP_USE_ASIO
        std::cout << "New connection from " << conn->socket().native_socket().remote_endpoint()
                  << std::endl;
#else
        std::cout << "New connection accepted" << std::endl;
#endif

        conn->set_on_message([conn](const std::vector<uint8_t> &data, frame::opcode) {
            const std::string message(data.begin(), data.end());
            std::cout << "Received: " << message << std::endl;
            conn->send_text("Echo: " + message);
        });

        conn->set_on_close([](uint16_t code, const std::string &reason) {
            std::cout << "Connection closed: " << code << " - " << reason << std::endl;
        });
    });

    srv.set_on_error([](std::shared_ptr<connection>, const std::string &error) {
        std::cerr << "Connection error: " << error << std::endl;
    });

    const std::error_code listen_ec = srv.listen(port);
    if (listen_ec) {
        std::cerr << "Listen failed: " << listen_ec.message() << std::endl;
        return 1;
    }
    std::cout << "Server listening on port " << port << std::endl;

    const std::error_code start_ec = srv.start();
    if (start_ec) {
        std::cerr << "Start failed: " << start_ec.message() << std::endl;
        return 1;
    }
    srv.join();
    return 0;
}
