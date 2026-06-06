#ifndef WSCPP_FRAME_PARSER_HPP
#define WSCPP_FRAME_PARSER_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <array>

namespace wscpp {
namespace frame {

// WebSocket opcode types
enum class opcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT         = 0x1,
    BINARY       = 0x2,
    CLOSE        = 0x8,
    PING         = 0x9,
    PONG         = 0xA
};

// WebSocket frame structure
struct frame_header {
    bool fin;              // Final fragment
    bool rsv1;             // Reserved bits
    bool rsv2;
    bool rsv3;
    opcode op;             // Opcode
    bool mask;             // Mask flag
    uint64_t payload_len;  // Payload length
    std::array<uint8_t, 4> masking_key; // Masking key (if mask is true)
};

// Parse result
enum class parse_result {
    INCOMPLETE,  // Need more data
    COMPLETE,    // Frame fully parsed
    ERROR        // Parse error
};

// WebSocket frame parser
class parser {
public:
    parser();
    ~parser();
    
    // Parse incoming data
    parse_result parse(const uint8_t* data, size_t size, frame_header& header, std::vector<uint8_t>& payload);
    
    // Reset parser state
    void reset();
    
    // Get current state
    size_t bytes_needed() const;
    bool is_frame_complete() const;
    
private:
    enum class state {
        HEADER,
        PAYLOAD_LENGTH_EXT,
        MASKING_KEY,
        PAYLOAD
    };
    
    state state_;
    frame_header header_;
    std::vector<uint8_t> buffer_;
    size_t bytes_read_;
    size_t payload_remaining_;
    bool mask_set_;
    
    // Helper functions
    void unmask_payload(std::vector<uint8_t>& payload);
    parse_result parse_header(const uint8_t* data, size_t size, size_t& consumed);
    parse_result parse_payload_length(const uint8_t* data, size_t size, size_t& consumed);
    parse_result parse_masking_key(const uint8_t* data, size_t size, size_t& consumed);
    parse_result parse_payload(const uint8_t* data, size_t size, size_t& consumed);
};

} // namespace frame
} // namespace wscpp

#endif // WSCPP_FRAME_PARSER_HPP
