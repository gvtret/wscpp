add_library(wscpp STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/wscpp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/net/asio_socket.cpp
)

target_include_directories(wscpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${ASIO_INCLUDE_DIRS}
)

target_link_libraries(wscpp
    PUBLIC
        asio::asio
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

# Set target properties
set_target_properties(wscpp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Export target
install(EXPORT wscppTargets
    FILE wscppTargets.cmake
    NAMESPACE wscpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wscpp
)
