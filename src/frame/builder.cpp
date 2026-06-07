#include <algorithm>
#include <cstring>
#include <random>
#include <wscpp/frame/builder.hpp>

namespace wscpp {
namespace frame {

builder::builder() {
    // Initialize with default masking key
    masking_key_ = {{0x00, 0x00, 0x00, 0x00}};
}

builder::~builder() = default;

std::array<uint8_t, 4> builder::generate_masking_key() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    return {{static_cast<uint8_t>(dis(gen)), static_cast<uint8_t>(dis(gen)),
             static_cast<uint8_t>(dis(gen)), static_cast<uint8_t>(dis(gen))}};
}

void builder::mask_data(uint8_t *data, size_t size, const std::array<uint8_t, 4> &key) {
    for (size_t i = 0; i < size; ++i) {
        data[i] ^= key[i % 4];
    }
}

std::vector<uint8_t> builder::build(opcode op, const uint8_t *data, size_t size, bool fin,
                                    bool mask, const std::array<uint8_t, 4> &masking_key,
                                    bool rsv1) {
    std::vector<uint8_t> frame;

    // Reserve space for header (2 bytes + up to 16 bytes for extended length)
    frame.reserve(2 + 16 + size);

    // First byte: FIN + RSV + Opcode
    uint8_t first_byte = 0;
    if (fin) {
        first_byte |= 0x80;
    }
    if (rsv1) {
        first_byte |= 0x40;
    }
    first_byte |= static_cast<uint8_t>(op);
    frame.push_back(first_byte);

    // Second byte: MASK + Payload length
    uint8_t second_byte = 0;
    if (mask) {
        second_byte |= 0x80;
    }

    if (size <= 125) {
        second_byte |= static_cast<uint8_t>(size);
        frame.push_back(second_byte);
    } else if (size <= 65535) {
        second_byte |= 126;
        frame.push_back(second_byte);
        // 16-bit extended payload length
        frame.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(size & 0xFF));
    } else {
        second_byte |= 127;
        frame.push_back(second_byte);
        // 64-bit extended payload length (big-endian)
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
        }
    }

    // Add masking key if mask is set
    std::array<uint8_t, 4> key = mask ? masking_key : masking_key_;
    if (mask) {
        frame.insert(frame.end(), key.begin(), key.end());
    }

    // Add payload
    std::vector<uint8_t> payload(data, data + size);

    // Mask payload if mask is set
    if (mask) {
        mask_data(payload.data(), payload.size(), key);
    }

    frame.insert(frame.end(), payload.begin(), payload.end());

    return frame;
}

std::vector<uint8_t> builder::build_text(const std::string &text, bool fin, bool mask) {
    return build(opcode::TEXT, reinterpret_cast<const uint8_t *>(text.data()), text.size(), fin,
                 mask, generate_masking_key());
}

std::vector<uint8_t> builder::build_binary(const uint8_t *data, size_t size, bool fin, bool mask) {
    return build(opcode::BINARY, data, size, fin, mask, generate_masking_key());
}

std::vector<uint8_t> builder::build_close(uint16_t status_code, const std::string &reason,
                                          bool mask) {
    // Close frame payload: 2-byte status code + optional reason
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>((status_code >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>(status_code & 0xFF));

    if (!reason.empty()) {
        payload.insert(payload.end(), reason.begin(), reason.end());
    }

    return build(opcode::CLOSE, payload.data(), payload.size(), true, mask, generate_masking_key());
}

std::vector<uint8_t> builder::build_ping(const uint8_t *data, size_t size, bool mask) {
    return build(opcode::PING, data, size, true, mask, generate_masking_key());
}

std::vector<uint8_t> builder::build_pong(const uint8_t *data, size_t size, bool mask) {
    return build(opcode::PONG, data, size, true, mask, generate_masking_key());
}

} // namespace frame
} // namespace wscpp
