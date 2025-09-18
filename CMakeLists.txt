cmake_minimum_required(VERSION 3.20)

project(qaultra-cpp
    VERSION 1.0.0
    DESCRIPTION "High-performance quantitative trading system in C++ - QARS port"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Build options
option(QAULTRA_BUILD_TESTS "Build tests" ON)
option(QAULTRA_BUILD_EXAMPLES "Build examples" ON)
option(QAULTRA_BUILD_PYTHON_BINDINGS "Build Python bindings with pybind11" ON)
option(QAULTRA_BUILD_BENCHMARKS "Build benchmarks" ON)
option(QAULTRA_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(QAULTRA_ENABLE_MIMALLOC "Use mimalloc allocator" ON)
option(QAULTRA_ENABLE_NATIVE "Enable native CPU optimizations" ON)
option(QAULTRA_ENABLE_LTO "Enable Link Time Optimization" ON)
option(QAULTRA_ENABLE_ASAN "Enable Address Sanitizer (Debug only)" OFF)
option(QAULTRA_ENABLE_TSAN "Enable Thread Sanitizer (Debug only)" OFF)

# Platform detection
if(WIN32)
    set(QAULTRA_PLATFORM_WINDOWS ON)
elseif(APPLE)
    set(QAULTRA_PLATFORM_MACOS ON)
else()
    set(QAULTRA_PLATFORM_LINUX ON)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

    if(QAULTRA_ENABLE_NATIVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    endif()

    if(QAULTRA_ENABLE_SIMD)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -mavx -mavx2")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GCC" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "7.0")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -mavx512cd")
        endif()
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -ffast-math")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g -DNDEBUG")

    if(QAULTRA_ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
    endif()

    # Sanitizers
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(QAULTRA_ENABLE_ASAN)
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
            set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
        endif()

        if(QAULTRA_ENABLE_TSAN)
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
            set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=thread")
        endif()
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")

    if(QAULTRA_ENABLE_SIMD)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
    endif()
endif()

# Find required packages
find_package(Threads REQUIRED)

# Try to find packages with fallback to FetchContent
include(FetchContent)

# nlohmann/json
find_package(nlohmann_json 3.11.0 QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Apache Arrow C++
find_package(Arrow QUIET)
if(NOT Arrow_FOUND)
    message(STATUS "Arrow not found, using FetchContent...")
    set(ARROW_BUILD_TESTS OFF CACHE BOOL "")
    set(ARROW_BUILD_BENCHMARKS OFF CACHE BOOL "")
    set(ARROW_COMPUTE ON CACHE BOOL "")
    set(ARROW_CSV ON CACHE BOOL "")
    set(ARROW_DATASET ON CACHE BOOL "")
    set(ARROW_FILESYSTEM ON CACHE BOOL "")
    set(ARROW_JSON ON CACHE BOOL "")
    set(ARROW_PARQUET ON CACHE BOOL "")
    set(ARROW_WITH_SNAPPY ON CACHE BOOL "")
    set(ARROW_WITH_ZLIB ON CACHE BOOL "")
    set(ARROW_WITH_LZ4 ON CACHE BOOL "")
    set(ARROW_WITH_ZSTD ON CACHE BOOL "")

    FetchContent_Declare(
        arrow
        GIT_REPOSITORY https://github.com/apache/arrow.git
        GIT_TAG apache-arrow-14.0.0
        SOURCE_SUBDIR cpp
    )
    FetchContent_MakeAvailable(arrow)
endif()

# spdlog for logging
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# fmt (required by spdlog)
find_package(fmt QUIET)
if(NOT fmt_FOUND)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.1.1
    )
    FetchContent_MakeAvailable(fmt)
endif()

# Intel TBB for threading
find_package(TBB QUIET)
if(NOT TBB_FOUND)
    FetchContent_Declare(
        tbb
        GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
        GIT_TAG v2021.10.0
    )
    FetchContent_MakeAvailable(tbb)
endif()

# mimalloc for high-performance memory allocation
if(QAULTRA_ENABLE_MIMALLOC)
    find_package(mimalloc 2.0 QUIET)
    if(NOT mimalloc_FOUND)
        FetchContent_Declare(
            mimalloc
            GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
            GIT_TAG v2.1.2
        )
        FetchContent_MakeAvailable(mimalloc)
    endif()
endif()

# pybind11 for Python bindings
if(QAULTRA_BUILD_PYTHON_BINDINGS)
    find_package(pybind11 QUIET)
    if(NOT pybind11_FOUND)
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.11.1
        )
        FetchContent_MakeAvailable(pybind11)
    endif()
endif()

# Google Test for testing
if(QAULTRA_BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

# Google Benchmark for benchmarking
if(QAULTRA_BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    if(NOT benchmark_FOUND)
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
        FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.8.3
        )
        FetchContent_MakeAvailable(benchmark)
    endif()
endif()

# MongoDB C++ driver (optional)
find_package(mongocxx QUIET)
find_package(bsoncxx QUIET)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Collect source files
file(GLOB_RECURSE QAULTRA_SOURCES
    "src/account/*.cpp"
    "src/market/*.cpp"
    "src/engine/*.cpp"
    "src/data/*.cpp"
    "src/protocol/*.cpp"
    "src/connector/*.cpp"
    "src/service/*.cpp"
    "src/analysis/*.cpp"
    "src/util/*.cpp"
    "src/env/*.cpp"
    "src/log/*.cpp"
    "src/simd/*.cpp"
    "src/arrow/*.cpp"
    "src/threading/*.cpp"
    "src/memory/*.cpp"
)

file(GLOB_RECURSE QAULTRA_HEADERS
    "include/qaultra/*.hpp"
)

# Create main library
add_library(qaultra STATIC
    ${QAULTRA_SOURCES}
    ${QAULTRA_HEADERS}
)

# Alias for namespaced usage
add_library(qaultra::qaultra ALIAS qaultra)

# Target compile definitions
target_compile_definitions(qaultra PRIVATE
    QAULTRA_VERSION="${PROJECT_VERSION}"
    $<$<CONFIG:Debug>:QAULTRA_DEBUG>
    $<$<CONFIG:Release>:QAULTRA_RELEASE>
)

if(QAULTRA_ENABLE_SIMD)
    target_compile_definitions(qaultra PUBLIC QAULTRA_ENABLE_SIMD)
endif()

if(QAULTRA_ENABLE_MIMALLOC AND mimalloc_FOUND)
    target_compile_definitions(qaultra PUBLIC QAULTRA_USE_MIMALLOC)
endif()

# Link libraries
target_link_libraries(qaultra
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
        arrow_shared
        spdlog::spdlog
        fmt::fmt
        TBB::tbb
)

if(QAULTRA_ENABLE_MIMALLOC AND TARGET mimalloc)
    target_link_libraries(qaultra PUBLIC mimalloc)
endif()

if(mongocxx_FOUND AND bsoncxx_FOUND)
    target_link_libraries(qaultra PUBLIC mongo::mongocxx_shared mongo::bsoncxx_shared)
    target_compile_definitions(qaultra PUBLIC QAULTRA_ENABLE_MONGODB)
endif()

# Set target properties
set_target_properties(qaultra PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${QAULTRA_HEADERS}"
    POSITION_INDEPENDENT_CODE ON
)

# Include directories for target
target_include_directories(qaultra
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler features
target_compile_features(qaultra PUBLIC cxx_std_20)

# Python bindings
if(QAULTRA_BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

# Build tests
if(QAULTRA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build examples
if(QAULTRA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build benchmarks
if(QAULTRA_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
install(TARGETS qaultra
    EXPORT qaultra-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/qaultra
)

# Install headers preserving directory structure
install(DIRECTORY include/qaultra
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Export configuration
install(EXPORT qaultra-targets
    FILE qaultra-targets.cmake
    NAMESPACE qaultra::
    DESTINATION lib/cmake/qaultra
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qaultra-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/qaultra-config.cmake"
    INSTALL_DESTINATION lib/cmake/qaultra
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/qaultra-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/qaultra-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/qaultra-config-version.cmake"
    DESTINATION lib/cmake/qaultra
)

# Create uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()

# Print build configuration
message(STATUS "")
message(STATUS "QAULTRA C++ Build Configuration:")
message(STATUS "================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  Build Tests: ${QAULTRA_BUILD_TESTS}")
message(STATUS "  Build Examples: ${QAULTRA_BUILD_EXAMPLES}")
message(STATUS "  Build Python Bindings: ${QAULTRA_BUILD_PYTHON_BINDINGS}")
message(STATUS "  Build Benchmarks: ${QAULTRA_BUILD_BENCHMARKS}")
message(STATUS "  Enable SIMD: ${QAULTRA_ENABLE_SIMD}")
message(STATUS "  Enable Mimalloc: ${QAULTRA_ENABLE_MIMALLOC}")
message(STATUS "  Enable Native Optimizations: ${QAULTRA_ENABLE_NATIVE}")
message(STATUS "  Enable LTO: ${QAULTRA_ENABLE_LTO}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  Arrow: ${Arrow_FOUND}")
message(STATUS "  MongoDB: ${mongocxx_FOUND}")
message(STATUS "  Mimalloc: ${mimalloc_FOUND}")
message(STATUS "  TBB: ${TBB_FOUND}")
message(STATUS "")