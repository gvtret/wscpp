// bench_libwebsockets_echo_server.cpp — echo server for LAN benchmarks

#include "../bench_net_args.hpp"
#include <cstdio>
#include <cstring>
#include <libwebsockets.h>
#include <string>

using namespace wscpp::bench;

namespace {

constexpr char kProtocolName[] = "wscpp-bench-echo";

static int callback_server(struct lws *wsi, enum lws_callback_reasons reason, void * /*user*/,
                           void *in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_RECEIVE: {
        unsigned char buf[LWS_PRE + 4096];
        if (len > sizeof(buf) - LWS_PRE) {
            len = sizeof(buf) - LWS_PRE;
        }
        std::memcpy(buf + LWS_PRE, in, len);
        lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
        break;
    }
    default:
        break;
    }
    return 0;
}

static struct lws_protocols kServerProtocols[] = {
    {kProtocolName, callback_server, 0, 4096, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0},
};

} // namespace

int main(int argc, char *argv[]) {
    uint16_t port = 19081;
    std::string bind_host = "0.0.0.0";
    if (!parse_echo_server_args(argc, argv, port, bind_host)) {
        return argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") ? 0
                                                                                             : 1;
    }

    lws_set_log_level(0, NULL);

    struct lws_context_creation_info info;
    std::memset(&info, 0, sizeof(info));
    info.port = static_cast<int>(port);
    info.iface = bind_host.c_str();
    info.protocols = kServerProtocols;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    struct lws_context *ctx = lws_create_context(&info);
    if (!ctx) {
        std::fprintf(stderr, "libwebsockets server context failed\n");
        return 1;
    }

    std::printf("libwebsockets echo server listening on %s:%u\n", bind_host.c_str(), port);
    while (true) {
        lws_service(ctx, 50);
    }

    lws_context_destroy(ctx);
    return 0;
}
