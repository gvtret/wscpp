// test_version.cpp
// Unit test for version module

#include <gtest/gtest.h>
#include <wscpp/version.hpp>
#include <string>

TEST(VersionTest, GetVersionString) {
    const char* version = wscpp::get_version_string();
    EXPECT_NE(version, nullptr);
    EXPECT_GT(strlen(version), 0);
}

TEST(VersionTest, GetVersionMajor) {
    int major = wscpp::get_version_major();
    EXPECT_GE(major, 0);
}

TEST(VersionTest, GetVersionMinor) {
    int minor = wscpp::get_version_minor();
    EXPECT_GE(minor, 0);
}

TEST(VersionTest, GetVersionPatch) {
    int patch = wscpp::get_version_patch();
    EXPECT_GE(patch, 0);
}

TEST(VersionTest, VersionComponents) {
    int major = wscpp::get_version_major();
    int minor = wscpp::get_version_minor();
    int patch = wscpp::get_version_patch();
    
    // Basic validation
    EXPECT_TRUE(major >= 0 && minor >= 0 && patch >= 0);
    
    // Synchronized with VERSION file (1.0.1)
    EXPECT_EQ(major, 1);
    EXPECT_EQ(minor, 0);
    EXPECT_EQ(patch, 1);
}
