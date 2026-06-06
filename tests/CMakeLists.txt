# Tests

option(WSCPP_BUILD_TESTS "Build tests" ON)

if(WSCPP_BUILD_TESTS)
    # Find GoogleTest
    find_package(GTest QUIET)
    
    if(NOT GTEST_FOUND)
        # Use FetchContent to download GoogleTest
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    
    add_subdirectory(unit)
    add_subdirectory(integration)
    add_subdirectory(regression)
endif()
