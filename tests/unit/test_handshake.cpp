#include <gtest/gtest.h>
#include <wscpp/handshake.hpp>

using namespace wscpp::handshake;

// RFC 6455 section 4.1.5 example
TEST(HandshakeTest, ComputeAcceptRfc6455Example) {
    const std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    EXPECT_EQ(compute_accept(key), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST(HandshakeTest, GenerateKeyIsBase64) {
    const std::string key = generate_key();
    EXPECT_EQ(key.size(), 24u);
    for (char c : key) {
        EXPECT_TRUE(
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '+' || c == '/' || c == '=');
    }
}

TEST(HandshakeTest, BuildClientRequestContainsRequiredHeaders) {
    const std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    const std::string request =
        build_client_request("example.com", "80", "/chat", key);

    EXPECT_NE(request.find("GET /chat HTTP/1.1"), std::string::npos);
    EXPECT_NE(request.find("Host: example.com"), std::string::npos);
    EXPECT_NE(request.find("Upgrade: websocket"), std::string::npos);
    EXPECT_NE(request.find("Connection: Upgrade"), std::string::npos);
    EXPECT_NE(request.find("Sec-WebSocket-Key: " + key), std::string::npos);
    EXPECT_NE(request.find("Sec-WebSocket-Version: 13"), std::string::npos);
    EXPECT_NE(request.find("\r\n\r\n"), std::string::npos);
}

TEST(HandshakeTest, BuildClientRequestNonDefaultPort) {
    const std::string request =
        build_client_request("example.com", "8080", "/", "key");
    EXPECT_NE(request.find("Host: example.com:8080"), std::string::npos);
}

TEST(HandshakeTest, ParseHttpHeaders) {
    const std::string raw =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";

    std::string request_line;
    std::map<std::string, std::string> headers;
    ASSERT_TRUE(parse_http_headers(raw, request_line, headers));

    EXPECT_EQ(request_line, "GET / HTTP/1.1");
    EXPECT_EQ(headers["host"], "example.com");
    EXPECT_EQ(headers["upgrade"], "websocket");
    EXPECT_EQ(headers["connection"], "Upgrade");
    EXPECT_EQ(headers["sec-websocket-key"], "dGhlIHNhbXBsZSBub25jZQ==");
    EXPECT_EQ(headers["sec-websocket-version"], "13");
}

TEST(HandshakeTest, ValidateClientRequestAcceptsValidHeaders) {
    std::map<std::string, std::string> headers;
    headers["upgrade"] = "websocket";
    headers["connection"] = "Upgrade";
    headers["sec-websocket-version"] = "13";
    headers["sec-websocket-key"] = "dGhlIHNhbXBsZSBub25jZQ==";
    EXPECT_TRUE(validate_client_request(headers));
}

TEST(HandshakeTest, ValidateClientRequestRejectsMissingKey) {
    std::map<std::string, std::string> headers;
    headers["upgrade"] = "websocket";
    headers["connection"] = "Upgrade";
    headers["sec-websocket-version"] = "13";
    EXPECT_FALSE(validate_client_request(headers));
}

TEST(HandshakeTest, ValidateClientRequestRejectsWrongVersion) {
    std::map<std::string, std::string> headers;
    headers["upgrade"] = "websocket";
    headers["connection"] = "Upgrade";
    headers["sec-websocket-version"] = "8";
    headers["sec-websocket-key"] = "key";
    EXPECT_FALSE(validate_client_request(headers));
}

TEST(HandshakeTest, BuildServerResponseContainsAccept) {
    const std::string accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
    const std::string response = build_server_response(accept);

    EXPECT_NE(response.find("HTTP/1.1 101 Switching Protocols"), std::string::npos);
    EXPECT_NE(response.find("Upgrade: websocket"), std::string::npos);
    EXPECT_NE(response.find("Connection: Upgrade"), std::string::npos);
    EXPECT_NE(response.find("Sec-WebSocket-Accept: " + accept), std::string::npos);
    EXPECT_NE(response.find("\r\n\r\n"), std::string::npos);
}
