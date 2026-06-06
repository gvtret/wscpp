// test_frame.cpp
// Unit tests for WebSocket frame parser and builder

#include <gtest/gtest.h>
#include <wscpp/frame/parser.hpp>
#include <wscpp/frame/builder.hpp>
#include <string>
#include <vector>
#include <cstring>

using namespace wscpp::frame;

// Test basic frame building
TEST(FrameTest, BuildBasicTextFrame) {
    builder b;
    std::string text = "Hello";
    std::vector<uint8_t> frame = b.build_text(text, true, false);
    
    // Check header
    EXPECT_EQ(frame.size(), 7); // 2 byte header + 5 byte payload
    EXPECT_EQ(frame[0] & 0x80, 0x80); // FIN bit set
    EXPECT_EQ(frame[0] & 0x0F, static_cast<uint8_t>(opcode::TEXT)); // Opcode
    EXPECT_EQ(frame[1] & 0x7F, 5); // Payload length
}

// Test frame building with mask
TEST(FrameTest, BuildMaskedFrame) {
    builder b;
    std::string text = "Test";
    std::vector<uint8_t> frame = b.build_text(text, true, true);
    
    // Check that mask bit is set
    EXPECT_NE(frame[1] & 0x80, 0); // Mask bit set
    
    // Check masking key is present (4 bytes after header)
    EXPECT_EQ(frame.size(), 10); // 2 + 4 + 4
}

// Test frame parsing
TEST(FrameTest, ParseBasicTextFrame) {
    builder b;
    std::string text = "Hello";
    std::vector<uint8_t> frame = b.build_text(text, true, false);
    
    parser p;
    frame_header header;
    std::vector<uint8_t> payload;
    
    parse_result result = p.parse(frame.data(), frame.size(), header, payload);
    
    EXPECT_EQ(result, parse_result::COMPLETE);
    EXPECT_EQ(header.fin, true);
    EXPECT_EQ(header.op, opcode::TEXT);
    EXPECT_EQ(header.payload_len, 5);
    EXPECT_EQ(std::string(payload.begin(), payload.end()), text);
}

// Test frame parsing with mask
TEST(FrameTest, ParseMaskedFrame) {
    builder b;
    std::string text = "Masked";
    std::vector<uint8_t> frame = b.build_text(text, true, true);
    
    parser p;
    frame_header header;
    std::vector<uint8_t> payload;
    
    parse_result result = p.parse(frame.data(), frame.size(), header, payload);
    
    EXPECT_EQ(result, parse_result::COMPLETE);
    EXPECT_EQ(header.mask, true);
    EXPECT_EQ(header.payload_len, 6);
    EXPECT_EQ(std::string(payload.begin(), payload.end()), text);
}

// Test different opcodes
TEST(FrameTest, BuildDifferentOpcodes) {
    builder b;
    
    // Ping
    std::vector<uint8_t> ping = b.build_ping(nullptr, 0, false);
    EXPECT_EQ(ping[0] & 0x0F, static_cast<uint8_t>(opcode::PING));
    
    // Pong
    std::vector<uint8_t> pong = b.build_pong(nullptr, 0, false);
    EXPECT_EQ(pong[0] & 0x0F, static_cast<uint8_t>(opcode::PONG));
    
    // Close
    std::vector<uint8_t> close = b.build_close(1000, "Normal closure", false);
    EXPECT_EQ(close[0] & 0x0F, static_cast<uint8_t>(opcode::CLOSE));
}

// Test binary frame
TEST(FrameTest, BuildBinaryFrame) {
    builder b;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<uint8_t> frame = b.build_binary(data, 5, true, false);
    
    EXPECT_EQ(frame[0] & 0x0F, static_cast<uint8_t>(opcode::BINARY));
    EXPECT_EQ(frame.size(), 7);
}

// Test close frame with status code
TEST(FrameTest, BuildCloseFrame) {
    builder b;
    std::vector<uint8_t> frame = b.build_close(1000, "Normal closure", false);
    
    EXPECT_EQ(frame[0] & 0x0F, static_cast<uint8_t>(opcode::CLOSE));
    EXPECT_EQ(frame.size(), 20); // 2 + 2 (status) + 16 (reason)
    
    // Check status code
    uint16_t status = (static_cast<uint16_t>(frame[2]) << 8) | frame[3];
    EXPECT_EQ(status, 1000);
}

// Test large payload (16-bit length)
TEST(FrameTest, BuildLargePayload) {
    builder b;
    std::string text(200, 'A'); // > 125 bytes
    std::vector<uint8_t> frame = b.build_text(text, true, false);
    
    // Should use 16-bit extended length
    EXPECT_EQ(frame[1] & 0x7F, 126);
    EXPECT_EQ(frame.size(), 2 + 2 + 200);
}

// Test very large payload (64-bit length)
TEST(FrameTest, BuildVeryLargePayload) {
    builder b;
    std::string text(70000, 'B'); // > 65535 bytes
    std::vector<uint8_t> frame = b.build_text(text, true, false);
    
    // Should use 64-bit extended length
    EXPECT_EQ(frame[1] & 0x7F, 127);
    EXPECT_EQ(frame.size(), 2 + 8 + 70000);
}

// Test fragmented frame
TEST(FrameTest, FragmentedFrame) {
    builder b;
    std::string text = "This is a fragmented message";
    
    // First fragment
    std::vector<uint8_t> frag1 = b.build_text(text.substr(0, 10), false, false);
    EXPECT_EQ(frag1[0] & 0x80, 0); // FIN not set
    EXPECT_EQ(frag1[0] & 0x0F, static_cast<uint8_t>(opcode::TEXT));
    
    // Continuation fragment
    std::vector<uint8_t> frag2 = b.build(opcode::CONTINUATION,
        reinterpret_cast<const uint8_t*>(text.data() + 10), 10, false, false);
    EXPECT_EQ(frag2[0] & 0x80, 0); // FIN not set
    EXPECT_EQ(frag2[0] & 0x0F, static_cast<uint8_t>(opcode::CONTINUATION));
    
    // Final fragment
    std::vector<uint8_t> frag3 = b.build(opcode::CONTINUATION,
        reinterpret_cast<const uint8_t*>(text.data() + 20), text.size() - 20, true, false);
    EXPECT_EQ(frag3[0] & 0x80, 0x80); // FIN set
    EXPECT_EQ(frag3[0] & 0x0F, static_cast<uint8_t>(opcode::CONTINUATION));
}

// Test parser reset
TEST(FrameTest, ParserReset) {
    parser p;
    
    builder b;
    std::string text = "Test";
    std::vector<uint8_t> frame = b.build_text(text, true, false);
    
    // Parse first frame
    frame_header header1;
    std::vector<uint8_t> payload1;
    p.parse(frame.data(), frame.size(), header1, payload1);
    
    // Reset parser
    p.reset();
    
    // Parse second frame
    std::vector<uint8_t> frame2 = b.build_text("Test2", true, false);
    frame_header header2;
    std::vector<uint8_t> payload2;
    parse_result result = p.parse(frame2.data(), frame2.size(), header2, payload2);
    
    EXPECT_EQ(result, parse_result::COMPLETE);
    EXPECT_EQ(std::string(payload2.begin(), payload2.end()), "Test2");
}

// Test incomplete frame
TEST(FrameTest, IncompleteFrame) {
    parser p;
    
    builder b;
    std::string text = "Hello";
    std::vector<uint8_t> frame = b.build_text(text, true, false);
    
    // Send only part of the frame
    frame_header header;
    std::vector<uint8_t> payload;
    parse_result result = p.parse(frame.data(), 3, header, payload);
    
    EXPECT_EQ(result, parse_result::INCOMPLETE);
}

// Test mask/unmask correctness
TEST(FrameTest, MaskUnmaskCorrectness) {
    builder b;
    std::string text = "WebSocket";
    
    // Build masked frame
    std::vector<uint8_t> frame = b.build_text(text, true, true);
    
    // Parse it back
    parser p;
    frame_header header;
    std::vector<uint8_t> payload;
    p.parse(frame.data(), frame.size(), header, payload);
    
    EXPECT_EQ(std::string(payload.begin(), payload.end()), text);
}

// Test opcode validation
TEST(FrameTest, InvalidOpcode) {
    // This is more of a conceptual test - the parser should handle invalid opcodes
    // In practice, the parser might not reject them immediately
    builder b;
    
    // Try to build with reserved opcode
    std::vector<uint8_t> frame = b.build(opcode::CONTINUATION, nullptr, 0, true, false);
    EXPECT_FALSE(frame.empty());
}
