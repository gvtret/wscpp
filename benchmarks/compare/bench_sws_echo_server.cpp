// bench_sws_echo_server.cpp — echo server for LAN benchmarks

#include "../bench_net_args.hpp"
#include "server_ws.hpp"
#include <cstdio>
#include <memory>
#include <string>

using namespace wscpp::bench;

namespace {

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using Connection = WsServer::Connection;
using InMessage = WsServer::InMessage;

} // namespace

int main(int argc, char *argv[]) {
    uint16_t port = 19081;
    std::string bind_host = "0.0.0.0";
    if (!parse_echo_server_args(argc, argv, port, bind_host)) {
        return argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") ? 0
                                                                                              : 1;
    }

    WsServer server;
    server.config.port = port;
    server.config.address = bind_host;
    server.config.reuse_address = true;
    server.config.thread_pool_size = 1;

    auto &echo = server.endpoint["^/echo/?$"];
    echo.on_message = [](std::shared_ptr<Connection> connection,
                         std::shared_ptr<InMessage> message) {
        connection->send(message->string());
    };

    std::printf("simple-websocket-server echo on %s:%u/echo\n", bind_host.c_str(), port);
    server.start();
    return 0;
}
