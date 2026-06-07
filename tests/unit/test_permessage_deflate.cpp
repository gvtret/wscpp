#include <gtest/gtest.h>
#include <wscpp/extensions/permessage_deflate.hpp>
#include <string>
#include <vector>

using namespace wscpp::extensions;

#if WSCPP_ENABLE_DEFLATE

TEST(PermessageDeflate, HelloRfc7692Vector) {
    const std::string hello = "Hello";
    std::vector<uint8_t> compressed;
    ASSERT_TRUE(compress_message(reinterpret_cast<const uint8_t*>(hello.data()),
                                 hello.size(), compressed));

    const uint8_t expected[] = {0xf2, 0x48, 0xcd, 0xc9, 0xc9, 0x07, 0x00};
    ASSERT_EQ(compressed.size(), sizeof(expected));
    for (std::size_t i = 0; i < compressed.size(); ++i) {
        EXPECT_EQ(compressed[i], expected[i]) << "at index " << i;
    }
}

TEST(PermessageDeflate, RoundTripText) {
    const std::string input =
        "The quick brown fox jumps over the lazy dog. "
        "RFC 7692 permessage-deflate test payload.";
    std::vector<uint8_t> compressed;
    ASSERT_TRUE(compress_message(reinterpret_cast<const uint8_t*>(input.data()),
                                 input.size(), compressed));

    std::vector<uint8_t> plain;
    ASSERT_TRUE(decompress_message(compressed.data(), compressed.size(), plain));
    EXPECT_EQ(std::string(plain.begin(), plain.end()), input);
}

TEST(PermessageDeflate, ExtensionHeaderDetection) {
    EXPECT_TRUE(header_offers_permessage_deflate(
        "permessage-deflate; client_no_context_takeover"));
    EXPECT_FALSE(header_offers_permessage_deflate(""));
}

#else

TEST(PermessageDeflate, DisabledAtBuildTime) {
    SUCCEED();
}

#endif
