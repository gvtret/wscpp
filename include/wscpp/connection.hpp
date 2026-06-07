#ifndef WSCPP_CONNECTION_HPP
#define WSCPP_CONNECTION_HPP

/**
 * @file connection.hpp
 * @brief WebSocket connection: framing, reader thread, callbacks.
 */

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include "net/asio_socket.hpp"
#include "frame/parser.hpp"
#include "frame/builder.hpp"

namespace wscpp {

/**
 * @brief A single WebSocket connection over TCP or TLS.
 */
class connection {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;
    using socket_type = net::asio_socket;

    using message_callback = std::function<void(const std::vector<uint8_t>&, frame::opcode)>;
    using close_callback = std::function<void(uint16_t, const std::string&)>;
    using error_callback = std::function<void(const std::string&)>;
    using open_callback = std::function<void()>;

    /**
     * @brief Create connection bound to @p io_context.
     * @param io_context ASIO io_context for socket operations.
     */
    explicit connection(asio::io_context& io_context);
    ~connection();

    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;

    /** @brief Connect outbound TCP socket to host:port. */
    void connect(const std::string& host, const std::string& port);

    /** @brief Connect to TCP endpoint. */
    void connect(const tcp::endpoint& endpoint);

    /** @brief Take ownership of accepted TCP socket (server path). */
    void adopt(tcp::socket socket);

    /** @brief Start background frame reader; invokes on_open first. */
    void activate();

    /** @brief Start reading frames (alias for activate). */
    void start_reading();

    /** @brief Send close frame and stop reader. */
    void close(uint16_t status_code = 1000, const std::string& reason = "");

    /** @brief Send text frame. */
    void send_text(const std::string& text, bool fin = true);

    /** @brief Send binary frame. */
    void send_binary(const uint8_t* data, size_t size, bool fin = true);

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
