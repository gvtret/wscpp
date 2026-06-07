#ifndef WSCPP_NET_STREAM_BUFFER_HPP
#define WSCPP_NET_STREAM_BUFFER_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace wscpp {
namespace net {

/** @brief Growable byte buffer for HTTP handshake reads (non-ASIO transport). */
class stream_buffer {
  public:
    void append(const char *data, std::size_t size) {
        data_.insert(data_.end(), data, data + size);
    }

    void append(char byte) {
        data_.push_back(byte);
    }

    std::string to_string() const {
        return std::string(data_.begin(), data_.end());
    }

    void clear() {
        data_.clear();
    }

    std::size_t size() const {
        return data_.size();
    }

  private:
    std::vector<char> data_;
};

} // namespace net
} // namespace wscpp

#endif
