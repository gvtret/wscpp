// Server example for wscpp
// This example demonstrates a simple WebSocket server using wscpp

#include <wscpp/server.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace wscpp;

int main(int argc, char* argv[]) {
    uint16_t port = 8080;
    
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }
    
    try {
        server srv;
        
        // Set callbacks
        srv.set_on_connection([&](std::shared_ptr<connection> conn) {
            std::cout << "New connection from " << conn->socket().native_handle().remote_endpoint() << std::endl;
            
            // Set per-connection callbacks
            conn->set_on_message([conn](const std::vector<uint8_t>& data, frame::opcode op) {
                std::string message(data.begin(), data.end());
                std::cout << "Received: " << message << std::endl;
                
                // Echo the message back
                conn->send_text("Echo: " + message);
            });
            
            conn->set_on_close([conn](uint16_t code, const std::string& reason) {
                std::cout << "Connection closed: " << code << " - " << reason << std::endl;
            });
        });
        
        srv.set_on_error([](std::shared_ptr<connection> conn, const std::string& error) {
            std::cerr << "Connection error: " << error << std::endl;
        });
        
        // Listen on port
        srv.listen(port);
        std::cout << "Server listening on port " << port << std::endl;
        
        // Start server (blocking)
        srv.start();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
