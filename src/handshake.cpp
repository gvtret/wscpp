#include <wscpp/handshake.hpp>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>
#include <stdexcept>

namespace wscpp {
namespace handshake {

namespace {

std::string base64_encode(const unsigned char* data, std::size_t len) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    BIO_write(bio, data, static_cast<int>(len));
    BIO_flush(bio);
    BUF_MEM* buf_mem = nullptr;
    BIO_get_mem_ptr(bio, &buf_mem);
    std::string result(buf_mem->data, buf_mem->length);
    BIO_free_all(bio);
    return result;
}

std::string to_lower(const std::string& s) {
    std::string out = s;
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

std::string trim_crlf(const std::string& s) {
    std::string out = s;
    while (!out.empty() && (out.back() == '\r' || out.back() == '\n')) {
        out.pop_back();
    }
    return out;
}

} // namespace

std::string generate_key() {
    unsigned char bytes[16];
    std::random_device rd;
    for (int i = 0; i < 16; ++i) {
        bytes[i] = static_cast<unsigned char>(rd() & 0xFF);
    }
    return base64_encode(bytes, 16);
}

std::string compute_accept(const std::string& key) {
    static const char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic;
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.data()), combined.size(), hash);
    return base64_encode(hash, SHA_DIGEST_LENGTH);
}

std::string build_client_request(
    const std::string& host,
    const std::string& port,
    const std::string& path,
    const std::string& key) {
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host;
    if (!port.empty() && port != "80" && port != "443") {
        request << ":" << port;
    }
    request << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << key << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";
    return request.str();
}

bool parse_http_headers(
    const std::string& raw,
    std::string& request_line,
    std::map<std::string, std::string>& headers) {
    std::istringstream stream(raw);
    if (!std::getline(stream, request_line)) {
        return false;
    }
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (line == "\r" || line.empty()) {
            break;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::size_t pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string name = trim_crlf(line.substr(0, pos));
        std::string value = trim_crlf(line.substr(pos + 1));
        while (!value.empty() && value[0] == ' ') {
            value.erase(0, 1);
        }
        headers[to_lower(name)] = value;
    }
    return true;
}

std::string build_server_response(const std::string& accept_key) {
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << accept_key << "\r\n";
    response << "\r\n";
    return response.str();
}

bool validate_client_request(const std::map<std::string, std::string>& headers) {
    const std::map<std::string, std::string>::const_iterator upgrade =
        headers.find("upgrade");
    const std::map<std::string, std::string>::const_iterator connection =
        headers.find("connection");
    const std::map<std::string, std::string>::const_iterator version =
        headers.find("sec-websocket-version");
    const std::map<std::string, std::string>::const_iterator key =
        headers.find("sec-websocket-key");

    if (upgrade == headers.end() || connection == headers.end() ||
        version == headers.end() || key == headers.end()) {
        return false;
    }
    if (to_lower(upgrade->second) != "websocket") {
        return false;
    }
    if (to_lower(connection->second).find("upgrade") == std::string::npos) {
        return false;
    }
    if (version->second != "13") {
        return false;
    }
    return true;
}

} // namespace handshake
} // namespace wscpp
