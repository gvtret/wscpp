#ifndef WSCPP_DETAIL_MAKE_UNIQUE_HPP
#define WSCPP_DETAIL_MAKE_UNIQUE_HPP

#include <memory>

namespace wscpp {
namespace detail {

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace detail
} // namespace wscpp

#endif // WSCPP_DETAIL_MAKE_UNIQUE_HPP
