// Client example for wscpp

#include <iostream>
#include <string>
#include <vector>
#include <wscpp/client.hpp>

using namespace wscpp;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ws|wss>://<host>:<port>/<path>" << std::endl;
        return 1;
    }

    const std::string url = argv[1];

    client cli;

    cli.set_on_open([&]() {
        std::cout << "Connected to " << url << std::endl;
        cli.send_text("Hello, WebSocket!");
    });

    cli.set_on_message([&](const std::vector<uint8_t> &data, frame::opcode) {
        const std::string message(data.begin(), data.end());
        std::cout << "Received: " << message << std::endl;
        cli.close();
    });

    cli.set_on_close([&](uint16_t code, const std::string &reason) {
        std::cout << "Closed: " << code << " - " << reason << std::endl;
    });

    cli.set_on_error(
        [&](const std::string &error) { std::cerr << "Error: " << error << std::endl; });

    const std::error_code ec = cli.connect(url);
    if (ec) {
        std::cerr << "Connect failed: " << ec.message() << std::endl;
        return 1;
    }

    cli.run();
    return 0;
}
