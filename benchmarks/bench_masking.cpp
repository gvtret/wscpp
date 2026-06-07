// bench_masking.cpp — RFC 6455 mask/unmask throughput

#include "bench_common.hpp"
#include <wscpp/frame/parser.hpp>
#include <wscpp/frame/builder.hpp>
#include <array>
#include <cstdio>
#include <vector>

using namespace wscpp;
using namespace wscpp::bench;

namespace {

void mask_payload(std::vector<uint8_t>& data, const std::array<uint8_t, 4>& key) {
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i % 4];
    }
}

} // namespace

int main() {
    const std::size_t payload_size = 1024 * 1024;
    const int iterations = 200;

    std::vector<uint8_t> payload(payload_size);
    for (std::size_t i = 0; i < payload_size; ++i) {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }

    const std::array<uint8_t, 4> key = {{0x37, 0xfa, 0x21, 0x3d}};

    const clock::time_point mask_start = clock::now();
    for (int i = 0; i < iterations; ++i) {
        mask_payload(payload, key);
        mask_payload(payload, key);
    }
    const clock::time_point mask_end = clock::now();

    const double mask_sec = elapsed_sec(mask_start, mask_end);
    print_throughput("mask_unmask_xor", static_cast<double>(payload_size * iterations * 2),
                     mask_sec, iterations * 2);

    frame::builder builder;
    frame::parser parser;
    frame::frame_header header;
    std::vector<uint8_t> parsed;

    const clock::time_point roundtrip_start = clock::now();
    for (int i = 0; i < 50; ++i) {
        const std::vector<uint8_t> frame =
            builder.build_binary(payload.data(), payload.size(), true, true);
        parser.reset();
        if (parser.parse(frame.data(), frame.size(), header, parsed) !=
            frame::parse_result::COMPLETE) {
            std::fprintf(stderr, "masked parse failed\n");
            return 1;
        }
    }
    const clock::time_point roundtrip_end = clock::now();

    const double rt_sec = elapsed_sec(roundtrip_start, roundtrip_end);
    print_throughput("masked_build_parse", static_cast<double>(payload_size * 50),
                     rt_sec, 50);

    return 0;
}
