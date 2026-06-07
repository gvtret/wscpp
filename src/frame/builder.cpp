#include <cstring>
#include <random>
#include <wscpp/detail/mask.hpp>
#include <wscpp/frame/builder.hpp>

namespace wscpp {
namespace frame {

namespace {

void append_payload_length(std::vector<uint8_t> &frame, uint8_t &second_byte, size_t size) {
    if (size <= 125) {
        second_byte |= static_cast<uint8_t>(size);
        frame.push_back(second_byte);
    } else if (size <= 65535) {
        second_byte |= 126;
        frame.push_back(second_byte);
        frame.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(size & 0xFF));
    } else {
        second_byte |= 127;
        frame.push_back(second_byte);
        frame.push_back(static_cast<uint8_t>((size >> 56) & 0xFF));
        frame.push_back(static_cast<uint8_t>((size >> 48) & 0xFF));
        frame.push_back(static_cast<uint8_t>((size >> 40) & 0xFF));
        frame.push_back(static_cast<uint8_t>((size >> 32) & 0xFF));
        frame.push_back(static_cast<uint8_t>((size >> 24) & 0xFF));
        frame.push_back(static_cast<uint8_t>((size >> 16) & 0xFF));
        frame.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(size & 0xFF));
    }
}

} // namespace

builder::builder() {
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
    detail::apply_mask(data, size, key.data());
}

void builder::build_into(std::vector<uint8_t> &out, opcode op, const uint8_t *data, size_t size,
                         bool fin, bool mask, const std::array<uint8_t, 4> &masking_key,
                         bool rsv1) {
    out.clear();
    out.reserve(2 + 16 + (mask ? 4 : 0) + size);

    uint8_t first_byte = 0;
    if (fin) {
        first_byte |= 0x80;
    }
    if (rsv1) {
        first_byte |= 0x40;
    }
    first_byte |= static_cast<uint8_t>(op);
    out.push_back(first_byte);

    uint8_t second_byte = 0;
    if (mask) {
        second_byte |= 0x80;
    }
    append_payload_length(out, second_byte, size);

    const std::array<uint8_t, 4> &key = mask ? masking_key : masking_key_;
    if (mask) {
        out.insert(out.end(), key.begin(), key.end());
    }

    const std::size_t payload_offset = out.size();
    out.resize(payload_offset + size);
    std::memcpy(out.data() + payload_offset, data, size);
    if (mask) {
        mask_data(out.data() + payload_offset, size, key);
    }
}

std::vector<uint8_t> builder::build(opcode op, const uint8_t *data, size_t size, bool fin,
                                    bool mask, const std::array<uint8_t, 4> &masking_key,
                                    bool rsv1) {
    std::vector<uint8_t> frame;
    build_into(frame, op, data, size, fin, mask, masking_key, rsv1);
    return frame;
}

void builder::build_close_into(std::vector<uint8_t> &out, uint16_t status_code,
                               const std::string &reason, bool mask) {
    uint8_t payload[2];
    payload[0] = static_cast<uint8_t>((status_code >> 8) & 0xFF);
    payload[1] = static_cast<uint8_t>(status_code & 0xFF);

    if (reason.empty()) {
        const std::array<uint8_t, 4> key = mask ? generate_masking_key() : masking_key_;
        build_into(out, opcode::CLOSE, payload, 2, true, mask, key);
        return;
    }

    out.clear();
    out.reserve(2 + 16 + (mask ? 4 : 0) + 2 + reason.size());

    uint8_t first_byte = 0x80 | static_cast<uint8_t>(opcode::CLOSE);
    out.push_back(first_byte);

    const size_t payload_size = 2 + reason.size();
    uint8_t second_byte = mask ? 0x80 : 0;
    append_payload_length(out, second_byte, payload_size);

    const std::array<uint8_t, 4> key = mask ? generate_masking_key() : masking_key_;
    if (mask) {
        out.insert(out.end(), key.begin(), key.end());
    }

    const std::size_t payload_offset = out.size();
    out.resize(payload_offset + payload_size);
    out[payload_offset] = payload[0];
    out[payload_offset + 1] = payload[1];
    std::memcpy(out.data() + payload_offset + 2, reason.data(), reason.size());
    if (mask) {
        mask_data(out.data() + payload_offset, payload_size, key);
    }
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
    std::vector<uint8_t> frame;
    build_close_into(frame, status_code, reason, mask);
    return frame;
}

std::vector<uint8_t> builder::build_ping(const uint8_t *data, size_t size, bool mask) {
    return build(opcode::PING, data, size, true, mask, generate_masking_key());
}

std::vector<uint8_t> builder::build_pong(const uint8_t *data, size_t size, bool mask) {
    return build(opcode::PONG, data, size, true, mask, generate_masking_key());
}

} // namespace frame
} // namespace wscpp
