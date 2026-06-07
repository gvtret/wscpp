#include <wscpp/connection.hpp>
#include <wscpp/detail/make_unique.hpp>
#include <wscpp/detail/utf8.hpp>
#include <wscpp/frame/parser.hpp>
#include <wscpp/frame/builder.hpp>
#include <stdexcept>
#include <thread>
#include <vector>
#include <string>

namespace wscpp {

class connection::impl {
public:
    explicit impl(asio::io_context& io_context)
        : socket_(io_context),
          io_context_(io_context),
          role_(connection_role::server),
          is_open_(false),
          is_closing_(false),
          expecting_close_(false),
          reader_running_(false),
          fragmented_(false),
          message_opcode_(frame::opcode::TEXT) {}

    ~impl() {
        is_open_ = false;
        try {
            socket_.close();
        } catch (...) {
        }
        join_reader_if_needed();
    }

    void connect(const std::string& host, const std::string& port) {
        if (is_open_) {
            throw std::runtime_error("Connection already open");
        }
        socket_.connect(host, port);
        open_and_start_reader();
    }

    void connect(const tcp::endpoint& endpoint) {
        if (is_open_) {
            throw std::runtime_error("Connection already open");
        }
        socket_.connect(endpoint);
        open_and_start_reader();
    }

    void adopt(tcp::socket sock) {
        if (is_open_) {
            throw std::runtime_error("Connection already open");
        }
        socket_.adopt(std::move(sock));
    }

    void activate() {
        open_and_start_reader();
    }

    void start_reading() {
        start_reader_thread();
    }

    void close(uint16_t status_code, const std::string& reason) {
        if (!is_open_ || is_closing_) {
            return;
        }
        if (!is_valid_close_code(status_code)) {
            status_code = 1000;
        }
        is_closing_ = true;
        frame::builder b;
        std::vector<uint8_t> close_frame =
            b.build_close(status_code, reason, outbound_mask());
        try {
            socket_.write(close_frame.data(), close_frame.size());
        } catch (...) {
        }
        is_open_ = false;
        try {
            socket_.close();
        } catch (...) {
        }
        join_reader_if_needed();
        if (on_close_) {
            on_close_(status_code, reason);
        }
    }

    void send_text(const std::string& text, bool fin) {
        send_data_frame(frame::opcode::TEXT,
                        reinterpret_cast<const uint8_t*>(text.data()), text.size(), fin);
    }

    void send_binary(const uint8_t* data, size_t size, bool fin) {
        send_data_frame(frame::opcode::BINARY, data, size, fin);
    }

    void send_continuation(const uint8_t* data, size_t size, bool fin) {
        send_data_frame(frame::opcode::CONTINUATION, data, size, fin);
    }

    void send_ping(const uint8_t* data, size_t size) {
        if (!is_open_) {
            throw std::runtime_error("Connection not open");
        }
        frame::builder b;
        const std::vector<uint8_t> frame =
            b.build_ping(data, size, outbound_mask());
        socket_.write(frame.data(), frame.size());
    }

    void set_role(connection_role role) { role_ = role; }

    void set_on_open(open_callback cb) { on_open_ = cb; }
    void set_on_message(message_callback cb) { on_message_ = cb; }
    void set_on_close(close_callback cb) { on_close_ = cb; }
    void set_on_error(error_callback cb) { on_error_ = cb; }

    bool is_open() const { return is_open_; }
    bool is_ssl() const { return socket_.is_ssl(); }
    socket_type& socket() { return socket_; }
    const socket_type& socket() const { return socket_; }

private:
    bool outbound_mask() const {
        return role_ == connection_role::client;
    }

    void send_data_frame(frame::opcode op, const uint8_t* data, size_t size, bool fin) {
        if (!is_open_) {
            throw std::runtime_error("Connection not open");
        }
        frame::builder b;
        const std::vector<uint8_t> frame =
            b.build(op, data, size, fin, outbound_mask());
        socket_.write(frame.data(), frame.size());
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

    void fail_with_close(uint16_t code, const std::string& reason) {
        if (on_error_) {
            on_error_(reason);
        }
        if (is_open_ && !is_closing_) {
            is_closing_ = true;
            frame::builder b;
            const std::vector<uint8_t> close_frame =
                b.build_close(code, reason, outbound_mask());
            try {
                socket_.write(close_frame.data(), close_frame.size());
            } catch (...) {
            }
        }
        is_open_ = false;
        try {
            socket_.close();
        } catch (...) {
        }
        if (on_close_) {
            on_close_(code, reason);
        }
    }

    void fail_protocol(const std::string& reason) {
        fail_with_close(1002, reason);
    }

    bool validate_frame_header() {
        if (current_header_.rsv1 || current_header_.rsv2 || current_header_.rsv3) {
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

    void deliver_message(const std::vector<uint8_t>& payload, frame::opcode op) {
        if (op == frame::opcode::TEXT && !detail::is_valid_utf8(payload)) {
            fail_with_close(1007, "Invalid UTF-8 in text frame");
            return;
        }
        if (on_message_) {
            on_message_(payload, op);
        }
    }

    void reset_fragment_state() {
        fragmented_ = false;
        message_buffer_.clear();
        message_opcode_ = frame::opcode::TEXT;
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
        try {
            socket_.close();
        } catch (...) {
        }
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

    void read_loop() {
        while (is_open_ && !is_closing_) {
            try {
                if (!read_one_frame()) {
                    break;
                }
            } catch (const std::exception& ex) {
                if (on_error_) {
                    on_error_(ex.what());
                }
                is_open_ = false;
                break;
            }
        }
    }

    bool read_one_frame() {
        if (!is_open_) {
            return false;
        }
        if (socket_.read(header_buffer_, 2) != 2) {
            is_open_ = false;
            return false;
        }
        parse_header();
        return is_open_;
    }

    void read_header() {
        (void)read_one_frame();
    }

    void parse_header() {
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
            socket_.read(ext_payload_buffer_, 2);
            current_header_.payload_len =
                (static_cast<uint64_t>(ext_payload_buffer_[0]) << 8) |
                ext_payload_buffer_[1];
        } else if (current_header_.payload_len == 127) {
            socket_.read(ext_payload_buffer_, 8);
            current_header_.payload_len = 0;
            for (int i = 0; i < 8; ++i) {
                current_header_.payload_len =
                    (current_header_.payload_len << 8) | ext_payload_buffer_[i];
            }
        }

        if (current_header_.mask) {
            socket_.read(masking_key_buffer_, 4);
            current_header_.masking_key = {{
                masking_key_buffer_[0], masking_key_buffer_[1],
                masking_key_buffer_[2], masking_key_buffer_[3]
            }};
        }

        if (!validate_frame_header()) {
            return;
        }

        read_payload();
    }

    void read_payload() {
        if (current_header_.payload_len == 0) {
            handle_frame();
            return;
        }

        payload_buffer_.resize(static_cast<std::size_t>(current_header_.payload_len));
        socket_.read(payload_buffer_.data(), payload_buffer_.size());

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
            message_buffer_.insert(message_buffer_.end(),
                                   payload_buffer_.begin(), payload_buffer_.end());
            if (current_header_.fin) {
                deliver_message(message_buffer_, message_opcode_);
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
                return;
            }
            deliver_message(payload_buffer_, op);
            return;
        }

        fail_protocol("Unsupported data frame opcode");
    }

    void handle_close_frame() {
        expecting_close_ = true;
        uint16_t status_code = 1000;
        std::string reason;
        if (payload_buffer_.size() >= 2) {
            status_code = static_cast<uint16_t>(
                (static_cast<uint16_t>(payload_buffer_[0]) << 8) |
                payload_buffer_[1]);
            if (!is_valid_close_code(status_code)) {
                status_code = 1002;
            }
            reason = std::string(payload_buffer_.begin() + 2, payload_buffer_.end());
        }
        frame::builder b;
        const std::vector<uint8_t> close_frame =
            b.build_close(status_code, "", outbound_mask());
        try {
            socket_.write(close_frame.data(), close_frame.size());
        } catch (...) {
        }
        is_open_ = false;
        if (on_close_) {
            on_close_(status_code, reason);
        }
    }

    void handle_ping_frame() {
        frame::builder b;
        const std::vector<uint8_t> pong_frame =
            b.build_pong(payload_buffer_.data(), payload_buffer_.size(), outbound_mask());
        socket_.write(pong_frame.data(), pong_frame.size());
    }

    socket_type socket_;
    asio::io_context& io_context_;
    connection_role role_;

    uint8_t header_buffer_[2];
    uint8_t ext_payload_buffer_[8];
    uint8_t masking_key_buffer_[4];

    frame::frame_header current_header_;
    std::vector<uint8_t> payload_buffer_;
    std::vector<uint8_t> message_buffer_;
    bool fragmented_;
    frame::opcode message_opcode_;

    bool is_open_;
    bool is_closing_;
    bool expecting_close_;
    bool reader_running_;
    std::thread reader_thread_;

    open_callback on_open_;
    message_callback on_message_;
    close_callback on_close_;
    error_callback on_error_;
};

connection::connection(asio::io_context& io_context)
    : pimpl_(detail::make_unique<impl>(io_context)) {}

connection::~connection() = default;

void connection::connect(const std::string& host, const std::string& port) {
    pimpl_->connect(host, port);
}

void connection::connect(const tcp::endpoint& endpoint) {
    pimpl_->connect(endpoint);
}

void connection::adopt(tcp::socket socket) {
    pimpl_->adopt(std::move(socket));
}

void connection::activate() {
    pimpl_->activate();
}

void connection::start_reading() {
    pimpl_->start_reading();
}

void connection::close(uint16_t status_code, const std::string& reason) {
    pimpl_->close(status_code, reason);
}

void connection::send_text(const std::string& text, bool fin) {
    pimpl_->send_text(text, fin);
}

void connection::send_binary(const uint8_t* data, size_t size, bool fin) {
    pimpl_->send_binary(data, size, fin);
}

void connection::send_continuation(const uint8_t* data, size_t size, bool fin) {
    pimpl_->send_continuation(data, size, fin);
}

void connection::send_ping(const uint8_t* data, size_t size) {
    pimpl_->send_ping(data, size);
}

void connection::set_role(connection_role role) {
    pimpl_->set_role(role);
}

void connection::set_on_open(open_callback cb) {
    pimpl_->set_on_open(cb);
}

void connection::set_on_message(message_callback cb) {
    pimpl_->set_on_message(cb);
}

void connection::set_on_close(close_callback cb) {
    pimpl_->set_on_close(cb);
}

void connection::set_on_error(error_callback cb) {
    pimpl_->set_on_error(cb);
}

bool connection::is_open() const {
    return pimpl_->is_open();
}

bool connection::is_ssl() const {
    return pimpl_->is_ssl();
}

connection::socket_type& connection::socket() {
    return pimpl_->socket();
}

const connection::socket_type& connection::socket() const {
    return pimpl_->socket();
}

} // namespace wscpp
