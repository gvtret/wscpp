#ifndef WSCPP_EXTENSIONS_PERMESSAGE_DEFLATE_HPP
#define WSCPP_EXTENSIONS_PERMESSAGE_DEFLATE_HPP

/**
 * @file permessage_deflate.hpp
 * @brief RFC 7692 permessage-deflate compress/decompress helpers.
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wscpp {
namespace extensions {

/** @brief RFC 7692 client/server offer string (no context takeover). */
const char* permessage_deflate_offer();

/** @brief True if header value contains permessage-deflate extension. */
bool header_offers_permessage_deflate(const std::string& extensions_header);

/**
 * @brief Compress message payload (raw DEFLATE, strip 0x00 0x00 0xff 0xff tail).
 * @return false on zlib failure or invalid tail.
 */
bool compress_message(const uint8_t* data, std::size_t size, std::vector<uint8_t>& out);

/**
 * @brief Decompress RSV1 message payload (append tail before inflate).
 * @return false on zlib failure.
 */
bool decompress_message(const uint8_t* data, std::size_t size, std::vector<uint8_t>& out);

} // namespace extensions
} // namespace wscpp

#endif
