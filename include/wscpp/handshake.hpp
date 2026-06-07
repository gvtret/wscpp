#ifndef WSCPP_HANDSHAKE_HPP
#define WSCPP_HANDSHAKE_HPP

/**
 * @file handshake.hpp
 * @brief RFC 6455 HTTP WebSocket opening handshake helpers.
 */

#include <map>
#include <string>

namespace wscpp {
namespace handshake {

/** @brief Generate random Sec-WebSocket-Key (Base64). */
std::string generate_key();

/** @brief Compute Sec-WebSocket-Accept from client key. */
std::string compute_accept(const std::string& key);

/** @brief Build HTTP GET upgrade request for client handshake. */
std::string build_client_request(
    const std::string& host,
    const std::string& port,
    const std::string& path,
    const std::string& key,
    const std::string& extensions = std::string());

/** @brief Build HTTP 101 Switching Protocols response. */
std::string build_server_response(
    const std::string& accept_key,
    const std::string& extensions = std::string());

/** @brief Parse HTTP headers from raw request bytes. */
bool parse_http_headers(
    const std::string& raw,
    std::string& request_line,
    std::map<std::string, std::string>& headers);

/** @brief Validate required WebSocket upgrade headers. */
bool validate_client_request(const std::map<std::string, std::string>& headers);

} // namespace handshake
} // namespace wscpp

#endif // WSCPP_HANDSHAKE_HPP
