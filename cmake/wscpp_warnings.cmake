# Strict compiler warnings for first-party wscpp targets only (not FetchContent deps).

option(WSCPP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors on wscpp targets" ON)

function(wscpp_target_strict_warnings target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "wscpp_target_strict_warnings: no target '${target}'")
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(WSCPP_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
        if(WSCPP_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
            # GCC 15: false positive in std::vector after reserve()+push_back (builder.cpp).
            if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15)
                target_compile_options(${target} PRIVATE -Wno-error=free-nonheap-object)
            endif()
        endif()
    endif()
endfunction()
