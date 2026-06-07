cmake_minimum_required(VERSION 3.10)

function(wscpp_add_bench target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE wscpp Threads::Threads)
endfunction()

wscpp_add_bench(bench_frame_parse bench_frame_parse.cpp)
wscpp_add_bench(bench_masking bench_masking.cpp)
wscpp_add_bench(bench_roundtrip bench_roundtrip.cpp)

add_custom_target(benchmarks
    DEPENDS bench_frame_parse bench_masking bench_roundtrip
    COMMENT "Build all wscpp micro-benchmarks"
)

add_subdirectory(compare)
