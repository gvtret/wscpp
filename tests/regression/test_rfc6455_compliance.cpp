#include <gtest/gtest.h>
#include <wscpp/frame/builder.hpp>
#include <wscpp/frame/parser.hpp>

using namespace wscpp::frame;

// RFC 6455 section 5.7: single-frame unmasked text "Hello"
TEST(Rfc6455Compliance, Section57UnmaskedTextHello) {
    const uint8_t raw[] = {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f};
    parser p;
    frame_header header;
    std::vector<uint8_t> payload;
    EXPECT_EQ(p.parse(raw, sizeof(raw), header, payload), parse_result::COMPLETE);
    EXPECT_TRUE(header.fin);
    EXPECT_EQ(header.op, opcode::TEXT);
    EXPECT_EQ(std::string(payload.begin(), payload.end()), "Hello");
}

// RFC 6455 section 5.7: masked text "Hello"
TEST(Rfc6455Compliance, Section57MaskedTextHello) {
    const uint8_t raw[] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58};
    parser p;
    frame_header header;
    std::vector<uint8_t> payload;
    EXPECT_EQ(p.parse(raw, sizeof(raw), header, payload), parse_result::COMPLETE);
    EXPECT_TRUE(header.mask);
    EXPECT_EQ(std::string(payload.begin(), payload.end()), "Hello");
}

TEST(Rfc6455Compliance, CloseFrameRoundTrip) {
    builder b;
    std::vector<uint8_t> frame = b.build_close(1000, "bye", false);
    parser p;
    frame_header header;
    std::vector<uint8_t> payload;
    EXPECT_EQ(p.parse(frame.data(), frame.size(), header, payload), parse_result::COMPLETE);
    EXPECT_EQ(header.op, opcode::CLOSE);
    ASSERT_GE(payload.size(), 2u);
    const uint16_t code = static_cast<uint16_t>((payload[0] << 8) | payload[1]);
    EXPECT_EQ(code, 1000u);
}

TEST(Rfc6455Compliance, PingPongOpcodes) {
    builder b;
    const uint8_t body[] = {'h', 'i'};
    std::vector<uint8_t> ping = b.build_ping(body, 2, false);
    std::vector<uint8_t> pong = b.build_pong(body, 2, false);
    EXPECT_EQ(ping[0] & 0x0F, static_cast<uint8_t>(opcode::PING));
    EXPECT_EQ(pong[0] & 0x0F, static_cast<uint8_t>(opcode::PONG));
}

TEST(Rfc6455Compliance, InvalidOpcodeRejected) {
    const uint8_t raw[] = {0x8F, 0x00};
    parser p;
    frame_header header;
    std::vector<uint8_t> payload;
    EXPECT_EQ(p.parse(raw, sizeof(raw), header, payload), parse_result::ERROR);
}

TEST(Rfc6455Compliance, FragmentedFramesParseSeparately) {
    builder b;
    const std::string text = "fragmented message";
    const std::vector<uint8_t> frag1 = b.build_text(text.substr(0, 10), false, false);
    const std::vector<uint8_t> frag2 =
        b.build(opcode::CONTINUATION, reinterpret_cast<const uint8_t *>(text.data() + 10),
                text.size() - 10, true, false);

    parser p;
    frame_header header;
    std::vector<uint8_t> payload;

    EXPECT_EQ(p.parse(frag1.data(), frag1.size(), header, payload), parse_result::COMPLETE);
    EXPECT_FALSE(header.fin);
    EXPECT_EQ(header.op, opcode::TEXT);

    p.reset();
    EXPECT_EQ(p.parse(frag2.data(), frag2.size(), header, payload), parse_result::COMPLETE);
    EXPECT_TRUE(header.fin);
    EXPECT_EQ(header.op, opcode::CONTINUATION);
    EXPECT_EQ(std::string(payload.begin(), payload.end()), text.substr(10));
}
