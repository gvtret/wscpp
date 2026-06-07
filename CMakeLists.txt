cmake_minimum_required(VERSION 3.10)

# Project name and version
project(wscpp
    VERSION 1.0.0
    DESCRIPTION "Lightweight WebSocket stack for C++11 and above"
    LANGUAGES CXX
)

# Read version from VERSION file
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION WSCPP_VERSION_STRING)
string(STRIP "${WSCPP_VERSION_STRING}" WSCPP_VERSION_STRING)
string(REPLACE "." ";" WSCPP_VERSION_LIST "${WSCPP_VERSION_STRING}")
list(LENGTH WSCPP_VERSION_LIST WSCPP_VERSION_LIST_LEN)
if(WSCPP_VERSION_LIST_LEN EQUAL 3)
    list(GET WSCPP_VERSION_LIST 0 WSCPP_VERSION_MAJOR)
    list(GET WSCPP_VERSION_LIST 1 WSCPP_VERSION_MINOR)
    list(GET WSCPP_VERSION_LIST 2 WSCPP_VERSION_PATCH)
else()
    message(FATAL_ERROR "VERSION file must contain MAJOR.MINOR.PATCH")
endif()

# Sync project() version with VERSION file
set(PROJECT_VERSION ${WSCPP_VERSION_STRING})

# Set C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler-specific warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Installation directories
include(GNUInstallDirs)

# Find dependencies
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Fetch ASIO
include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-20-0
)
FetchContent_MakeAvailable(asio)

# Create imported target if not exists
if(NOT TARGET asio::asio)
    FetchContent_GetProperties(asio)
    add_library(asio::asio INTERFACE IMPORTED)
    set_target_properties(asio::asio PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${asio_SOURCE_DIR}/asio/include"
    )
endif()

# Add subdirectories
add_subdirectory(src)

# Add tests if enabled
option(WSCPP_BUILD_TESTS "Build tests" ON)
option(WSCPP_ENABLE_STRESS_TESTS "Build stress integration tests" OFF)
if(WSCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Add examples if enabled
option(WSCPP_BUILD_EXAMPLES "Build examples" ON)
if(WSCPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Benchmarks (scaffold)
option(WSCPP_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(WSCPP_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# API documentation (Doxygen)
option(WSCPP_BUILD_DOCS "Build API documentation with Doxygen" ON)
if(WSCPP_BUILD_DOCS)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        set(WSCPP_VERSION ${WSCPP_VERSION_STRING})
        configure_file(
            ${CMAKE_SOURCE_DIR}/Doxyfile.in
            ${CMAKE_BINARY_DIR}/Doxyfile
            @ONLY
        )
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating Doxygen API documentation"
            VERBATIM
        )
    else()
        message(STATUS "Doxygen not found — docs target disabled")
    endif()
endif()

# Configure pkg-config
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/wscpp.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/wscpp.pc
    @ONLY
)

# Installation
install(TARGETS wscpp
    EXPORT wscppTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/wscpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install CMake config
install(EXPORT wscppTargets
    FILE wscppTargets.cmake
    NAMESPACE wscpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wscpp
)

# Install pkg-config file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wscpp.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Generate version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/wscpp_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/wscpp_version.h
    @ONLY
)

# Install version header
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wscpp_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/wscpp
)

# Generate package version file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/wscppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/wscppConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wscpp
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/wscppConfigVersion.cmake
    VERSION ${WSCPP_VERSION_STRING}
    COMPATIBILITY SameMajorVersion
)

# Install CMake config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/wscppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/wscppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wscpp
)

# Print summary
message(STATUS "")
message(STATUS "wscpp v${WSCPP_VERSION_STRING}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "  ASIO: ${asio_SOURCE_DIR}/asio/include")
message(STATUS "")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
