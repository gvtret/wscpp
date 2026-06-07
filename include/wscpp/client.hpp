#ifndef WSCPP_CLIENT_HPP
#define WSCPP_CLIENT_HPP

#ifndef WSCPP_ENABLE_DEFLATE
#define WSCPP_ENABLE_DEFLATE 0
#endif

/**
 * @file client.hpp
 * @brief WebSocket client API (ws:// and wss://).
 */

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include "connection.hpp"
#include "frame/parser.hpp"
#include "frame/builder.hpp"

namespace wscpp {

/**
 * @brief High-level WebSocket client with URL-based connect.
 */
class client {
public:
    using tcp = asio::ip::tcp;
    using ssl_context = asio::ssl::context;
    using connection_type = connection;

    using message_callback = std::function<void(const std::vector<uint8_t>&, frame::opcode)>;
    using close_callback = std::function<void(uint16_t, const std::string&)>;
    using error_callback = std::function<void(const std::string&)>;
    using open_callback = std::function<void()>;

    /** @brief Construct client with internal io_context. */
    client();
    /** @brief Destructor; closes connection if open. */
    ~client();

    client(const client&) = delete;
    client& operator=(const client&) = delete;

    /**
     * @brief Connect to WebSocket URL (ws:// or wss://).
     * @param url Full WebSocket URL including path.
     */
    void connect(const std::string& url);

    /**
     * @brief Send close frame and shut down connection.
     * @param status_code RFC 6455 close status (default 1000).
     * @param reason Optional UTF-8 reason phrase.
     */
    void close(uint16_t status_code = 1000, const std::string& reason = "");

    /** @brief Send a text WebSocket frame. */
    void send_text(const std::string& text, bool fin = true);

    /** @brief Send a binary WebSocket frame. */
    void send_binary(const uint8_t* data, size_t size, bool fin = true);

    /** @brief Send continuation fragment (multi-frame message). */
    void send_continuation(const std::string& data, bool fin = true);

    /** @brief Register callback invoked after WebSocket handshake completes. */
    void set_on_open(open_callback cb);

    /** @brief Register callback for incoming messages. */
    void set_on_message(message_callback cb);

    /** @brief Register callback when connection closes. */
    void set_on_close(close_callback cb);

    /** @brief Register callback for protocol or I/O errors. */
    void set_on_error(error_callback cb);

    /** @brief True if WebSocket session is open. */
    bool is_open() const;

    /** @brief True if connection uses TLS (wss://). */
    bool is_ssl() const;

    /** @brief Run io_context until stopped (blocking). */
    void run();

    /** @brief Stop io_context event loop. */
    void stop();

#if WSCPP_ENABLE_DEFLATE
    /** @brief Request RFC 7692 permessage-deflate on connect (default off). */
    void enable_permessage_deflate(bool enable = true);
#endif

private:
    struct url_info {
        std::string scheme;
        std::string host;
        std::string port;
        std::string path;
        bool secure;
    };

    url_info parse_url(const std::string& url);

    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace wscpp

#endif // WSCPP_CLIENT_HPP
