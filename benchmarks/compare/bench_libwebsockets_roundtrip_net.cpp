// bench_libwebsockets_roundtrip_net.cpp — client-only echo latency over LAN

#include "../bench_net_args.hpp"
#include "../bench_util.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <libwebsockets.h>
#include <string>
#include <thread>
#include <vector>

using namespace wscpp::bench;

namespace {

constexpr char kProtocolName[] = "wscpp-bench-echo";

struct bench_state {
    int samples;
    int next_to_send;
    int in_flight;
    std::vector<double> latencies_ms;
    std::vector<clock::time_point> send_times;
    std::atomic<bool> done;
    struct lws *client_wsi;
};

static int callback_client(struct lws *wsi, enum lws_callback_reasons reason, void * /*user*/,
                           void *, size_t) {
    bench_state *st = static_cast<bench_state *>(lws_context_user(lws_get_context(wsi)));

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        st->client_wsi = wsi;
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        if (st->in_flight >= 0) {
            break;
        }
        if (st->next_to_send >= st->samples) {
            st->done = true;
            break;
        }
        char msg[32];
        std::snprintf(msg, sizeof(msg), "ping-%d", st->next_to_send);
        const size_t mlen = std::strlen(msg);
        unsigned char buf[LWS_PRE + 32];
        std::memcpy(buf + LWS_PRE, msg, mlen);
        st->send_times[static_cast<std::size_t>(st->next_to_send)] = clock::now();
        st->in_flight = st->next_to_send;
        ++st->next_to_send;
        if (lws_write(wsi, buf + LWS_PRE, mlen, LWS_WRITE_TEXT) < 0) {
            st->done = true;
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE: {
        if (st->in_flight >= 0) {
            const double ms =
                elapsed_sec(st->send_times[static_cast<std::size_t>(st->in_flight)], clock::now()) *
                1000.0;
            st->latencies_ms.push_back(ms);
            st->in_flight = -1;
        }
        if (st->next_to_send >= st->samples &&
            static_cast<int>(st->latencies_ms.size()) >= st->samples) {
            st->done = true;
        } else {
            lws_callback_on_writable(wsi);
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    case LWS_CALLBACK_CLIENT_CLOSED:
        st->done = true;
        break;

    default:
        break;
    }
    return 0;
}

static struct lws_protocols kClientProtocols[] = {
    {kProtocolName, callback_client, 0, 4096, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0},
};

} // namespace

int main(int argc, char *argv[]) {
    net_bench_args args;
    if (!parse_net_client_args(argc, argv, args, "bench_libwebsockets_roundtrip_net")) {
        return 1;
    }

    print_compare_banner("bench_libwebsockets_roundtrip_net");
    std::printf("target ws://%s:%u/ samples=%d\n", args.host.c_str(), args.port, args.samples);

    lws_set_log_level(0, NULL);

    bench_state state;
    state.samples = args.samples;
    state.next_to_send = 0;
    state.in_flight = -1;
    state.done = false;
    state.client_wsi = NULL;
    state.latencies_ms.reserve(static_cast<std::size_t>(args.samples));
    state.send_times.resize(static_cast<std::size_t>(args.samples));

    struct lws_context_creation_info cinfo;
    std::memset(&cinfo, 0, sizeof(cinfo));
    cinfo.port = CONTEXT_PORT_NO_LISTEN;
    cinfo.protocols = kClientProtocols;
    cinfo.user = &state;
    cinfo.fd_limit_per_thread = 4;

    struct lws_context *client_ctx = lws_create_context(&cinfo);
    if (!client_ctx) {
        std::fprintf(stderr, "libwebsockets client context failed\n");
        return 1;
    }

    struct lws_client_connect_info ccinfo;
    std::memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = client_ctx;
    ccinfo.address = args.host.c_str();
    ccinfo.port = static_cast<int>(args.port);
    ccinfo.path = "/";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = kProtocolName;

    if (!lws_client_connect_via_info(&ccinfo)) {
        lws_context_destroy(client_ctx);
        std::fprintf(stderr, "libwebsockets client connect failed\n");
        return 1;
    }

    while (!state.done.load()) {
        lws_service(client_ctx, 10);
    }

    lws_context_destroy(client_ctx);

    std::printf("libwebsockets net roundtrip (%zu samples)\n", state.latencies_ms.size());
    print_latency("echo_latency", state.latencies_ms);

    return state.latencies_ms.size() == static_cast<std::size_t>(args.samples) ? 0 : 1;
}
