#include <gtest/gtest.h>
#include <wscpp/detail/utf8.hpp>

using wscpp::detail::is_valid_utf8;

TEST(Utf8Test, AcceptsAscii) {
    const uint8_t data[] = {'H', 'e', 'l', 'l', 'o'};
    EXPECT_TRUE(is_valid_utf8(data, sizeof(data)));
}

TEST(Utf8Test, AcceptsValidMultibyte) {
    const uint8_t data[] = {0xC2, 0xA9};
    EXPECT_TRUE(is_valid_utf8(data, sizeof(data)));
}

TEST(Utf8Test, RejectsOverlongEncoding) {
    const uint8_t data[] = {0xC0, 0x80};
    EXPECT_FALSE(is_valid_utf8(data, sizeof(data)));
}

TEST(Utf8Test, RejectsSurrogateHalf) {
    const uint8_t data[] = {0xED, 0xA0, 0x80};
    EXPECT_FALSE(is_valid_utf8(data, sizeof(data)));
}

TEST(Utf8Test, RejectsTruncatedSequence) {
    const uint8_t data[] = {0xE2, 0x82};
    EXPECT_FALSE(is_valid_utf8(data, sizeof(data)));
}

TEST(Utf8Test, RejectsInvalidLeadByte) {
    const uint8_t data[] = {0xFF};
    EXPECT_FALSE(is_valid_utf8(data, sizeof(data)));
}

TEST(Utf8Test, EmptyIsValid) {
    EXPECT_TRUE(is_valid_utf8(static_cast<const uint8_t*>(nullptr), 0));
}
