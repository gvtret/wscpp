cmake_minimum_required(VERSION 3.10)

function(wscpp_setup_bench target)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE wscpp Threads::Threads)
    wscpp_target_strict_warnings(${target})
    if(TARGET asio::asio)
        target_link_libraries(${target} PRIVATE asio::asio)
        target_compile_definitions(${target} PRIVATE ASIO_STANDALONE WSCPP_BENCH_HAVE_ASIO)
    endif()
    if(WSCPP_USE_ASIO)
        target_compile_definitions(${target} PRIVATE WSCPP_USE_ASIO=1)
    else()
        target_compile_definitions(${target} PRIVATE WSCPP_USE_ASIO=0)
    endif()
endfunction()

function(wscpp_add_bench target source)
    add_executable(${target} ${source})
    wscpp_setup_bench(${target})
endfunction()

wscpp_add_bench(bench_frame_parse bench_frame_parse.cpp)
wscpp_add_bench(bench_masking bench_masking.cpp)
wscpp_add_bench(bench_roundtrip bench_roundtrip.cpp)
wscpp_add_bench(bench_echo_server bench_echo_server.cpp)
wscpp_add_bench(bench_wscpp_connect_server bench_wscpp_connect_server.cpp)
wscpp_add_bench(bench_roundtrip_net bench_roundtrip_net.cpp)

set(WSCPP_BENCH_TARGETS bench_frame_parse bench_masking bench_roundtrip bench_echo_server
    bench_wscpp_connect_server bench_roundtrip_net)

add_custom_target(benchmarks
    DEPENDS ${WSCPP_BENCH_TARGETS}
    COMMENT "Build all wscpp micro-benchmarks"
)

add_subdirectory(compare)

set(WSCPP_ALL_BENCH_TARGETS ${WSCPP_BENCH_TARGETS})
if(WSCPP_COMPARE_TARGETS)
    list(APPEND WSCPP_ALL_BENCH_TARGETS ${WSCPP_COMPARE_TARGETS})
endif()

add_custom_target(run_benchmarks
    COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_CURRENT_SOURCE_DIR}/run_benchmarks.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEPENDS benchmarks compare_benchmarks
    COMMENT "Run all built WebSocket benchmarks"
)
