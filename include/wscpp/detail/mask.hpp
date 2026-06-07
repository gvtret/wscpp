#ifndef WSCPP_DETAIL_MASK_HPP
#define WSCPP_DETAIL_MASK_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ >= 8
using wscpp_mask_uintptr = std::uint64_t;
#else
using wscpp_mask_uintptr = std::uint32_t;
#endif

namespace wscpp {
namespace detail {

/** RFC 6455 payload mask/unmask in place (offset 0). */
inline void apply_mask(uint8_t *data, std::size_t size, const uint8_t key[4]) {
    if (size == 0) {
        return;
    }

    std::size_t i = 0;

    while (i < size && (i & 3) != 0) {
        data[i] ^= key[i & 3];
        ++i;
    }

    uint32_t mask_word = 0;
    std::memcpy(&mask_word, key, 4);

    for (; i + 4 <= size; i += 4) {
        uint32_t word = 0;
        std::memcpy(&word, data + i, 4);
        word ^= mask_word;
        std::memcpy(data + i, &word, 4);
    }

    for (; i < size; ++i) {
        data[i] ^= key[i & 3];
    }
}

/** Copy @p size bytes and unmask into @p dst starting at @p dst_offset in the frame. */
inline void copy_and_unmask(uint8_t *dst, const uint8_t *src, std::size_t size,
                            std::size_t dst_offset, const uint8_t key[4]) {
    if (size == 0) {
        return;
    }

    std::size_t i = 0;
    std::size_t pos = dst_offset;

    while (i < size) {
        if ((pos & 3) == 0 && i + 4 <= size) {
            uint32_t mask_word = 0;
            std::memcpy(&mask_word, key, 4);
            uint32_t word = 0;
            std::memcpy(&word, src + i, 4);
            word ^= mask_word;
            std::memcpy(dst + i, &word, 4);
            i += 4;
            pos += 4;
            continue;
        }
        dst[i] = static_cast<uint8_t>(src[i] ^ key[pos & 3]);
        ++i;
        ++pos;
    }
}

} // namespace detail
} // namespace wscpp

#endif // WSCPP_DETAIL_MASK_HPP
