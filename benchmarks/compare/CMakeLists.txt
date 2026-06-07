# Shared compare targets list for parent benchmarks/CMakeLists.txt
set(WSCPP_COMPARE_TARGETS)
set(WSCPP_COMPARE_NET_SERVERS)
set(WSCPP_COMPARE_NET_CLIENTS)

function(wscpp_bench_harness_deps target)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(${target} PRIVATE Threads::Threads)
    wscpp_target_strict_warnings(${target})
    if(TARGET asio::asio)
        target_link_libraries(${target} PRIVATE asio::asio)
        target_compile_definitions(${target} PRIVATE ASIO_STANDALONE WSCPP_BENCH_HAVE_ASIO)
    endif()
endfunction()

function(wscpp_allow_easywsclient_warnings target)
    if(NOT easywsclient_SOURCE_DIR)
        return()
    endif()
    set_source_files_properties(${easywsclient_SOURCE_DIR}/easywsclient.cpp
        PROPERTIES COMPILE_OPTIONS "-Wno-error=unused-parameter")
endfunction()

include(FetchContent)

# --- websocketpp 0.8.2 ---
FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG 0.8.2
)
FetchContent_MakeAvailable(websocketpp)

add_executable(bench_websocketpp_roundtrip bench_websocketpp_roundtrip.cpp)
wscpp_bench_harness_deps(bench_websocketpp_roundtrip)
target_include_directories(bench_websocketpp_roundtrip SYSTEM PRIVATE
    ${websocketpp_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
)
list(APPEND WSCPP_COMPARE_TARGETS bench_websocketpp_roundtrip)

add_executable(bench_websocketpp_echo_server bench_websocketpp_echo_server.cpp)
wscpp_bench_harness_deps(bench_websocketpp_echo_server)
target_include_directories(bench_websocketpp_echo_server SYSTEM PRIVATE
    ${websocketpp_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
)
add_executable(bench_websocketpp_roundtrip_net bench_websocketpp_roundtrip_net.cpp)
wscpp_bench_harness_deps(bench_websocketpp_roundtrip_net)
target_include_directories(bench_websocketpp_roundtrip_net SYSTEM PRIVATE
    ${websocketpp_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
)
list(APPEND WSCPP_COMPARE_NET_SERVERS bench_websocketpp_echo_server)
list(APPEND WSCPP_COMPARE_NET_CLIENTS bench_websocketpp_roundtrip_net)

# --- IXWebSocket 11.4.x ---
option(WSCPP_COMPARE_IXWEBSOCKET "Build IXWebSocket compare benchmark" ON)
if(WSCPP_COMPARE_IXWEBSOCKET)
    set(USE_TLS OFF CACHE INTERNAL "" FORCE)
    set(USE_ZLIB OFF CACHE INTERNAL "" FORCE)
    set(IXWEBSOCKET_INSTALL OFF CACHE INTERNAL "" FORCE)
    FetchContent_Declare(
        ixwebsocket
        GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
        GIT_TAG v11.4.6
    )
    FetchContent_MakeAvailable(ixwebsocket)

    add_executable(bench_ixwebsocket_roundtrip bench_ixwebsocket_roundtrip.cpp)
    wscpp_bench_harness_deps(bench_ixwebsocket_roundtrip)
    target_include_directories(bench_ixwebsocket_roundtrip SYSTEM PRIVATE
        ${asio_SOURCE_DIR}/asio/include
    )
    target_link_libraries(bench_ixwebsocket_roundtrip PRIVATE ixwebsocket::ixwebsocket)
    list(APPEND WSCPP_COMPARE_TARGETS bench_ixwebsocket_roundtrip)

    add_executable(bench_ixwebsocket_echo_server bench_ixwebsocket_echo_server.cpp)
    wscpp_bench_harness_deps(bench_ixwebsocket_echo_server)
    target_include_directories(bench_ixwebsocket_echo_server SYSTEM PRIVATE
        ${asio_SOURCE_DIR}/asio/include
    )
    target_link_libraries(bench_ixwebsocket_echo_server PRIVATE ixwebsocket::ixwebsocket)
    add_executable(bench_ixwebsocket_roundtrip_net bench_ixwebsocket_roundtrip_net.cpp)
    wscpp_bench_harness_deps(bench_ixwebsocket_roundtrip_net)
    target_include_directories(bench_ixwebsocket_roundtrip_net SYSTEM PRIVATE
        ${asio_SOURCE_DIR}/asio/include
    )
    target_link_libraries(bench_ixwebsocket_roundtrip_net PRIVATE ixwebsocket::ixwebsocket)
    list(APPEND WSCPP_COMPARE_NET_SERVERS bench_ixwebsocket_echo_server)
    list(APPEND WSCPP_COMPARE_NET_CLIENTS bench_ixwebsocket_roundtrip_net)
endif()

# --- easywsclient (client connect latency) ---
option(WSCPP_COMPARE_EASYWSCLIENT "Build easywsclient compare benchmark" ON)
if(WSCPP_COMPARE_EASYWSCLIENT)
    FetchContent_Declare(
        easywsclient
        GIT_REPOSITORY https://github.com/dhbaird/easywsclient.git
        GIT_TAG afc1d8cfc584e0f1f4a77e8c0ce3e979d9fe7ce2
    )
    FetchContent_MakeAvailable(easywsclient)

    add_executable(bench_easywsclient_connect
        bench_easywsclient_connect.cpp
        ${easywsclient_SOURCE_DIR}/easywsclient.cpp
    )
    wscpp_setup_bench(bench_easywsclient_connect)
    wscpp_allow_easywsclient_warnings(bench_easywsclient_connect)
    target_include_directories(bench_easywsclient_connect PRIVATE ${easywsclient_SOURCE_DIR})
    list(APPEND WSCPP_COMPARE_TARGETS bench_easywsclient_connect)

    add_executable(bench_easywsclient_connect_net
        bench_easywsclient_connect_net.cpp
        ${easywsclient_SOURCE_DIR}/easywsclient.cpp
    )
    wscpp_setup_bench(bench_easywsclient_connect_net)
    wscpp_allow_easywsclient_warnings(bench_easywsclient_connect_net)
    target_include_directories(bench_easywsclient_connect_net PRIVATE ${easywsclient_SOURCE_DIR})
    list(APPEND WSCPP_COMPARE_NET_CLIENTS bench_easywsclient_connect_net)
endif()

# --- libwebsockets 4.3.x (system pkg-config) ---
option(WSCPP_COMPARE_LIBWEBSOCKETS "Build libwebsockets compare benchmark" ON)
if(WSCPP_COMPARE_LIBWEBSOCKETS)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LWS IMPORTED_TARGET libwebsockets)
    endif()
    if(LWS_FOUND)
        add_executable(bench_libwebsockets_roundtrip bench_libwebsockets_roundtrip.cpp)
        wscpp_bench_harness_deps(bench_libwebsockets_roundtrip)
        target_link_libraries(bench_libwebsockets_roundtrip PRIVATE PkgConfig::LWS)
        list(APPEND WSCPP_COMPARE_TARGETS bench_libwebsockets_roundtrip)

        add_executable(bench_libwebsockets_echo_server bench_libwebsockets_echo_server.cpp)
        wscpp_bench_harness_deps(bench_libwebsockets_echo_server)
        target_link_libraries(bench_libwebsockets_echo_server PRIVATE PkgConfig::LWS)
        add_executable(bench_libwebsockets_roundtrip_net bench_libwebsockets_roundtrip_net.cpp)
        wscpp_bench_harness_deps(bench_libwebsockets_roundtrip_net)
        target_link_libraries(bench_libwebsockets_roundtrip_net PRIVATE PkgConfig::LWS)
        list(APPEND WSCPP_COMPARE_NET_SERVERS bench_libwebsockets_echo_server)
        list(APPEND WSCPP_COMPARE_NET_CLIENTS bench_libwebsockets_roundtrip_net)
    else()
        message(STATUS "libwebsockets not found; skip bench_libwebsockets_roundtrip")
    endif()
endif()

# --- Boost.Beast (system Boost) ---
option(WSCPP_COMPARE_BEAST "Build Boost.Beast compare benchmark" ON)
if(WSCPP_COMPARE_BEAST)
    find_package(Boost 1.70 COMPONENTS system QUIET)
    if(Boost_FOUND)
        add_executable(bench_beast_roundtrip bench_beast_roundtrip.cpp)
        wscpp_bench_harness_deps(bench_beast_roundtrip)
        target_link_libraries(bench_beast_roundtrip PRIVATE Boost::system)
        list(APPEND WSCPP_COMPARE_TARGETS bench_beast_roundtrip)

        add_executable(bench_beast_echo_server bench_beast_echo_server.cpp)
        wscpp_bench_harness_deps(bench_beast_echo_server)
        target_link_libraries(bench_beast_echo_server PRIVATE Boost::system)
        add_executable(bench_beast_roundtrip_net bench_beast_roundtrip_net.cpp)
        wscpp_bench_harness_deps(bench_beast_roundtrip_net)
        target_link_libraries(bench_beast_roundtrip_net PRIVATE Boost::system)
        list(APPEND WSCPP_COMPARE_NET_SERVERS bench_beast_echo_server)
        list(APPEND WSCPP_COMPARE_NET_CLIENTS bench_beast_roundtrip_net)
    else()
        message(STATUS "Boost (system) not found; skip bench_beast_roundtrip")
    endif()
endif()

# --- Simple-WebSocket-Server (FetchContent + standalone ASIO) ---
option(WSCPP_COMPARE_SWS "Build Simple-WebSocket-Server compare benchmark" ON)
if(WSCPP_COMPARE_SWS)
    FetchContent_Declare(
        simple_websocket_server
        GIT_REPOSITORY https://gitlab.com/eidheim/Simple-WebSocket-Server.git
        GIT_TAG 0e1cf67ef0247545386102187af79269da431495
    )
    FetchContent_GetProperties(simple_websocket_server)
    if(NOT simple_websocket_server_POPULATED)
        FetchContent_Populate(simple_websocket_server)
    endif()

    add_library(wscpp_sws INTERFACE)
    target_include_directories(wscpp_sws INTERFACE
        ${simple_websocket_server_SOURCE_DIR}
        ${asio_SOURCE_DIR}/asio/include
    )
    target_compile_definitions(wscpp_sws INTERFACE ASIO_STANDALONE)
    target_link_libraries(wscpp_sws INTERFACE Threads::Threads OpenSSL::SSL OpenSSL::Crypto)

    add_executable(bench_sws_roundtrip bench_sws_roundtrip.cpp)
    wscpp_bench_harness_deps(bench_sws_roundtrip)
    target_link_libraries(bench_sws_roundtrip PRIVATE wscpp_sws)
    list(APPEND WSCPP_COMPARE_TARGETS bench_sws_roundtrip)

    add_executable(bench_sws_echo_server bench_sws_echo_server.cpp)
    wscpp_bench_harness_deps(bench_sws_echo_server)
    target_link_libraries(bench_sws_echo_server PRIVATE wscpp_sws)
    add_executable(bench_sws_roundtrip_net bench_sws_roundtrip_net.cpp)
    wscpp_bench_harness_deps(bench_sws_roundtrip_net)
    target_link_libraries(bench_sws_roundtrip_net PRIVATE wscpp_sws)
    list(APPEND WSCPP_COMPARE_NET_SERVERS bench_sws_echo_server)
    list(APPEND WSCPP_COMPARE_NET_CLIENTS bench_sws_roundtrip_net)
endif()

set(WSCPP_NET_SERVER_TARGETS bench_echo_server bench_wscpp_connect_server
    ${WSCPP_COMPARE_NET_SERVERS})
set(WSCPP_NET_CLIENT_TARGETS bench_roundtrip_net ${WSCPP_COMPARE_NET_CLIENTS})

add_custom_target(compare_net_servers
    DEPENDS ${WSCPP_NET_SERVER_TARGETS}
    COMMENT "Build echo servers for LAN benchmarks"
)

add_custom_target(compare_net_clients
    DEPENDS ${WSCPP_NET_CLIENT_TARGETS}
    COMMENT "Build LAN benchmark clients"
)

add_custom_target(compare_benchmarks
    DEPENDS ${WSCPP_COMPARE_TARGETS}
    COMMENT "Build all comparative WebSocket benchmarks"
)

set(WSCPP_COMPARE_TARGETS ${WSCPP_COMPARE_TARGETS} PARENT_SCOPE)
set(WSCPP_NET_SERVER_TARGETS ${WSCPP_NET_SERVER_TARGETS} PARENT_SCOPE)
set(WSCPP_NET_CLIENT_TARGETS ${WSCPP_NET_CLIENT_TARGETS} PARENT_SCOPE)
