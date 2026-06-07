#ifndef WSCPP_FRAME_BUILDER_HPP
#define WSCPP_FRAME_BUILDER_HPP

/**
 * @file builder.hpp
 * @brief RFC 6455 WebSocket frame builder.
 */

#include "parser.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace wscpp {
namespace frame {

/**
 * @brief Builds RFC 6455 WebSocket frames (with optional masking).
 */
class builder {
  public:
    builder();
    ~builder();

    /** @brief Build raw frame with given opcode and payload. */
    std::vector<uint8_t> build(opcode op, const uint8_t *data, size_t size, bool fin = true,
                               bool mask = false,
                               const std::array<uint8_t, 4> &masking_key = {{0, 0, 0, 0}},
                               bool rsv1 = false);

    /** @brief Build text (UTF-8) frame. */
    std::vector<uint8_t> build_text(const std::string &text, bool fin = true, bool mask = false);

    /** @brief Build binary frame. */
    std::vector<uint8_t> build_binary(const uint8_t *data, size_t size, bool fin = true,
                                      bool mask = false);

    /** @brief Build close frame with status code and optional reason. */
    std::vector<uint8_t> build_close(uint16_t status_code, const std::string &reason = "",
                                     bool mask = false);

    /** @brief Build ping frame. */
    std::vector<uint8_t> build_ping(const uint8_t *data, size_t size, bool mask = false);

    /** @brief Build pong frame. */
    std::vector<uint8_t> build_pong(const uint8_t *data, size_t size, bool mask = false);

    /** @brief Generate random RFC 6455 masking key. */
    std::array<uint8_t, 4> generate_masking_key();

  private:
    void mask_data(uint8_t *data, size_t size, const std::array<uint8_t, 4> &key);

    std::array<uint8_t, 4> masking_key_;
};

} // namespace frame
} // namespace wscpp

#endif // WSCPP_FRAME_BUILDER_HPP
