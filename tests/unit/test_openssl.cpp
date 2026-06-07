// test_openssl.cpp
// Unit test for OpenSSL integration

#include <gtest/gtest.h>
#include <wscpp/crypto/ssl_context.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/error.hpp>
#include <asio/ssl/verify_mode.hpp>
#include <string>

// Test basic SSL context creation
TEST(OpenSSLTest, CreateSSLContext) {
    wscpp::crypto::ssl_context ctx;
    EXPECT_NE(ctx.native_handle().native_handle(), nullptr);
}

// Test SSL context with specific method
TEST(OpenSSLTest, CreateSSLContextWithMethod) {
    wscpp::crypto::ssl_context ctx(wscpp::crypto::ssl_context::method::tlsv12);
    EXPECT_NE(ctx.native_handle().native_handle(), nullptr);
}

// Test SSL context move
TEST(OpenSSLTest, MoveSSLContext) {
    wscpp::crypto::ssl_context ctx1(wscpp::crypto::ssl_context::method::tlsv12);
    wscpp::crypto::ssl_context ctx2(std::move(ctx1));
    EXPECT_NE(ctx2.native_handle().native_handle(), nullptr);
}

// Test SSL context copy prevention (compile-time check)
// This test would fail to compile if copy constructor is not deleted
/*
TEST(OpenSSLTest, CopySSLContext) {
    wscpp::crypto::ssl_context ctx1;
    wscpp::crypto::ssl_context ctx2 = ctx1; // Should not compile
}
*/

// Test set_verify_mode
TEST(OpenSSLTest, SetVerifyMode) {
    wscpp::crypto::ssl_context ctx;
    ctx.set_verify_mode(asio::ssl::verify_none);
    ctx.set_verify_mode(asio::ssl::verify_peer);
}

// Test set_verify_depth
TEST(OpenSSLTest, SetVerifyDepth) {
    wscpp::crypto::ssl_context ctx;
    ctx.set_verify_depth(5);
}

// Test set_options
TEST(OpenSSLTest, SetOptions) {
    wscpp::crypto::ssl_context ctx;
    // Set common SSL options
    ctx.set_options(
        asio::ssl::context::no_sslv2 |
        asio::ssl::context::no_sslv3 |
        asio::ssl::context::no_compression);
}

// Test shared_context
TEST(OpenSSLTest, SharedContext) {
    wscpp::crypto::ssl_context ctx;
    auto shared = ctx.shared_context();
    EXPECT_NE(shared, nullptr);
    EXPECT_EQ(shared.use_count(), 2); // One from ctx, one from shared
}

// Test native_handle
TEST(OpenSSLTest, NativeHandle) {
    wscpp::crypto::ssl_context ctx;
    asio::ssl::context& native = ctx.native_handle();
    const asio::ssl::context& native_const = ctx.native_handle();
    
    EXPECT_EQ(&native, &native_const);
}

// Test multiple context methods
TEST(OpenSSLTest, MultipleMethods) {
    wscpp::crypto::ssl_context ctx1(wscpp::crypto::ssl_context::method::tlsv12);
    wscpp::crypto::ssl_context ctx2(wscpp::crypto::ssl_context::method::tlsv11);
    wscpp::crypto::ssl_context ctx3(wscpp::crypto::ssl_context::method::tls_client);
    wscpp::crypto::ssl_context ctx4(wscpp::crypto::ssl_context::method::tls_server);
    
    EXPECT_NE(ctx1.native_handle().native_handle(), nullptr);
    EXPECT_NE(ctx2.native_handle().native_handle(), nullptr);
    EXPECT_NE(ctx3.native_handle().native_handle(), nullptr);
    EXPECT_NE(ctx4.native_handle().native_handle(), nullptr);
}
