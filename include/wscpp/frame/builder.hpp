#ifndef WSCPP_FRAME_BUILDER_HPP
#define WSCPP_FRAME_BUILDER_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include "parser.hpp"

namespace wscpp {
namespace frame {

// WebSocket frame builder
class builder {
public:
    builder();
    ~builder();
    
    // Build a frame
    std::vector<uint8_t> build(
        opcode op,
        const uint8_t* data,
        size_t size,
        bool fin = true,
        bool mask = false,
        const std::array<uint8_t, 4>& masking_key = {{0, 0, 0, 0}}
    );
    
    // Build a text frame
    std::vector<uint8_t> build_text(
        const std::string& text,
        bool fin = true,
        bool mask = false
    );
    
    // Build a binary frame
    std::vector<uint8_t> build_binary(
        const uint8_t* data,
        size_t size,
        bool fin = true,
        bool mask = false
    );
    
    // Build a close frame
    std::vector<uint8_t> build_close(
        uint16_t status_code,
        const std::string& reason = "",
        bool mask = false
    );
    
    // Build a ping frame
    std::vector<uint8_t> build_ping(
        const uint8_t* data,
        size_t size,
        bool mask = false
    );
    
    // Build a pong frame
    std::vector<uint8_t> build_pong(
        const uint8_t* data,
        size_t size,
        bool mask = false
    );
    
private:
    std::array<uint8_t, 4> generate_masking_key();
    void mask_data(uint8_t* data, size_t size, const std::array<uint8_t, 4>& key);
    
    std::array<uint8_t, 4> masking_key_;
};

} // namespace frame
} // namespace wscpp

#endif // WSCPP_FRAME_BUILDER_HPP
