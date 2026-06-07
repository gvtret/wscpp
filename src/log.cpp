#include <wscpp/log.hpp>

#if WSCPP_ENABLE_LOGGING
#define SPDLOG_HEADER_ONLY
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include <wscpp/detail/log.hpp>
#include <memory>
#include <mutex>

namespace wscpp {
namespace detail {

#if WSCPP_ENABLE_LOGGING

namespace {

std::once_flag log_init_flag;

} // namespace

spdlog::level::level_enum to_spdlog_level(log_level level) {
    switch (level) {
    case log_level::trace:
        return spdlog::level::trace;
    case log_level::debug:
        return spdlog::level::debug;
    case log_level::info:
        return spdlog::level::info;
    case log_level::warn:
        return spdlog::level::warn;
    case log_level::err:
        return spdlog::level::err;
    case log_level::critical:
        return spdlog::level::critical;
    case log_level::off:
        return spdlog::level::off;
    }
    return spdlog::level::err;
}

void ensure_logger() {
    std::call_once(log_init_flag, []() {
        const std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_mt("wscpp");
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::err);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
    });
}

void log_error(const char *component, const std::string &message) {
    ensure_logger();
    spdlog::error("[{}] {}", component, message);
}

void log_error_ec(const char *component, const char *action, const std::error_code &ec) {
    if (!ec) {
        return;
    }
    ensure_logger();
    spdlog::error("[{}] {}: {} ({})", component, action, ec.message(), ec.value());
}

#endif // WSCPP_ENABLE_LOGGING

} // namespace detail

void set_log_level(log_level level) {
#if WSCPP_ENABLE_LOGGING
    detail::ensure_logger();
    spdlog::set_level(detail::to_spdlog_level(level));
#else
    (void)level;
#endif
}

} // namespace wscpp
