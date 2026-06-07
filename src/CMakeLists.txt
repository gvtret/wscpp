add_library(wscpp STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/wscpp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/net/asio_socket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto/ssl_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frame/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frame/builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handshake.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
)

target_include_directories(wscpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_compile_definitions(wscpp
    PUBLIC
        ASIO_STANDALONE
)

target_link_libraries(wscpp
    PUBLIC
        asio::asio
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

set_target_properties(wscpp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

install(EXPORT wscppTargets
    FILE wscppTargets.cmake
    NAMESPACE wscpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wscpp
)
