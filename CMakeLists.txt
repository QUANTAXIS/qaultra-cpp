cmake_minimum_required(VERSION 3.16)

project(qaultra-cpp VERSION 1.0.0 LANGUAGES CXX)

# 基本设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译标志 - 优化但不过度
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

# 选项 - 逐步启用
option(QAULTRA_BUILD_TESTS "Build tests" ON)
option(QAULTRA_BUILD_EXAMPLES "Build examples" OFF)
option(QAULTRA_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(QAULTRA_USE_ARROW "Use Apache Arrow" OFF)
option(QAULTRA_USE_FULL_FEATURES "Use all features" OFF)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 查找基础依赖
find_package(Threads REQUIRED)

# 可选依赖
if(QAULTRA_USE_ARROW)
    find_package(Arrow QUIET)
    if(Arrow_FOUND)
        set(ARROW_AVAILABLE TRUE)
    endif()
endif()

# 收集源文件 - 根据选项决定包含哪些
if(QAULTRA_USE_FULL_FEATURES)
    # 完整功能 - 所有模块
    file(GLOB_RECURSE ALL_SOURCES
        "src/account/*.cpp"
        "src/market/*.cpp"
        "src/data/*.cpp"
        "src/protocol/*.cpp"
        "src/util/*.cpp"
    )
    set(QAULTRA_SOURCES ${ALL_SOURCES})
else()
    # 基础功能 - 包含稳定模块
    file(GLOB BASIC_SOURCES
        "src/data/datatype_simple.cpp"
        "src/account/marketpreset.cpp"
        "src/protocol/mifi.cpp"
        "src/protocol/tifi.cpp"
        "src/engine/backtest_engine.cpp"
        "src/analysis/performance_analyzer.cpp"
        "src/connector/database_connector.cpp"
        "src/connector/qa_connector.cpp"
    )
    set(QAULTRA_SOURCES ${BASIC_SOURCES})
endif()

# 创建库
add_library(qaultra STATIC ${QAULTRA_SOURCES})

# 基础链接
target_link_libraries(qaultra PUBLIC Threads::Threads)

# 可选链接
if(ARROW_AVAILABLE)
    target_link_libraries(qaultra PUBLIC arrow_shared)
    target_compile_definitions(qaultra PUBLIC QAULTRA_HAVE_ARROW)
endif()

# 设置属性
set_target_properties(qaultra PROPERTIES
    VERSION ${PROJECT_VERSION}
    POSITION_INDEPENDENT_CODE ON
)

# 测试
if(QAULTRA_BUILD_TESTS)
    add_executable(progressive_test tests/test_minimal.cpp)
    target_link_libraries(progressive_test qaultra)

    add_executable(protocol_test tests/test_protocol.cpp)
    target_link_libraries(protocol_test qaultra)

    add_executable(backtest_test tests/test_backtest_simple.cpp)
    target_link_libraries(backtest_test qaultra)

    add_executable(full_backtest_demo tests/test_full_backtest.cpp)
    target_link_libraries(full_backtest_demo qaultra)

    add_executable(performance_analysis_test tests/test_performance_analysis.cpp)
    target_link_libraries(performance_analysis_test qaultra)

    add_executable(database_connector_test tests/test_database_connector.cpp)
    target_link_libraries(database_connector_test qaultra)
endif()

# 示例
if(QAULTRA_BUILD_EXAMPLES)
    add_executable(simple_example simple_test.cpp)
    target_link_libraries(simple_example qaultra)
endif()

# 打印配置信息
message(STATUS "")
message(STATUS "QAULTRA Progressive Build Configuration:")
message(STATUS "======================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests: ${QAULTRA_BUILD_TESTS}")
message(STATUS "  Use Arrow: ${QAULTRA_USE_ARROW}")
message(STATUS "  Use Full Features: ${QAULTRA_USE_FULL_FEATURES}")
message(STATUS "  Arrow Available: ${ARROW_AVAILABLE}")
message(STATUS "")