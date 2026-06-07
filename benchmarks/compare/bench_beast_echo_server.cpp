// bench_beast_echo_server.cpp — echo server for LAN benchmarks

#include "../bench_net_args.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdio>
#include <string>

using namespace wscpp::bench;

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

void echo_session(tcp::socket socket) {
    try {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();

        beast::flat_buffer buffer;
        for (;;) {
            ws.read(buffer);
            ws.text(ws.got_text());
            ws.write(buffer.data());
            buffer.consume(buffer.size());
        }
    } catch (const beast::system_error &se) {
        if (se.code() != websocket::error::closed && se.code() != net::error::connection_reset &&
            se.code() != net::error::eof) {
            std::fprintf(stderr, "beast session: %s\n", se.code().message().c_str());
        }
    } catch (const std::exception &ex) {
        std::fprintf(stderr, "beast session: %s\n", ex.what());
    }
}

} // namespace

int main(int argc, char *argv[]) {
    uint16_t port = 19081;
    std::string bind_host = "0.0.0.0";
    if (!parse_echo_server_args(argc, argv, port, bind_host)) {
        return argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") ? 0
                                                                                              : 1;
    }

    try {
        net::io_context ioc;
        const tcp::endpoint endpoint(net::ip::make_address(bind_host), port);
        tcp::acceptor acceptor(ioc, endpoint);
        acceptor.set_option(tcp::acceptor::reuse_address(true));

        std::printf("beast echo server listening on %s:%u\n", bind_host.c_str(), port);
        for (;;) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            echo_session(std::move(socket));
        }
    } catch (const std::exception &ex) {
        std::fprintf(stderr, "beast server: %s\n", ex.what());
        return 1;
    }

    return 0;
}
