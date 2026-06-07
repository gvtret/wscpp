#ifndef WSCPP_SERVER_HPP
#define WSCPP_SERVER_HPP

/**
 * @file server.hpp
 * @brief WebSocket server API with optional TLS.
 */

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include "connection.hpp"
#include "frame/parser.hpp"
#include "frame/builder.hpp"

namespace wscpp {

/**
 * @brief WebSocket server accepting TCP (and optional TLS) connections.
 */
class server {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;
    using connection_type = connection;

    using connection_callback = std::function<void(std::shared_ptr<connection_type>)>;
    using message_callback = std::function<void(std::shared_ptr<connection_type>, const std::vector<uint8_t>&, frame::opcode)>;
    using close_callback = std::function<void(std::shared_ptr<connection_type>, uint16_t, const std::string&)>;
    using error_callback = std::function<void(std::shared_ptr<connection_type>, const std::string&)>;

    /** @brief Construct server with internal io_context. */
    server();
    /** @brief Destructor; stops acceptor and closes connections. */
    ~server();

    server(const server&) = delete;
    server& operator=(const server&) = delete;

    /** @brief Listen on all interfaces at @p port. */
    void listen(uint16_t port);

    /** @brief Listen on @p host at @p port. */
    void listen(const std::string& host, uint16_t port);

    /**
     * @brief Enable WSS by providing TLS context with certificate/key loaded.
     * @param ctx Shared ASIO SSL context (server mode).
     */
    void set_ssl_context(std::shared_ptr<ssl_context> ctx);

    /**
     * @brief Called for each new WebSocket connection after HTTP upgrade.
     * Set per-connection handlers here; reader starts after callback returns.
     */
    void set_on_connection(connection_callback cb);

    /** @brief Global message callback (optional; prefer per-connection handlers). */
    void set_on_message(message_callback cb);

    /** @brief Global close callback. */
    void set_on_close(close_callback cb);

    /** @brief Global error callback. */
    void set_on_error(error_callback cb);

    /** @brief Start accept loop on background thread. */
    void start();

    /** @brief Stop server and close all connections. */
    void stop();

    /** @brief Join server background thread. */
    void join();

    /** @brief True after listen() succeeded. */
    bool is_running() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace wscpp

#endif // WSCPP_SERVER_HPP
