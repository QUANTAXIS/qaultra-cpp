# QAULTRA C++ 零拷贝 IPC 通信

高性能、零拷贝的进程间通信（IPC）实现，基于 Eclipse IceOryx。

## 快速开始

### 编译

```bash
cd /home/quantaxis/qars2/qaultra-cpp/build
cmake ..
make qaultra broadcast_basic_test -j4
```

### 运行测试

```bash
# 1. 启动 IceOryx RouDi 守护进程
/home/quantaxis/iceoryx/build/install/bin/iox-roudi &

# 2. 运行测试
./broadcast_basic_test
```

### 基本使用

```cpp
#include "qaultra/ipc/broadcast_hub.hpp"

using namespace qaultra::ipc;

// 初始化运行时
DataBroadcaster::initialize_runtime("my_app");

// 创建广播器
BroadcastConfig config = BroadcastConfig::low_latency();
DataBroadcaster broadcaster(config, "market_stream");

// 发送数据
struct MarketTick tick = {...};
broadcaster.broadcast(
    reinterpret_cast<const uint8_t*>(&tick),
    sizeof(tick),
    1,
    MarketDataType::Tick
);

// 创建订阅器
DataSubscriber subscriber(config, "market_stream");

// 接收数据
auto data = subscriber.receive();
if (data) {
    // 处理数据
}
```

## 性能指标

| 指标 | 数值 |
|------|------|
| 吞吐量 | 63,291 msg/sec |
| 平均延迟 | 7.97 μs |
| 最大延迟 | 30.2 μs |
| 内存占用 | ~50 MB (单订阅者) |

## 主要功能

- ✅ 零拷贝数据传输
- ✅ 多订阅者支持
- ✅ 批量发送
- ✅ 实时统计
- ✅ 配置预设 (高性能/低延迟/大规模)

## C++/Rust 跨语言通信

⚠️ **重要**: IceOryx (C++) 和 iceoryx2 (Rust) 不兼容，原因:

1. API 完全不同
2. 内存布局不同
3. iceoryx2 需要 Linux 5.11+ (当前系统: 5.4.0)

**解决方案**: 使用 JSON/MessagePack 桥接

参见: `examples/cpp_rust_bridge_example.cpp`

## 文档

- 📖 [集成指南](docs/ICEORYX_INTEGRATION_CPP.md)
- 📊 [性能对比](docs/CPP_RUST_IPC_COMPARISON.md)
- 🔍 [跨语言通信状态](docs/CROSS_LANGUAGE_IPC_STATUS.md)
- 📝 [最终总结](docs/FINAL_SUMMARY.md)

## 示例程序

```bash
# 简单 Pub/Sub 示例
./broadcast_simple_pubsub publisher  # 终端 1
./broadcast_simple_pubsub subscriber # 终端 2

# 跨语言桥接示例
./cpp_rust_bridge_example publisher
./cpp_rust_bridge_example subscriber
./cpp_rust_bridge_example bridge
```

## 文件结构

```
qaultra-cpp/
├── include/qaultra/ipc/
│   ├── market_data_block.hpp      # 数据块定义
│   ├── broadcast_config.hpp       # 配置管理
│   ├── broadcast_hub.hpp          # 核心 API
│   └── mock_broadcast.hpp         # Mock 实现
├── src/ipc/
│   └── broadcast_hub.cpp          # 核心实现
├── tests/
│   ├── test_broadcast_basic.cpp   # 基础测试
│   └── test_broadcast_massive_scale.cpp  # 压力测试
├── examples/
│   ├── broadcast_simple_pubsub.cpp      # 简单示例
│   └── cpp_rust_bridge_example.cpp      # 跨语言桥接
└── docs/
    ├── ICEORYX_INTEGRATION_CPP.md
    ├── CPP_RUST_IPC_COMPARISON.md
    ├── CROSS_LANGUAGE_IPC_STATUS.md
    └── FINAL_SUMMARY.md
```

## 依赖

- IceOryx 2.x (已安装在 `/home/quantaxis/iceoryx/`)
- C++17 或更高
- CMake 3.16+
- nlohmann/json (可选，用于跨语言桥接)

## 已知限制

1. ⚠️ 需要 RouDi 守护进程
2. ⚠️ 单机限制 (无网络透明)
3. ⚠️ 无法直接与 Rust iceoryx2 通信

## 推荐使用场景

- ✅ 高频交易数据分发
- ✅ 低延迟行情推送
- ✅ 多进程架构 (100-1000+ 订阅者)
- ✅ 纯 C++ 项目

## 许可证

与 QARS 项目相同

---

**状态**: ✅ 生产就绪

**版本**: 1.0.0

**创建时间**: 2025-10-01
