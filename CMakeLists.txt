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
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # 尝试从系统包管理器安装的版本
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
    if(NLOHMANN_JSON_INCLUDE_DIR)
        set(nlohmann_json_FOUND TRUE)
    endif()
endif()

# 可选依赖
if(QAULTRA_USE_ARROW)
    find_package(Arrow QUIET)
    if(Arrow_FOUND)
        set(ARROW_AVAILABLE TRUE)
    endif()
endif()

# 收集源文件 - 使用统一版本
set(UNIFIED_SOURCES
    # 统一数据类型系统
    "src/data/unified_datatype.cpp"

    # 统一账户系统
    "src/account/unified_account.cpp"
    "src/account/marketpreset.cpp"

    # 统一回测引擎
    "src/engine/unified_backtest_engine.cpp"

    # 协议支持（只包含存在的文件）
    "src/protocol/mifi.cpp"
    "src/protocol/tifi.cpp"

    # 分析模块
    "src/analysis/performance_analyzer.cpp"

    # 连接器
    "src/connector/database_connector.cpp"
    "src/connector/qa_connector.cpp"
)

# 可选的完整功能源文件
if(QAULTRA_USE_FULL_FEATURES)
    list(APPEND UNIFIED_SOURCES
        # 市场模块（只包含存在的文件）
        "src/market/simmarket.cpp"
        "src/market/match_engine.cpp"

        # 账户扩展功能
        "src/account/account.cpp"
        "src/account/account_full.cpp"
        "src/account/order.cpp"
        "src/account/position.cpp"

        # 数据扩展功能
        "src/data/datatype.cpp"
        "src/data/kline.cpp"
        "src/data/marketcenter.cpp"
    )
endif()

set(QAULTRA_SOURCES ${UNIFIED_SOURCES})

# 创建库
add_library(qaultra STATIC ${QAULTRA_SOURCES})

# 基础链接
target_link_libraries(qaultra PUBLIC Threads::Threads)

# nlohmann_json 链接
if(nlohmann_json_FOUND)
    if(TARGET nlohmann_json::nlohmann_json)
        target_link_libraries(qaultra PUBLIC nlohmann_json::nlohmann_json)
    else()
        target_include_directories(qaultra PUBLIC ${NLOHMANN_JSON_INCLUDE_DIR})
    endif()
    target_compile_definitions(qaultra PUBLIC QAULTRA_HAVE_JSON)
endif()

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
    # 基础测试
    add_executable(progressive_test tests/test_minimal.cpp)
    target_link_libraries(progressive_test qaultra)

    # 协议测试
    add_executable(protocol_test tests/test_protocol.cpp)
    target_link_libraries(protocol_test qaultra)

    # 统一回测引擎测试
    add_executable(unified_backtest_test tests/test_unified_backtest.cpp)
    target_link_libraries(unified_backtest_test qaultra)

    # 统一账户测试
    add_executable(unified_account_test tests/test_unified_account.cpp)
    target_link_libraries(unified_account_test qaultra)

    # 统一数据类型测试
    add_executable(unified_datatype_test tests/test_unified_datatype.cpp)
    target_link_libraries(unified_datatype_test qaultra)

    # 性能分析测试
    add_executable(performance_analysis_test tests/test_performance_analysis.cpp)
    target_link_libraries(performance_analysis_test qaultra)

    # 数据库连接器测试
    add_executable(database_connector_test tests/test_database_connector.cpp)
    target_link_libraries(database_connector_test qaultra)

    # 兼容性测试 - 保留原始测试以确保兼容性
    add_executable(backtest_legacy_test tests/test_backtest_simple.cpp)
    target_link_libraries(backtest_legacy_test qaultra)

    add_executable(full_backtest_legacy_demo tests/test_full_backtest.cpp)
    target_link_libraries(full_backtest_legacy_demo qaultra)
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