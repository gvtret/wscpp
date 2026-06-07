// wscpp.cpp
// Main source file for wscpp library

#include <wscpp_version.h>

namespace wscpp {

const char* get_version_string() {
    return WSCPP_VERSION_STRING;
}

int get_version_major() {
    return WSCPP_VERSION_MAJOR;
}

int get_version_minor() {
    return WSCPP_VERSION_MINOR;
}

int get_version_patch() {
    return WSCPP_VERSION_PATCH;
}

} // namespace wscpp
