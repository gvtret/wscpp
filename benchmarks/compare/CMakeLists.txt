include(FetchContent)

set(WSCPP_COMPARE_TARGETS)

# --- websocketpp 0.8.2 ---
FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG 0.8.2
)
FetchContent_MakeAvailable(websocketpp)

add_executable(bench_websocketpp_roundtrip bench_websocketpp_roundtrip.cpp)
target_include_directories(bench_websocketpp_roundtrip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(bench_websocketpp_roundtrip PRIVATE ASIO_STANDALONE)
target_include_directories(bench_websocketpp_roundtrip SYSTEM PRIVATE
    ${websocketpp_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
)
target_link_libraries(bench_websocketpp_roundtrip PRIVATE Threads::Threads)
list(APPEND WSCPP_COMPARE_TARGETS bench_websocketpp_roundtrip)

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
    target_include_directories(bench_ixwebsocket_roundtrip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_include_directories(bench_ixwebsocket_roundtrip SYSTEM PRIVATE
        ${asio_SOURCE_DIR}/asio/include
    )
    target_link_libraries(bench_ixwebsocket_roundtrip PRIVATE ixwebsocket::ixwebsocket Threads::Threads)
    list(APPEND WSCPP_COMPARE_TARGETS bench_ixwebsocket_roundtrip)
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
    target_include_directories(bench_easywsclient_connect PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${easywsclient_SOURCE_DIR}
    )
    target_link_libraries(bench_easywsclient_connect PRIVATE wscpp Threads::Threads)
    list(APPEND WSCPP_COMPARE_TARGETS bench_easywsclient_connect)
endif()

add_custom_target(compare_benchmarks
    DEPENDS ${WSCPP_COMPARE_TARGETS}
    COMMENT "Build all comparative WebSocket benchmarks"
)
