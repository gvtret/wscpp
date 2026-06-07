#pragma once

#include <string>
#include <system_error>

namespace wscpp {
namespace detail {

#if WSCPP_ENABLE_LOGGING
void log_error(const char *component, const std::string &message);
void log_error_ec(const char *component, const char *action, const std::error_code &ec);
#else
inline void log_error(const char *, const std::string &) {}
inline void log_error_ec(const char *, const char *, const std::error_code &ec) {
    (void)ec;
}
#endif

template <typename Callback>
inline void emit_error(Callback &cb, const char *component, const std::string &message) {
    log_error(component, message);
    if (cb) {
        cb(message);
    }
}

template <typename Callback>
inline void emit_error_ec(Callback &cb, const char *component, const char *action,
                          const std::error_code &ec) {
    if (!ec) {
        return;
    }
    log_error_ec(component, action, ec);
    if (cb) {
        cb(ec.message());
    }
}

} // namespace detail
} // namespace wscpp
