#pragma once

namespace wscpp {

/// Minimum log level for the internal spdlog backend (when WSCPP_ENABLE_LOGGING is on).
enum class log_level {
    trace,
    debug,
    info,
    warn,
    err,
    critical,
    off
};

/// Set minimum log level for library diagnostics (default: err).
void set_log_level(log_level level);

} // namespace wscpp
