#ifndef WSCPP_NET_TRANSPORT_HPP
#define WSCPP_NET_TRANSPORT_HPP

#ifndef WSCPP_USE_ASIO
#define WSCPP_USE_ASIO 1
#endif

#if WSCPP_USE_ASIO

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <wscpp/net/asio_socket.hpp>

namespace wscpp {
namespace net {

using io_context = asio::io_context;
using stream_buffer = asio::streambuf;
using tcp = asio::ip::tcp;
using tcp_endpoint = asio::ip::tcp::endpoint;
using tcp_socket = asio::ip::tcp::socket;
using ssl_context = asio::ssl::context;
using stream_socket = asio_socket;

inline std::string stream_buffer_to_string(const stream_buffer &buf) {
    const stream_buffer::const_buffers_type bufs = buf.data();
    return std::string(asio::buffers_begin(bufs), asio::buffers_end(bufs));
}

} // namespace net
} // namespace wscpp

#else

#include <string>
#include <wscpp/net/io_context.hpp>
#include <wscpp/net/linux_socket.hpp>
#include <wscpp/net/openssl_context.hpp>
#include <wscpp/net/stream_buffer.hpp>
#include <wscpp/net/tcp_endpoint.hpp>
#include <wscpp/net/tcp_socket.hpp>

namespace wscpp {
namespace net {

using stream_socket = linux_socket;

inline std::string stream_buffer_to_string(const stream_buffer &buf) {
    return buf.to_string();
}

} // namespace net
} // namespace wscpp

#endif

#endif
