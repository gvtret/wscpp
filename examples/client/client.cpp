// Client example for wscpp
// This example demonstrates a simple WebSocket client using wscpp

#include <wscpp/client.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>

using namespace wscpp;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ws|wss>://<host>:<port>/<path>" << std::endl;
        return 1;
    }
    
    std::string url = argv[1];
    
    try {
        client cli;
        
        // Set callbacks
        cli.set_on_open([&]() {
            std::cout << "Connected to " << url << std::endl;
            
            // Send a test message
            cli.send_text("Hello, WebSocket!");
        });
        
        cli.set_on_message([&](const std::vector<uint8_t>& data, frame::opcode op) {
            std::string message(data.begin(), data.end());
            std::cout << "Received: " << message << std::endl;
            
            // Echo the message back
            cli.send_text("Echo: " + message);
        });
        
        cli.set_on_close([&](uint16_t code, const std::string& reason) {
            std::cout << "Closed: " << code << " - " << reason << std::endl;
        });
        
        cli.set_on_error([&](const std::string& error) {
            std::cerr << "Error: " << error << std::endl;
        });
        
        // Connect
        cli.connect(url);
        
        // Run the event loop
        cli.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
