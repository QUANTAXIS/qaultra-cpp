# Tests CMakeLists.txt

# Find required testing frameworks
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Common test sources for unified system
set(TEST_SOURCES
    test_main.cpp
    # 核心组件测试
    test_unified_account.cpp
    test_position.cpp
    test_order.cpp
    test_market_preset.cpp
    test_qifi_protocol.cpp
    # 数据类型测试
    test_unified_datatype.cpp
    # 引擎测试
    test_unified_backtest_engine.cpp
    test_event_engine.cpp
    # 集成测试
    test_trading_integration.cpp
    test_portfolio_management.cpp
    # 性能相关测试
    test_performance_metrics.cpp
    test_thread_safety.cpp
)

# Create test executable
add_executable(qaultra_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(qaultra_tests
    PRIVATE
    qaultra
    GTest::gtest
    GTest::gtest_main
    benchmark::benchmark
)

# Set test properties
set_target_properties(qaultra_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(qaultra_tests PRIVATE
        -O3
        -march=native
        -ffast-math
    )
endif()

# Add test discovery
include(GoogleTest)
gtest_discover_tests(qaultra_tests)

# Benchmark executable
add_executable(qaultra_benchmarks
    benchmark_main.cpp
    benchmark_simd.cpp
    benchmark_account.cpp
    benchmark_market.cpp
    benchmark_memory.cpp
)

target_link_libraries(qaultra_benchmarks
    PRIVATE
    qaultra
    benchmark::benchmark
    benchmark::benchmark_main
)

set_target_properties(qaultra_benchmarks PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

# Performance test executable (for profiling)
add_executable(qaultra_perf_tests
    perf_test_main.cpp
    perf_test_trading.cpp
    perf_test_calculations.cpp
)

target_link_libraries(qaultra_perf_tests
    PRIVATE
    qaultra
)

# Memory leak tests (if Valgrind is available)
find_program(VALGRIND_PROGRAM valgrind)
if(VALGRIND_PROGRAM)
    add_custom_target(memcheck
        COMMAND ${VALGRIND_PROGRAM} --tool=memcheck --leak-check=full --show-leak-kinds=all
                --track-origins=yes --verbose $<TARGET_FILE:qaultra_tests>
        DEPENDS qaultra_tests
        COMMENT "Running memory leak tests with Valgrind"
    )
endif()

# Thread sanitizer tests
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_executable(qaultra_tests_tsan ${TEST_SOURCES})
    target_link_libraries(qaultra_tests_tsan
        PRIVATE
        qaultra
        GTest::gtest
        GTest::gtest_main
    )
    target_compile_options(qaultra_tests_tsan PRIVATE -fsanitize=thread -g)
    target_link_options(qaultra_tests_tsan PRIVATE -fsanitize=thread)

    add_custom_target(test_threading
        COMMAND $<TARGET_FILE:qaultra_tests_tsan>
        DEPENDS qaultra_tests_tsan
        COMMENT "Running threading tests with ThreadSanitizer"
    )
endif()

# Custom test targets
add_custom_target(test_quick
    COMMAND $<TARGET_FILE:qaultra_tests> --gtest_filter="*Quick*"
    DEPENDS qaultra_tests
    COMMENT "Running quick tests only"
)

add_custom_target(test_performance
    COMMAND $<TARGET_FILE:qaultra_benchmarks> --benchmark_min_time=1
    DEPENDS qaultra_benchmarks
    COMMENT "Running performance benchmarks"
)

add_custom_target(test_all
    COMMAND $<TARGET_FILE:qaultra_tests>
    COMMAND $<TARGET_FILE:qaultra_benchmarks> --benchmark_min_time=0.1
    DEPENDS qaultra_tests qaultra_benchmarks
    COMMENT "Running all tests and benchmarks"
)