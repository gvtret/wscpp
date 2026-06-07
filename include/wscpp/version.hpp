#ifndef WSCPP_VERSION_HPP
#define WSCPP_VERSION_HPP

/**
 * @file version.hpp
 * @brief Runtime and compile-time version queries.
 */

namespace wscpp {

/** @return SemVer string from build (e.g. "0.1.0"). */
const char *get_version_string();

/** @return Major version component. */
int get_version_major();

/** @return Minor version component. */
int get_version_minor();

/** @return Patch version component. */
int get_version_patch();

} // namespace wscpp

#endif // WSCPP_VERSION_HPP
