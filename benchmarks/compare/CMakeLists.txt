include(FetchContent)

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
