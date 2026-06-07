// TLS WebSocket client example (wss://)

#include <wscpp/client.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace wscpp;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " wss://<host>:<port>/<path>\n"
                  << "Example: " << argv[0] << " wss://127.0.0.1:8443/\n";
        return 1;
    }

    const std::string url = argv[1];

    try {
        client cli;

        cli.set_on_open([&]() {
            std::cout << "Connected to " << url << std::endl;
            cli.send_text("Hello over TLS!");
        });

        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode) {
            std::string message(data.begin(), data.end());
            std::cout << "Received: " << message << std::endl;
            cli.close();
        });

        cli.set_on_close([&](uint16_t code, const std::string& reason) {
            std::cout << "Closed: " << code << " - " << reason << std::endl;
        });

        cli.set_on_error([&](const std::string& error) {
            std::cerr << "Error: " << error << std::endl;
        });

        cli.connect(url);
        cli.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
