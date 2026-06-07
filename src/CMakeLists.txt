add_library(wscpp STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wscpp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frame/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frame/builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handshake.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
)

if(WSCPP_USE_ASIO)
    target_sources(wscpp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/net/asio_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto/ssl_context.cpp
    )
else()
    target_sources(wscpp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/net/linux_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/net/openssl_context.cpp
    )
endif()

if(WSCPP_ENABLE_DEFLATE)
    target_sources(wscpp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/permessage_deflate.cpp
    )
    target_link_libraries(wscpp PUBLIC ZLIB::ZLIB)
    target_compile_definitions(wscpp PUBLIC WSCPP_ENABLE_DEFLATE=1)
else()
    target_compile_definitions(wscpp PUBLIC WSCPP_ENABLE_DEFLATE=0)
endif()

target_include_directories(wscpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

if(WSCPP_USE_ASIO)
    target_compile_definitions(wscpp
        PUBLIC
            WSCPP_USE_ASIO=1
            ASIO_STANDALONE
    )
    target_link_libraries(wscpp PUBLIC asio::asio)
else()
    target_compile_definitions(wscpp PUBLIC WSCPP_USE_ASIO=0)
endif()

target_link_libraries(wscpp
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

if(WSCPP_ENABLE_LOGGING)
    target_compile_definitions(wscpp PRIVATE WSCPP_ENABLE_LOGGING=1)
    target_include_directories(wscpp SYSTEM PRIVATE ${spdlog_SOURCE_DIR}/include)
else()
    target_compile_definitions(wscpp PRIVATE WSCPP_ENABLE_LOGGING=0)
endif()

set_target_properties(wscpp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

wscpp_target_strict_warnings(wscpp)
