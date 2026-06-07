#ifndef WSCPP_DETAIL_UTF8_HPP
#define WSCPP_DETAIL_UTF8_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace wscpp {
namespace detail {

inline bool is_valid_utf8(const uint8_t *data, std::size_t len) {
    std::size_t i = 0;
    while (i < len) {
        const uint8_t c = data[i];
        if (c <= 0x7F) {
            ++i;
            continue;
        }
        if (c >= 0xC2 && c <= 0xDF) {
            if (i + 1 >= len || (data[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            i += 2;
            continue;
        }
        if (c >= 0xE0 && c <= 0xEF) {
            if (i + 2 >= len) {
                return false;
            }
            const uint8_t c1 = data[i + 1];
            const uint8_t c2 = data[i + 2];
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) {
                return false;
            }
            if (c == 0xE0 && c1 < 0xA0) {
                return false;
            }
            if (c == 0xED && c1 >= 0xA0) {
                return false;
            }
            i += 3;
            continue;
        }
        if (c >= 0xF0 && c <= 0xF4) {
            if (i + 3 >= len) {
                return false;
            }
            const uint8_t c1 = data[i + 1];
            const uint8_t c2 = data[i + 2];
            const uint8_t c3 = data[i + 3];
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) {
                return false;
            }
            if (c == 0xF0 && c1 < 0x90) {
                return false;
            }
            if (c == 0xF4 && c1 >= 0x90) {
                return false;
            }
            i += 4;
            continue;
        }
        return false;
    }
    return true;
}

inline bool is_valid_utf8(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        return true;
    }
    return is_valid_utf8(data.data(), data.size());
}

} // namespace detail
} // namespace wscpp

#endif
