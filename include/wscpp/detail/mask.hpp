#ifndef WSCPP_DETAIL_MASK_HPP
#define WSCPP_DETAIL_MASK_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace wscpp {
namespace detail {

inline std::uint32_t load_u32_le(const uint8_t *p) {
    std::uint32_t v = 0;
    std::memcpy(&v, p, 4);
    return v;
}

inline void store_u32_le(uint8_t *p, std::uint32_t v) {
    std::memcpy(p, &v, 4);
}

inline std::uint64_t load_u64_le(const uint8_t *p) {
    std::uint64_t v = 0;
    std::memcpy(&v, p, 8);
    return v;
}

inline void store_u64_le(uint8_t *p, std::uint64_t v) {
    std::memcpy(p, &v, 8);
}

inline std::uint64_t mask_key64(std::uint32_t key) {
    return static_cast<std::uint64_t>(key) | (static_cast<std::uint64_t>(key) << 32);
}

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

    const std::uint32_t key32 = load_u32_le(key);
    const std::uint64_t key64 = mask_key64(key32);

    while (i + 64 <= size) {
        store_u64_le(data + i, load_u64_le(data + i) ^ key64);
        store_u64_le(data + i + 8, load_u64_le(data + i + 8) ^ key64);
        store_u64_le(data + i + 16, load_u64_le(data + i + 16) ^ key64);
        store_u64_le(data + i + 24, load_u64_le(data + i + 24) ^ key64);
        store_u64_le(data + i + 32, load_u64_le(data + i + 32) ^ key64);
        store_u64_le(data + i + 40, load_u64_le(data + i + 40) ^ key64);
        store_u64_le(data + i + 48, load_u64_le(data + i + 48) ^ key64);
        store_u64_le(data + i + 56, load_u64_le(data + i + 56) ^ key64);
        i += 64;
    }

    while (i + 32 <= size) {
        store_u64_le(data + i, load_u64_le(data + i) ^ key64);
        store_u64_le(data + i + 8, load_u64_le(data + i + 8) ^ key64);
        store_u64_le(data + i + 16, load_u64_le(data + i + 16) ^ key64);
        store_u64_le(data + i + 24, load_u64_le(data + i + 24) ^ key64);
        i += 32;
    }

    while (i + 8 <= size) {
        store_u64_le(data + i, load_u64_le(data + i) ^ key64);
        i += 8;
    }

    while (i + 4 <= size) {
        store_u32_le(data + i, load_u32_le(data + i) ^ key32);
        i += 4;
    }

    while (i < size) {
        data[i] ^= key[i & 3];
        ++i;
    }
}

/** Copy @p size bytes and unmask into @p dst starting at @p dst_offset in the frame. */
inline void copy_and_unmask(uint8_t *dst, const uint8_t *src, std::size_t size,
                            std::size_t dst_offset, const uint8_t key[4]) {
    if (size == 0) {
        return;
    }

    const std::uint32_t key32 = load_u32_le(key);
    const std::uint64_t key64 = mask_key64(key32);

    std::size_t i = 0;
    std::size_t pos = dst_offset;

    while (i < size) {
        if ((pos & 7) == 0 && i + 8 <= size) {
            store_u64_le(dst + i, load_u64_le(src + i) ^ key64);
            i += 8;
            pos += 8;
            continue;
        }
        if ((pos & 3) == 0 && i + 4 <= size) {
            store_u32_le(dst + i, load_u32_le(src + i) ^ key32);
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
