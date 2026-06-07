// bench_ixwebsocket_echo_server.cpp — echo server for LAN benchmarks

#include "../bench_net_args.hpp"
#include <cstdio>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketServer.h>
#include <string>

using namespace wscpp::bench;

int main(int argc, char *argv[]) {
    uint16_t port = 19081;
    std::string bind_host = "0.0.0.0";
    if (!parse_echo_server_args(argc, argv, port, bind_host)) {
        return argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") ? 0
                                                                                              : 1;
    }

    ix::initNetSystem();

    ix::WebSocketServer server(static_cast<int>(port), bind_host);
    server.setOnClientMessageCallback([](std::shared_ptr<ix::ConnectionState>,
                                         ix::WebSocket &webSocket,
                                         const ix::WebSocketMessagePtr &msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            webSocket.send(msg->str, msg->binary);
        }
    });

    if (!server.listenAndStart()) {
        std::fprintf(stderr, "ixwebsocket server failed on %s:%u\n", bind_host.c_str(), port);
        ix::uninitNetSystem();
        return 1;
    }

    std::printf("ixwebsocket echo server listening on %s:%u\n", bind_host.c_str(), port);
    server.wait();
    ix::uninitNetSystem();
    return 0;
}
