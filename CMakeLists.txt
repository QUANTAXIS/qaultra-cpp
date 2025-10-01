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
option(QAULTRA_USE_ICEORYX "Use IceOryx for zero-copy IPC" ON)
option(QAULTRA_USE_ICEORYX2 "Use iceoryx2 for zero-copy IPC" ON)

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

# MongoDB 依赖 (可选)
option(QAULTRA_USE_MONGODB "Use MongoDB connector" OFF)
if(QAULTRA_USE_MONGODB)
    find_package(mongocxx QUIET)
    find_package(bsoncxx QUIET)
    if(mongocxx_FOUND AND bsoncxx_FOUND)
        set(MONGODB_AVAILABLE TRUE)
        message(STATUS "MongoDB C++ driver found")
    else()
        message(WARNING "MongoDB C++ driver not found. Database features will be disabled.")
        set(MONGODB_AVAILABLE FALSE)
    endif()
endif()

# 可选依赖
if(QAULTRA_USE_ARROW)
    find_package(Arrow QUIET)
    if(Arrow_FOUND)
        set(ARROW_AVAILABLE TRUE)
    endif()
endif()

# IceOryx 依赖
if(QAULTRA_USE_ICEORYX)
    # 设置 IceOryx 路径
    set(iceoryx_posh_DIR "/home/quantaxis/iceoryx/build/install/lib/cmake/iceoryx_posh")
    set(iceoryx_hoofs_DIR "/home/quantaxis/iceoryx/build/install/lib/cmake/iceoryx_hoofs")
    set(iceoryx_platform_DIR "/home/quantaxis/iceoryx/build/install/lib/cmake/iceoryx_platform")

    # 添加 IceOryx 头文件路径
    include_directories("/home/quantaxis/iceoryx/build/install/include")

    find_package(iceoryx_posh QUIET)
    find_package(iceoryx_hoofs QUIET)
    find_package(iceoryx_platform QUIET)

    if(iceoryx_posh_FOUND AND iceoryx_hoofs_FOUND)
        set(ICEORYX_AVAILABLE TRUE)
        message(STATUS "IceOryx found: ${iceoryx_posh_DIR}")
    else()
        message(WARNING "IceOryx not found. Zero-copy IPC features will be disabled.")
        set(ICEORYX_AVAILABLE FALSE)
    endif()
endif()

# iceoryx2 依赖
if(QAULTRA_USE_ICEORYX2)
    # 添加 iceoryx2 头文件路径
    set(ICEORYX2_INCLUDE_DIR "/home/quantaxis/iceoryx2/iceoryx2-cxx/include")
    set(ICEORYX2_C_INCLUDE_DIR "/home/quantaxis/iceoryx2/build/rust/native/release/iceoryx2-ffi-c-cbindgen/include")
    set(ICEORYX2_LIB_DIR "/home/quantaxis/iceoryx2/build/iceoryx2-cxx")

    include_directories(${ICEORYX2_INCLUDE_DIR})
    include_directories(${ICEORYX2_C_INCLUDE_DIR})

    # 检查库文件是否存在
    if(EXISTS "${ICEORYX2_LIB_DIR}/libiceoryx2_cxx.a")
        set(ICEORYX2_AVAILABLE TRUE)
        message(STATUS "iceoryx2 found: ${ICEORYX2_LIB_DIR}/libiceoryx2_cxx.a")
    else()
        message(WARNING "iceoryx2 not found. iceoryx2 IPC features will be disabled.")
        set(ICEORYX2_AVAILABLE FALSE)
    endif()
endif()

# 收集源文件 - 使用统一版本
set(UNIFIED_SOURCES
    # 统一数据类型系统
    "src/data/datatype.cpp"
    "src/data/kline.cpp"

    # 统一账户系统
    "src/account/qa_account.cpp"
    "src/account/marketpreset.cpp"
    "src/account/batch_operations.cpp"


    # 协议支持（只包含存在的文件）
    "src/protocol/mifi.cpp"
    "src/protocol/tifi.cpp"

    # 分析模块
    "src/analysis/performance_analyzer.cpp"

    # 连接器
    "src/connector/database_connector.cpp"
)

# MongoDB 连接器 (可选)
if(MONGODB_AVAILABLE)
    list(APPEND UNIFIED_SOURCES
        "src/connector/mongodb_connector.cpp"
    )
endif()

# IceOryx IPC 模块 (v1)
if(ICEORYX_AVAILABLE)
    list(APPEND UNIFIED_SOURCES
        "src/ipc/broadcast_hub_v1.cpp"
    )
endif()

# iceoryx2 IPC 模块 (v2)
if(ICEORYX2_AVAILABLE)
    list(APPEND UNIFIED_SOURCES
        "src/ipc/broadcast_hub_v2.cpp"
        "src/ipc/cross_lang_data.cpp"
    )
endif()

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

# IceOryx 链接
if(ICEORYX_AVAILABLE)
    target_link_libraries(qaultra PUBLIC
        iceoryx_posh::iceoryx_posh
        iceoryx_hoofs::iceoryx_hoofs
        iceoryx_platform::iceoryx_platform
    )
    target_compile_definitions(qaultra PUBLIC QAULTRA_HAVE_ICEORYX)
endif()

# MongoDB 链接
if(MONGODB_AVAILABLE)
    target_link_libraries(qaultra PUBLIC
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
    )
    target_compile_definitions(qaultra PUBLIC QAULTRA_HAVE_MONGODB)
endif()

# iceoryx2 链接
if(ICEORYX2_AVAILABLE)
    set(ICEORYX2_DEPS_DIR "/home/quantaxis/iceoryx2/build/_deps/iceoryx-build")

    target_link_libraries(qaultra PUBLIC
        ${ICEORYX2_LIB_DIR}/libiceoryx2_cxx.a
        /home/quantaxis/iceoryx2/build/rust/native/release/libiceoryx2_ffi_c.a
        ${ICEORYX2_DEPS_DIR}/iceoryx_hoofs/libiceoryx_hoofs.so
        ${ICEORYX2_DEPS_DIR}/iceoryx_platform/libiceoryx_platform.so
        pthread
        dl
        m
        rt
    )
    target_compile_definitions(qaultra PUBLIC QAULTRA_HAVE_ICEORYX2)

    # 添加运行时库路径
    set_target_properties(qaultra PROPERTIES
        BUILD_RPATH "${ICEORYX2_DEPS_DIR}/iceoryx_hoofs:${ICEORYX2_DEPS_DIR}/iceoryx_platform"
        INSTALL_RPATH "${ICEORYX2_DEPS_DIR}/iceoryx_hoofs:${ICEORYX2_DEPS_DIR}/iceoryx_platform"
    )
endif()

# 设置属性
set_target_properties(qaultra PROPERTIES
    VERSION ${PROJECT_VERSION}
    POSITION_INDEPENDENT_CODE ON
)

# 测试
if(QAULTRA_BUILD_TESTS)
    # 查找Google Test框架
    find_package(GTest QUIET)
    find_package(benchmark QUIET)

    if(GTest_FOUND)
        # 综合测试套件
        add_executable(qaultra_tests
            tests/test_main.cpp
            tests/test_unified_account.cpp
            tests/test_position.cpp
            tests/test_order.cpp
            tests/test_market_preset.cpp
            tests/test_qifi_protocol.cpp
            tests/test_unified_backtest_engine.cpp
            tests/test_event_engine.cpp
            tests/test_trading_integration.cpp
            tests/test_portfolio_management.cpp
            tests/test_performance_metrics.cpp
            tests/test_thread_safety.cpp
        )
        target_link_libraries(qaultra_tests qaultra GTest::gtest GTest::gtest_main)

        # 启用Google Test发现
        include(GoogleTest)
        gtest_discover_tests(qaultra_tests)
    endif()

    if(benchmark_FOUND)
        # 性能基准测试
        add_executable(qaultra_benchmarks
            tests/benchmark_main.cpp
            tests/benchmark_account.cpp
            tests/benchmark_market.cpp
            tests/benchmark_simd.cpp
            tests/benchmark_memory.cpp
        )
        target_link_libraries(qaultra_benchmarks qaultra benchmark::benchmark benchmark::benchmark_main)
    endif()

    # 基础测试（不依赖外部框架）
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

    # 批量操作测试 (需要 GTest)
    if(GTest_FOUND)
        add_executable(batch_operations_test tests/test_batch_operations.cpp)
        target_link_libraries(batch_operations_test qaultra GTest::gtest GTest::gtest_main)
        message(STATUS "Batch operations tests enabled")
    endif()

    # 性能分析测试
    add_executable(performance_analysis_test tests/test_performance_analysis.cpp)
    target_link_libraries(performance_analysis_test qaultra)

    # 数据库连接器测试
    add_executable(database_connector_test tests/test_database_connector.cpp)
    target_link_libraries(database_connector_test qaultra)

    # MongoDB 连接器测试 (需要 GTest)
    if(MONGODB_AVAILABLE AND GTest_FOUND)
        add_executable(mongodb_connector_test tests/test_mongodb_connector.cpp)
        target_link_libraries(mongodb_connector_test qaultra GTest::gtest GTest::gtest_main)
        message(STATUS "MongoDB connector tests enabled")
    endif()

    # IceOryx 广播测试
    if(ICEORYX_AVAILABLE)
        add_executable(broadcast_basic_test tests/test_broadcast_basic.cpp)
        target_link_libraries(broadcast_basic_test qaultra)

        add_executable(broadcast_massive_scale_test tests/test_broadcast_massive_scale.cpp)
        target_link_libraries(broadcast_massive_scale_test qaultra)
    endif()

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

    # 跨语言IPC示例 (iceoryx2)
    if(ICEORYX2_AVAILABLE)
        add_executable(cross_lang_publisher examples/cross_lang_publisher.cpp)
        target_link_libraries(cross_lang_publisher qaultra)

        add_executable(cross_lang_arrow_publisher examples/cross_lang_arrow_publisher.cpp)
        target_link_libraries(cross_lang_arrow_publisher qaultra)

        # 标准 Apache Arrow IPC 示例
        find_package(Arrow QUIET)
        if(Arrow_FOUND)
            add_executable(arrow_ipc_publisher examples/arrow_ipc_publisher.cpp)
            target_link_libraries(arrow_ipc_publisher qaultra arrow_shared)
            message(STATUS "Apache Arrow IPC example enabled")
        endif()
    endif()
endif()

# 跨语言 qadataswap 示例 (独立于 QAULTRA_BUILD_EXAMPLES)
find_package(Arrow QUIET)
if(Arrow_FOUND)
    # qadataswap C++ 发布器
    add_executable(cross_lang_cpp_publisher examples/cross_lang_cpp_publisher.cpp)
    target_link_libraries(cross_lang_cpp_publisher
        ${CMAKE_SOURCE_DIR}/../libs/qadataswap/build/cpp/libqadataswap_core.so
        arrow_shared
    )
    target_include_directories(cross_lang_cpp_publisher PRIVATE
        ${CMAKE_SOURCE_DIR}/../libs/qadataswap/src/cpp/include
    )

    # qadataswap C++ 订阅器
    add_executable(cross_lang_cpp_subscriber examples/cross_lang_cpp_subscriber.cpp)
    target_link_libraries(cross_lang_cpp_subscriber
        ${CMAKE_SOURCE_DIR}/../libs/qadataswap/build/cpp/libqadataswap_core.so
        arrow_shared
    )
    target_include_directories(cross_lang_cpp_subscriber PRIVATE
        ${CMAKE_SOURCE_DIR}/../libs/qadataswap/src/cpp/include
    )

    message(STATUS "Cross-language qadataswap examples enabled")
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
message(STATUS "  Use IceOryx: ${QAULTRA_USE_ICEORYX}")
message(STATUS "  Use iceoryx2: ${QAULTRA_USE_ICEORYX2}")
message(STATUS "  Arrow Available: ${ARROW_AVAILABLE}")
message(STATUS "  IceOryx Available: ${ICEORYX_AVAILABLE}")
message(STATUS "  iceoryx2 Available: ${ICEORYX2_AVAILABLE}")
message(STATUS "")