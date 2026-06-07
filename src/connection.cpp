#include <utility>
#include <wscpp/connection.hpp>
#include <wscpp/detail/make_unique.hpp>
#include <wscpp/detail/utf8.hpp>
#include <wscpp/error.hpp>
#include <wscpp/frame/builder.hpp>
#include <wscpp/frame/parser.hpp>
#if WSCPP_ENABLE_DEFLATE
#include <wscpp/extensions/permessage_deflate.hpp>
#endif
#include <array>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace wscpp {

class connection::impl {
  public:
    explicit impl(net::io_context &io_context)
        : socket_(io_context), role_(connection_role::server), fragmented_(false),
          message_opcode_(frame::opcode::TEXT), message_compressed_(false)
#if WSCPP_ENABLE_DEFLATE
          ,
          permessage_deflate_(false)
#endif
          ,
          is_open_(false), is_closing_(false), expecting_close_(false), reader_running_(false) {
    }

    ~impl() {
        is_open_ = false;
        socket_.close();
        join_reader_if_needed();
    }

    std::error_code connect(const std::string &host, const std::string &port) {
        if (is_open_) {
            return make_error_code(errc::already_open);
        }
        const std::error_code ec = socket_.connect(host, port);
        if (ec) {
            return ec;
        }
        open_and_start_reader();
        return std::error_code();
    }

    std::error_code connect(const tcp_endpoint &endpoint) {
        if (is_open_) {
            return make_error_code(errc::already_open);
        }
        const std::error_code ec = socket_.connect(endpoint);
        if (ec) {
            return ec;
        }
        open_and_start_reader();
        return std::error_code();
    }

#if WSCPP_USE_ASIO
    std::error_code adopt(net::tcp::socket sock) {
        if (is_open_) {
            return make_error_code(errc::already_open);
        }
        return socket_.adopt(std::move(sock));
    }
#else
    std::error_code adopt(net::tcp_socket sock) {
        if (is_open_) {
            return make_error_code(errc::already_open);
        }
        return socket_.adopt(sock.release());
    }
#endif

    void activate() {
        open_and_start_reader();
    }

    void start_reading() {
        start_reader_thread();
    }

    void close(uint16_t status_code, const std::string &reason) {
        if (!is_open_ || is_closing_) {
            return;
        }
        if (!is_valid_close_code(status_code)) {
            status_code = 1000;
        }
        is_closing_ = true;
        frame::builder b;
        const std::vector<uint8_t> close_frame =
            b.build_close(status_code, reason, outbound_mask());
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            std::error_code ec;
            socket_.write(close_frame.data(), close_frame.size(), ec);
        }
        is_open_ = false;
        socket_.close();
        join_reader_if_needed();
        if (on_close_) {
            on_close_(status_code, reason);
        }
    }

    std::error_code send_text(const std::string &text, bool fin) {
        return send_data_frame(frame::opcode::TEXT, reinterpret_cast<const uint8_t *>(text.data()),
                               text.size(), fin);
    }

    std::error_code send_binary(const uint8_t *data, size_t size, bool fin) {
        return send_data_frame(frame::opcode::BINARY, data, size, fin);
    }

    std::error_code send_continuation(const uint8_t *data, size_t size, bool fin) {
        return send_data_frame(frame::opcode::CONTINUATION, data, size, fin);
    }

    std::error_code send_ping(const uint8_t *data, size_t size) {
        if (!is_open_) {
            return make_error_code(errc::not_open);
        }
        frame::builder b;
        const std::vector<uint8_t> frame = b.build_ping(data, size, outbound_mask());
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (!is_open_ || is_closing_) {
            return make_error_code(errc::not_open);
        }
        std::error_code ec;
        socket_.write(frame.data(), frame.size(), ec);
        return ec;
    }

    void set_role(connection_role role) {
        role_ = role;
    }

#if WSCPP_ENABLE_DEFLATE
    void set_permessage_deflate(bool enabled) {
        permessage_deflate_ = enabled;
    }

    bool permessage_deflate() const {
        return permessage_deflate_;
    }
#endif

    void set_on_open(open_callback cb) {
        on_open_ = std::move(cb);
    }
    void set_on_message(message_callback cb) {
        on_message_ = std::move(cb);
    }
    void set_on_close(close_callback cb) {
        on_close_ = std::move(cb);
    }
    void set_on_error(error_callback cb) {
        on_error_ = std::move(cb);
    }

    bool is_open() const {
        return is_open_;
    }
    bool is_ssl() const {
        return socket_.is_ssl();
    }
    socket_type &socket() {
        return socket_;
    }
    const socket_type &socket() const {
        return socket_;
    }

  private:
    bool outbound_mask() const {
        return role_ == connection_role::client;
    }

    std::error_code send_data_frame(frame::opcode op, const uint8_t *data, size_t size, bool fin) {
        if (!is_open_) {
            return make_error_code(errc::not_open);
        }
        const uint8_t *send_data = data;
        std::size_t send_size = size;
        std::vector<uint8_t> compressed;
        bool rsv1 = false;

#if WSCPP_ENABLE_DEFLATE
        if (permessage_deflate_ && fin &&
            (op == frame::opcode::TEXT || op == frame::opcode::BINARY)) {
            if (!extensions::compress_message(data, size, compressed)) {
                return make_error_code(errc::compression_failed);
            }
            send_data = compressed.data();
            send_size = compressed.size();
            rsv1 = true;
        }
#endif

        frame::builder b;
        std::array<uint8_t, 4> mask_key = {{0, 0, 0, 0}};
        if (outbound_mask()) {
            mask_key = b.generate_masking_key();
        }
        const std::vector<uint8_t> frame =
            b.build(op, send_data, send_size, fin, outbound_mask(), mask_key, rsv1);
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (!is_open_ || is_closing_) {
            return make_error_code(errc::not_open);
        }
        std::error_code ec;
        socket_.write(frame.data(), frame.size(), ec);
        return ec;
    }

    static bool is_valid_close_code(uint16_t code) {
        if (code < 1000) {
            return false;
        }
        if (code >= 1000 && code <= 1003) {
            return true;
        }
        if (code >= 1004 && code <= 1006) {
            return false;
        }
        if (code >= 1007 && code <= 1011) {
            return true;
        }
        if (code >= 1012 && code <= 2999) {
            return false;
        }
        return code <= 4999;
    }

    void fail_with_close(uint16_t code, const std::string &reason) {
        if (on_error_) {
            on_error_(reason);
        }
        if (is_open_ && !is_closing_) {
            is_closing_ = true;
            frame::builder b;
            const std::vector<uint8_t> close_frame = b.build_close(code, reason, outbound_mask());
            std::lock_guard<std::mutex> lock(write_mutex_);
            std::error_code ec;
            socket_.write(close_frame.data(), close_frame.size(), ec);
        }
        is_open_ = false;
        socket_.close();
        if (on_close_) {
            on_close_(code, reason);
        }
    }

    void fail_protocol(const std::string &reason) {
        fail_with_close(1002, reason);
    }

    bool validate_frame_header() {
        const bool data_opcode = current_header_.op == frame::opcode::TEXT ||
                                 current_header_.op == frame::opcode::BINARY ||
                                 current_header_.op == frame::opcode::CONTINUATION;

#if WSCPP_ENABLE_DEFLATE
        if (current_header_.rsv1) {
            if (!permessage_deflate_ || !data_opcode) {
                fail_protocol("RSV1 set without permessage-deflate");
                return false;
            }
            if (current_header_.op == frame::opcode::CONTINUATION) {
                fail_protocol("RSV1 on continuation frame");
                return false;
            }
        }
        if ((current_header_.rsv2 || current_header_.rsv3) ||
            (!permessage_deflate_ && current_header_.rsv1)) {
#else
        if (current_header_.rsv1 || current_header_.rsv2 || current_header_.rsv3) {
#endif
            fail_protocol("RSV bits set without extension negotiation");
            return false;
        }
        const uint8_t op_val = static_cast<uint8_t>(current_header_.op);
        if (!((op_val <= 0x02) || (op_val >= 0x08 && op_val <= 0x0A))) {
            fail_protocol("Invalid frame opcode");
            return false;
        }
        if (role_ == connection_role::server && !current_header_.mask) {
            fail_protocol("Client frame must be masked");
            return false;
        }
        if (role_ == connection_role::client && current_header_.mask) {
            fail_protocol("Server frame must not be masked");
            return false;
        }
        return true;
    }

    void deliver_message(std::vector<uint8_t> &payload, frame::opcode op, bool compressed) {
#if WSCPP_ENABLE_DEFLATE
        if (compressed) {
            if (!permessage_deflate_) {
                fail_protocol("Compressed frame without negotiated extension");
                return;
            }
            std::vector<uint8_t> plain;
            if (!extensions::decompress_message(payload.data(), payload.size(), plain)) {
                fail_protocol("permessage-deflate decompression failed");
                return;
            }
            payload.swap(plain);
        }
#endif
        if (op == frame::opcode::TEXT && !detail::is_valid_utf8(payload)) {
            fail_with_close(1007, "Invalid UTF-8 in text frame");
            return;
        }
        if (on_message_) {
            on_message_(payload, op);
        }
    }

    void deliver_message(const std::vector<uint8_t> &payload, frame::opcode op) {
        std::vector<uint8_t> copy = payload;
        deliver_message(copy, op, false);
    }

    void reset_fragment_state() {
        fragmented_ = false;
        message_buffer_.clear();
        message_opcode_ = frame::opcode::TEXT;
        message_compressed_ = false;
    }

    void open_and_start_reader() {
        is_open_ = true;
        if (on_open_) {
            on_open_();
        }
        start_reader_thread();
    }

    void start_reader_thread() {
        if (!is_open_ || reader_running_) {
            return;
        }
        reader_running_ = true;
        reader_thread_ = std::thread([this]() { read_loop(); });
    }

    void shutdown_reader() {
        is_open_ = false;
        socket_.close();
        join_reader_if_needed();
    }

    void join_reader_if_needed() {
        if (!reader_thread_.joinable()) {
            return;
        }
        if (reader_thread_.get_id() == std::this_thread::get_id()) {
            return;
        }
        reader_thread_.join();
        reader_running_ = false;
    }

    bool read_exact(void *data, std::size_t size, std::error_code &ec) {
        ec.clear();
        const std::size_t n = socket_.read(data, size, ec);
        return !ec && n == size;
    }

    void read_loop() {
        while (is_open_ && !is_closing_) {
            if (!read_one_frame()) {
                break;
            }
        }
    }

    bool read_one_frame() {
        if (!is_open_) {
            return false;
        }
        std::error_code ec;
        if (!read_exact(header_buffer_, 2, ec)) {
            if (ec && on_error_) {
                on_error_(ec.message());
            }
            is_open_ = false;
            return false;
        }
        parse_header(ec);
        if (ec) {
            if (on_error_) {
                on_error_(ec.message());
            }
            is_open_ = false;
            return false;
        }
        return is_open_;
    }

    void parse_header(std::error_code &ec) {
        ec.clear();
        const uint8_t first_byte = header_buffer_[0];
        const uint8_t second_byte = header_buffer_[1];

        current_header_.fin = (first_byte & 0x80) != 0;
        current_header_.rsv1 = (first_byte & 0x40) != 0;
        current_header_.rsv2 = (first_byte & 0x20) != 0;
        current_header_.rsv3 = (first_byte & 0x10) != 0;
        current_header_.op = static_cast<frame::opcode>(first_byte & 0x0F);
        current_header_.mask = (second_byte & 0x80) != 0;
        current_header_.payload_len = second_byte & 0x7F;

        if (current_header_.payload_len == 126) {
            if (!read_exact(ext_payload_buffer_, 2, ec)) {
                return;
            }
            current_header_.payload_len =
                (static_cast<uint64_t>(ext_payload_buffer_[0]) << 8) | ext_payload_buffer_[1];
        } else if (current_header_.payload_len == 127) {
            if (!read_exact(ext_payload_buffer_, 8, ec)) {
                return;
            }
            current_header_.payload_len = 0;
            for (int i = 0; i < 8; ++i) {
                current_header_.payload_len =
                    (current_header_.payload_len << 8) | ext_payload_buffer_[i];
            }
        }

        if (current_header_.mask) {
            if (!read_exact(masking_key_buffer_, 4, ec)) {
                return;
            }
            current_header_.masking_key = {{masking_key_buffer_[0], masking_key_buffer_[1],
                                            masking_key_buffer_[2], masking_key_buffer_[3]}};
        }

        if (!validate_frame_header()) {
            return;
        }

        read_payload(ec);
    }

    void read_payload(std::error_code &ec) {
        ec.clear();
        if (current_header_.payload_len == 0) {
            handle_frame();
            return;
        }

        payload_buffer_.resize(static_cast<std::size_t>(current_header_.payload_len));
        if (!read_exact(payload_buffer_.data(), payload_buffer_.size(), ec)) {
            return;
        }

        if (current_header_.mask) {
            for (std::size_t i = 0; i < payload_buffer_.size(); ++i) {
                payload_buffer_[i] ^= current_header_.masking_key[i % 4];
            }
        }
        handle_frame();
    }

    void handle_frame() {
        const frame::opcode op = current_header_.op;

        if (op == frame::opcode::CLOSE) {
            handle_close_frame();
            return;
        }
        if (op == frame::opcode::PING) {
            handle_ping_frame();
            return;
        }
        if (op == frame::opcode::PONG) {
            return;
        }

        if (op == frame::opcode::CONTINUATION) {
            if (!fragmented_) {
                fail_protocol("Unexpected continuation frame");
                return;
            }
            message_buffer_.insert(message_buffer_.end(), payload_buffer_.begin(),
                                   payload_buffer_.end());
            if (current_header_.fin) {
                deliver_message(message_buffer_, message_opcode_, message_compressed_);
                reset_fragment_state();
            }
            return;
        }

        if (op == frame::opcode::TEXT || op == frame::opcode::BINARY) {
            if (fragmented_) {
                fail_protocol("New data frame before fragmented message completed");
                return;
            }
            if (!current_header_.fin) {
                fragmented_ = true;
                message_opcode_ = op;
                message_buffer_ = payload_buffer_;
                message_compressed_ = current_header_.rsv1;
                return;
            }
            deliver_message(payload_buffer_, op, current_header_.rsv1);
            return;
        }

        fail_protocol("Unsupported data frame opcode");
    }

    void handle_close_frame() {
        expecting_close_ = true;
        uint16_t status_code = 1000;
        std::string reason;
        if (payload_buffer_.size() >= 2) {
            status_code = static_cast<uint16_t>((static_cast<uint16_t>(payload_buffer_[0]) << 8) |
                                                payload_buffer_[1]);
            if (!is_valid_close_code(status_code)) {
                status_code = 1002;
            }
            reason = std::string(payload_buffer_.begin() + 2, payload_buffer_.end());
        }
        frame::builder b;
        const std::vector<uint8_t> close_frame = b.build_close(status_code, "", outbound_mask());
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            std::error_code ec;
            socket_.write(close_frame.data(), close_frame.size(), ec);
        }
        is_open_ = false;
        if (on_close_) {
            on_close_(status_code, reason);
        }
    }

    void handle_ping_frame() {
        if (is_closing_ || !is_open_) {
            return;
        }
        frame::builder b;
        const std::vector<uint8_t> pong_frame =
            b.build_pong(payload_buffer_.data(), payload_buffer_.size(), outbound_mask());
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (is_closing_ || !is_open_) {
            return;
        }
        std::error_code ec;
        socket_.write(pong_frame.data(), pong_frame.size(), ec);
        if (ec && on_error_) {
            on_error_(ec.message());
        }
    }

    socket_type socket_;
    connection_role role_;

    uint8_t header_buffer_[2];
    uint8_t ext_payload_buffer_[8];
    uint8_t masking_key_buffer_[4];

    frame::frame_header current_header_;
    std::vector<uint8_t> payload_buffer_;
    std::vector<uint8_t> message_buffer_;
    bool fragmented_;
    frame::opcode message_opcode_;
    bool message_compressed_;

#if WSCPP_ENABLE_DEFLATE
    bool permessage_deflate_;
#endif

    bool is_open_;
    bool is_closing_;
    bool expecting_close_;
    bool reader_running_;
    std::thread reader_thread_;
    std::mutex write_mutex_;

    open_callback on_open_;
    message_callback on_message_;
    close_callback on_close_;
    error_callback on_error_;
};

connection::connection(net::io_context &io_context)
    : pimpl_(detail::make_unique<impl>(io_context)) {}

connection::~connection() = default;

std::error_code connection::connect(const std::string &host, const std::string &port) {
    return pimpl_->connect(host, port);
}

std::error_code connection::connect(const tcp_endpoint &endpoint) {
    return pimpl_->connect(endpoint);
}

#if WSCPP_USE_ASIO
std::error_code connection::adopt(net::tcp::socket socket) {
    return pimpl_->adopt(std::move(socket));
}
#else
std::error_code connection::adopt(net::tcp_socket socket) {
    return pimpl_->adopt(std::move(socket));
}
#endif

void connection::activate() {
    pimpl_->activate();
}

void connection::start_reading() {
    pimpl_->start_reading();
}

void connection::close(uint16_t status_code, const std::string &reason) {
    pimpl_->close(status_code, reason);
}

std::error_code connection::send_text(const std::string &text, bool fin) {
    return pimpl_->send_text(text, fin);
}

std::error_code connection::send_binary(const uint8_t *data, size_t size, bool fin) {
    return pimpl_->send_binary(data, size, fin);
}

std::error_code connection::send_continuation(const uint8_t *data, size_t size, bool fin) {
    return pimpl_->send_continuation(data, size, fin);
}

std::error_code connection::send_ping(const uint8_t *data, size_t size) {
    return pimpl_->send_ping(data, size);
}

void connection::set_role(connection_role role) {
    pimpl_->set_role(role);
}

#if WSCPP_ENABLE_DEFLATE
void connection::set_permessage_deflate(bool enabled) {
    pimpl_->set_permessage_deflate(enabled);
}

bool connection::permessage_deflate() const {
    return pimpl_->permessage_deflate();
}
#endif

void connection::set_on_open(open_callback cb) {
    pimpl_->set_on_open(std::move(cb));
}

void connection::set_on_message(message_callback cb) {
    pimpl_->set_on_message(std::move(cb));
}

void connection::set_on_close(close_callback cb) {
    pimpl_->set_on_close(std::move(cb));
}

void connection::set_on_error(error_callback cb) {
    pimpl_->set_on_error(std::move(cb));
}

bool connection::is_open() const {
    return pimpl_->is_open();
}

bool connection::is_ssl() const {
    return pimpl_->is_ssl();
}

connection::socket_type &connection::socket() {
    return pimpl_->socket();
}

const connection::socket_type &connection::socket() const {
    return pimpl_->socket();
}

} // namespace wscpp
