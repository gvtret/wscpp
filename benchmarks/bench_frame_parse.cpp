// bench_frame_parse.cpp — parse/build throughput for 1 MiB binary payload

#include "bench_common.hpp"
#include <wscpp/frame/parser.hpp>
#include <wscpp/frame/builder.hpp>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace wscpp;
using namespace wscpp::bench;

int main() {
    const std::size_t payload_size = 1024 * 1024;
    const int iterations = 50;

    std::vector<uint8_t> payload(payload_size);
    for (std::size_t i = 0; i < payload_size; ++i) {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }

    frame::builder builder;
    frame::parser parser;
    frame::frame_header header;
    std::vector<uint8_t> parsed;

    const clock::time_point build_start = clock::now();
    std::vector<uint8_t> frame;
    for (int i = 0; i < iterations; ++i) {
        frame = builder.build_binary(payload.data(), payload.size(), true, false);
    }
    const clock::time_point build_end = clock::now();

    const double build_sec = elapsed_sec(build_start, build_end);
    print_throughput("frame_build", static_cast<double>(payload_size * iterations),
                     build_sec, iterations);

    const clock::time_point parse_start = clock::now();
    for (int i = 0; i < iterations; ++i) {
        parser.reset();
        const frame::parse_result result =
            parser.parse(frame.data(), frame.size(), header, parsed);
        if (result != frame::parse_result::COMPLETE || parsed.size() != payload_size) {
            std::fprintf(stderr, "parse failed\n");
            return 1;
        }
    }
    const clock::time_point parse_end = clock::now();

    const double parse_sec = elapsed_sec(parse_start, parse_end);
    print_throughput("frame_parse", static_cast<double>(payload_size * iterations),
                     parse_sec, iterations);

    return 0;
}
