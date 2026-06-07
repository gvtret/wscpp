#ifndef WSCPP_FRAME_PARSER_HPP
#define WSCPP_FRAME_PARSER_HPP

/**
 * @file parser.hpp
 * @brief RFC 6455 WebSocket frame parser.
 */

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace wscpp {
namespace frame {

/** @brief WebSocket frame opcode (RFC 6455). */
enum class opcode : uint8_t {
    CONTINUATION = 0x0, ///< Continuation frame
    TEXT = 0x1,         ///< Text frame
    BINARY = 0x2,       ///< Binary frame
    CLOSE = 0x8,        ///< Connection close
    PING = 0x9,         ///< Ping
    PONG = 0xA          ///< Pong
};

/** @brief Parsed WebSocket frame header fields. */
struct frame_header {
    bool fin;                           ///< FIN bit
    bool rsv1;                          ///< RSV1 reserved
    bool rsv2;                          ///< RSV2 reserved
    bool rsv3;                          ///< RSV3 reserved
    opcode op;                          ///< Frame opcode
    bool mask;                          ///< Payload masked (client-to-server)
    uint64_t payload_len;               ///< Payload length in bytes
    std::array<uint8_t, 4> masking_key; ///< Masking key when mask is true
};

/** @brief Incremental parse status. */
enum class parse_result {
    INCOMPLETE, ///< Need more bytes
    COMPLETE,   ///< One full frame parsed
    ERROR       ///< Protocol error
};

/**
 * @brief Incremental RFC 6455 frame parser.
 */
class parser {
  public:
    parser();
    ~parser();

    /**
     * @brief Feed bytes into parser.
     * @return COMPLETE when one frame is ready in @p header and @p payload.
     */
    parse_result parse(const uint8_t *data, size_t size, frame_header &header,
                       std::vector<uint8_t> &payload);

    /** @brief Reset parser for next frame. */
    void reset();

    /** @brief Bytes still needed to advance current state. */
    size_t bytes_needed() const;

    /** @brief True when between frames. */
    bool is_frame_complete() const;

  private:
    enum class state { HEADER, PAYLOAD_LENGTH_EXT, MASKING_KEY, PAYLOAD };

    state state_;
    frame_header header_;
    std::vector<uint8_t> buffer_;
    size_t bytes_read_;
    size_t payload_remaining_;
    bool mask_set_;

    void unmask_payload(std::vector<uint8_t> &payload);
    parse_result parse_header(const uint8_t *data, size_t size, size_t &consumed);
    parse_result parse_payload_length(const uint8_t *data, size_t size, size_t &consumed);
    parse_result parse_masking_key(const uint8_t *data, size_t size, size_t &consumed);
    parse_result parse_payload(const uint8_t *data, size_t size, size_t &consumed);
};

} // namespace frame
} // namespace wscpp

#endif // WSCPP_FRAME_PARSER_HPP
