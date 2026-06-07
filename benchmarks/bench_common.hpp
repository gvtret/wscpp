#ifndef WSCPP_BENCH_COMMON_HPP
#define WSCPP_BENCH_COMMON_HPP

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace wscpp {
namespace bench {

using clock = std::chrono::steady_clock;

inline double elapsed_sec(const clock::time_point &start, const clock::time_point &end) {
    return std::chrono::duration<double>(end - start).count();
}

inline double percentile(std::vector<double> values, double p) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t idx = static_cast<std::size_t>(p * static_cast<double>(values.size() - 1));
    return values[idx];
}

inline void print_throughput(const char *name, double bytes, double seconds, int iterations) {
    const double mb = bytes / (1024.0 * 1024.0);
    const double mbps = seconds > 0.0 ? mb / seconds : 0.0;
    std::printf("%-24s %8.2f MB/s  (%d iter, %.3f s)\n", name, mbps, iterations, seconds);
}

inline void print_latency(const char *name, const std::vector<double> &ms_samples) {
    if (ms_samples.empty()) {
        std::printf("%-24s (no samples)\n", name);
        return;
    }
    std::vector<double> sorted = ms_samples;
    const double p50 = percentile(sorted, 0.50);
    const double p99 = percentile(sorted, 0.99);
    std::printf("%-24s p50=%.3f ms  p99=%.3f ms  (n=%zu)\n", name, p50, p99, ms_samples.size());
}

} // namespace bench
} // namespace wscpp

#endif
