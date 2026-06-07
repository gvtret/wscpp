#ifndef WSCPP_HANDSHAKE_HPP
#define WSCPP_HANDSHAKE_HPP

#include <map>
#include <string>

namespace wscpp {
namespace handshake {

std::string generate_key();
std::string compute_accept(const std::string& key);

std::string build_client_request(
    const std::string& host,
    const std::string& port,
    const std::string& path,
    const std::string& key);

bool parse_http_headers(
    const std::string& raw,
    std::string& request_line,
    std::map<std::string, std::string>& headers);

std::string build_server_response(const std::string& accept_key);

bool validate_client_request(const std::map<std::string, std::string>& headers);

} // namespace handshake
} // namespace wscpp

#endif // WSCPP_HANDSHAKE_HPP
