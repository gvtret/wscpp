#ifndef WSCPP_CONNECTION_HPP
#define WSCPP_CONNECTION_HPP

#ifndef WSCPP_ENABLE_DEFLATE
#define WSCPP_ENABLE_DEFLATE 0
#endif

/**
 * @file connection.hpp
 * @brief WebSocket connection: framing, reader thread, callbacks.
 */

#include <wscpp/net/transport.hpp>
#include <memory>
#include <functional>
#include <string>
#include <system_error>
#include <vector>
#include "frame/parser.hpp"
#include "frame/builder.hpp"

namespace wscpp {

/** @brief Endpoint role for RFC 6455 masking rules. */
enum class connection_role {
    client, ///< Masks outbound frames; expects unmasked inbound
    server  ///< Unmasked outbound; expects masked inbound from peer
};

/**
 * @brief A single WebSocket connection over TCP or TLS.
 */
class connection {
public:
    using tcp_endpoint = net::tcp_endpoint;
    using ssl_context = net::ssl_context;
    using socket_type = net::stream_socket;

    using message_callback = std::function<void(const std::vector<uint8_t>&, frame::opcode)>;
    using close_callback = std::function<void(uint16_t, const std::string&)>;
    using error_callback = std::function<void(const std::string&)>;
    using open_callback = std::function<void()>;

    /**
     * @brief Create connection bound to @p io_context.
     * @param io_context Transport event loop (ASIO or stub).
     */
    explicit connection(net::io_context& io_context);
    ~connection();

    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;

    /** @brief Connect outbound TCP socket to host:port. */
    std::error_code connect(const std::string& host, const std::string& port);

    /** @brief Connect to TCP endpoint. */
    std::error_code connect(const tcp_endpoint& endpoint);

#if WSCPP_USE_ASIO
    /** @brief Take ownership of accepted TCP socket (server path). */
    std::error_code adopt(net::tcp::socket socket);
#else
    /** @brief Take ownership of accepted TCP socket (server path). */
    std::error_code adopt(net::tcp_socket socket);
#endif

    /** @brief Start background frame reader; invokes on_open first. */
    void activate();

    /** @brief Start reading frames (alias for activate). */
    void start_reading();

    /** @brief Send close frame and stop reader. */
    void close(uint16_t status_code = 1000, const std::string& reason = "");

    /** @brief Send text frame. */
    std::error_code send_text(const std::string& text, bool fin = true);

    /** @brief Send binary frame. */
    std::error_code send_binary(const uint8_t* data, size_t size, bool fin = true);

    /** @brief Send continuation fragment (after TEXT/BINARY with fin=false). */
    std::error_code send_continuation(const uint8_t* data, size_t size, bool fin = true);

    /** @brief Send ping control frame (RFC 6455 §5.5). */
    std::error_code send_ping(const uint8_t* data = nullptr, size_t size = 0);

    /** @brief Set client/server role for RFC 6455 masking (default: server). */
    void set_role(connection_role role);

#if WSCPP_ENABLE_DEFLATE
    /** @brief Enable RFC 7692 permessage-deflate after handshake negotiation. */
    void set_permessage_deflate(bool enabled);

    /** @brief True if permessage-deflate is active on this connection. */
    bool permessage_deflate() const;
#endif

    void set_on_open(open_callback cb);
    void set_on_message(message_callback cb);
    void set_on_close(close_callback cb);
    void set_on_error(error_callback cb);

    bool is_open() const;
    bool is_ssl() const;

    socket_type& socket();
    const socket_type& socket() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace wscpp

#endif // WSCPP_CONNECTION_HPP
