#include <algorithm>
#include <stdexcept>
#include <wscpp/frame/builder.hpp>
#include <wscpp/frame/parser.hpp>

namespace wscpp {
namespace frame {

parser::parser() : state_(state::HEADER), bytes_read_(0), payload_remaining_(0), mask_set_(false) {
    header_.fin = false;
    header_.rsv1 = false;
    header_.rsv2 = false;
    header_.rsv3 = false;
    header_.op = opcode::CONTINUATION;
    header_.mask = false;
    header_.payload_len = 0;
    header_.masking_key = {{0, 0, 0, 0}};
}

parser::~parser() = default;

void parser::reset() {
    state_ = state::HEADER;
    bytes_read_ = 0;
    payload_remaining_ = 0;
    mask_set_ = false;
    buffer_.clear();
    header_.fin = false;
    header_.rsv1 = false;
    header_.rsv2 = false;
    header_.rsv3 = false;
    header_.op = opcode::CONTINUATION;
    header_.mask = false;
    header_.payload_len = 0;
    header_.masking_key = {{0, 0, 0, 0}};
}

size_t parser::bytes_needed() const {
    switch (state_) {
    case state::HEADER:
        return 2 - bytes_read_;
    case state::PAYLOAD_LENGTH_EXT:
        if (header_.payload_len <= 125) {
            return 0;
        } else if (header_.payload_len <= 65535) {
            return 2;
        } else {
            return 8;
        }
    case state::MASKING_KEY:
        return header_.mask ? 4 : 0;
    case state::PAYLOAD:
        return payload_remaining_;
    default:
        return 0;
    }
}

bool parser::is_frame_complete() const {
    return state_ == state::HEADER && buffer_.empty();
}

parse_result parser::parse(const uint8_t *data, size_t size, frame_header &header,
                           std::vector<uint8_t> &payload) {
    size_t consumed = 0;

    while (consumed < size) {
        size_t prev_consumed = consumed;

        switch (state_) {
        case state::HEADER:
            if (parse_header(data + consumed, size - consumed, consumed) == parse_result::ERROR) {
                return parse_result::ERROR;
            }
            break;

        case state::PAYLOAD_LENGTH_EXT:
            if (parse_payload_length(data + consumed, size - consumed, consumed) ==
                parse_result::ERROR) {
                return parse_result::ERROR;
            }
            break;

        case state::MASKING_KEY:
            if (parse_masking_key(data + consumed, size - consumed, consumed) ==
                parse_result::ERROR) {
                return parse_result::ERROR;
            }
            break;

        case state::PAYLOAD: {
            const parse_result payload_result =
                parse_payload(data + consumed, size - consumed, consumed);
            if (payload_result == parse_result::ERROR) {
                return parse_result::ERROR;
            }
            if (payload_result == parse_result::COMPLETE) {
                header = header_;
                payload = std::move(buffer_);
                reset();
                return parse_result::COMPLETE;
            }
            break;
        }
        }

        if (consumed == prev_consumed) {
            break;
        }
    }

    return parse_result::INCOMPLETE;
}

parse_result parser::parse_header(const uint8_t *data, size_t size, size_t &consumed) {
    if (size < 2) {
        return parse_result::INCOMPLETE;
    }

    uint8_t first_byte = data[0];
    uint8_t second_byte = data[1];

    header_.fin = (first_byte & 0x80) != 0;
    header_.rsv1 = (first_byte & 0x40) != 0;
    header_.rsv2 = (first_byte & 0x20) != 0;
    header_.rsv3 = (first_byte & 0x10) != 0;
    header_.op = static_cast<opcode>(first_byte & 0x0F);
    header_.mask = (second_byte & 0x80) != 0;
    header_.payload_len = second_byte & 0x7F;

    consumed += 2;

    // Validate opcode (RFC 6455: 0,1,2,8,9,10 only)
    const uint8_t op_val = static_cast<uint8_t>(header_.op);
    if (!((op_val <= 0x02) || (op_val >= 0x08 && op_val <= 0x0A))) {
        return parse_result::ERROR;
    }

    if (header_.payload_len <= 125) {
        state_ = header_.mask ? state::MASKING_KEY : state::PAYLOAD;
        payload_remaining_ = header_.payload_len;
        buffer_.clear();
        if (header_.payload_len > 0) {
            buffer_.reserve(static_cast<std::size_t>(header_.payload_len));
        }
    } else if (header_.payload_len == 126 || header_.payload_len == 127) {
        state_ = state::PAYLOAD_LENGTH_EXT;
        payload_remaining_ = 0;
    }

    return parse_result::INCOMPLETE;
}

parse_result parser::parse_payload_length(const uint8_t *data, size_t size, size_t &consumed) {
    if (header_.payload_len == 126) {
        // 16-bit extended payload length
        if (size < 2) {
            return parse_result::INCOMPLETE;
        }
        header_.payload_len = (static_cast<uint64_t>(data[0]) << 8) | data[1];
        consumed += 2;
    } else {
        // 64-bit extended payload length
        if (size < 8) {
            return parse_result::INCOMPLETE;
        }
        header_.payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            header_.payload_len = (header_.payload_len << 8) | data[i];
        }
        consumed += 8;
    }

    // Validate payload length
    if (header_.payload_len > 0x7FFFFFFFFFFFFFFF) {
        return parse_result::ERROR;
    }

    state_ = header_.mask ? state::MASKING_KEY : state::PAYLOAD;
    payload_remaining_ = header_.payload_len;
    buffer_.clear();
    if (header_.payload_len > 0) {
        buffer_.reserve(static_cast<std::size_t>(header_.payload_len));
    }
    return parse_result::INCOMPLETE;
}

parse_result parser::parse_masking_key(const uint8_t *data, size_t size, size_t &consumed) {
    if (size < 4) {
        return parse_result::INCOMPLETE;
    }

    header_.masking_key = {{data[0], data[1], data[2], data[3]}};

    consumed += 4;
    state_ = state::PAYLOAD;
    payload_remaining_ = header_.payload_len;
    buffer_.reserve(header_.payload_len);

    return parse_result::INCOMPLETE;
}

parse_result parser::parse_payload(const uint8_t *data, size_t size, size_t &consumed) {
    if (payload_remaining_ == 0) {
        state_ = state::HEADER;
        return parse_result::COMPLETE;
    }

    size_t to_read = std::min(payload_remaining_, size);

    // Copy payload
    buffer_.insert(buffer_.end(), data, data + to_read);

    consumed += to_read;
    payload_remaining_ -= to_read;

    if (payload_remaining_ == 0) {
        // Unmask payload if mask is set
        if (header_.mask) {
            unmask_payload(buffer_);
        }
        state_ = state::HEADER;
        return parse_result::COMPLETE;
    }

    return parse_result::INCOMPLETE;
}

void parser::unmask_payload(std::vector<uint8_t> &payload) {
    if (!header_.mask || payload.empty()) {
        return;
    }

    for (size_t i = 0; i < payload.size(); ++i) {
        payload[i] ^= header_.masking_key[i % 4];
    }
}

} // namespace frame
} // namespace wscpp
