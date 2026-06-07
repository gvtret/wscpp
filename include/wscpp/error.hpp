#ifndef WSCPP_ERROR_HPP
#define WSCPP_ERROR_HPP

/**
 * @file error.hpp
 * @brief wscpp error codes (no exceptions in the public API).
 */

#include <system_error>

namespace wscpp {

enum class errc {
    success = 0,
    already_open,
    not_open,
    not_connected,
    handshake_failed,
    invalid_address,
    ssl_not_enabled,
    ssl_init_failed,
    ssl_io_error,
    ssl_would_block,
    compression_failed,
    server_not_listening,
    host_not_found,
};

const std::error_category &error_category();

inline std::error_code make_error_code(errc e) {
    return std::error_code(static_cast<int>(e), error_category());
}

} // namespace wscpp

namespace std {

template <> struct is_error_code_enum<wscpp::errc> : true_type {};

} // namespace std

#endif // WSCPP_ERROR_HPP
