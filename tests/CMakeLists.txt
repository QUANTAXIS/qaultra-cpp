# Test CMakeLists.txt

# Try to find Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # Fallback to FetchContent
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
endif()

# Test files
set(TEST_SOURCES
    test_kline.cpp
    test_order.cpp
    test_position.cpp
    test_account.cpp
    test_main.cpp
)

# Create test executable
add_executable(qaultra_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(qaultra_tests
    qaultra
    gtest
    gtest_main
    gmock
    Threads::Threads
)

# Include directories
target_include_directories(qaultra_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(qaultra_tests)