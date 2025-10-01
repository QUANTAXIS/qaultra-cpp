# QAULTRA C++ - IceOryx 集成指南

## 概述

QAULTRA C++ 使用 **IceOryx** (Eclipse iceoryx) 实现零拷贝进程间通信 (IPC)，提供高性能数据广播能力，对标 QARS Rust 版本的 iceoryx2 集成。

### 核心优势

- ✅ **真零拷贝**: 共享内存架构，无数据序列化和拷贝开销
- ✅ **超低延迟**: P99 延迟 < 10 μs
- ✅ **高吞吐量**: > 500K ticks/sec (500 订阅者场景)
- ✅ **大规模扩展**: 支持 1000+ 并发订阅者
- ✅ **生产级成熟度**: 用于 AUTOSAR Adaptive, ROS 2 等

## 架构设计

### 核心组件

```cpp
qaultra/ipc/
├── market_data_block.hpp    // 零拷贝数据块定义 (8KB 固定大小)
├── broadcast_config.hpp      // 广播配置和统计
└── broadcast_hub.hpp          // DataBroadcaster & DataSubscriber
```

### 数据流

```
┌─────────────────┐
│ Data Source     │
│ (Market Feed)   │
└────────┬────────┘
         │
         ▼
┌─────────────────────────┐
│   DataBroadcaster       │
│  (IceOryx Publisher)    │
└──────────┬──────────────┘
           │ Shared Memory (Zero-Copy)
           ├────────────────────┬────────────────────┐
           ▼                    ▼                    ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│ DataSubscriber 1 │  │ DataSubscriber 2 │  │ DataSubscriber N │
│  (Backtest 1)    │  │  (Backtest 2)    │  │  (Strategy N)    │
└──────────────────┘  └──────────────────┘  └──────────────────┘
```

## 安装 IceOryx

### 从源码编译

```bash
# 1. 克隆 IceOryx
cd /home/quantaxis
git clone https://github.com/eclipse-iceoryx/iceoryx.git
cd iceoryx

# 2. 编译安装
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=./install \
      ..
make -j$(nproc)
make install

# 3. 设置环境变量
export LD_LIBRARY_PATH=/home/quantaxis/iceoryx/build/install/lib:$LD_LIBRARY_PATH
```

### 验证安装

```bash
ls /home/quantaxis/iceoryx/build/install/lib/
# 应该看到: libiceoryx_posh.so, libiceoryx_hoofs.so
```

## 编译 QAULTRA with IceOryx

### 配置构建

```bash
cd /home/quantaxis/qars2/qaultra-cpp
mkdir build && cd build

# 启用 IceOryx 支持
cmake -DQAULTRA_USE_ICEORYX=ON \
      -DCMAKE_BUILD_TYPE=Release \
      ..

make -j$(nproc)
```

### 编译输出

```
-- IceOryx Available: TRUE
-- Building with IceOryx zero-copy IPC support
-- Compiling src/ipc/broadcast_hub.cpp
-- Generating broadcast_basic_test
-- Generating broadcast_massive_scale_test
```

## 快速开始

### 1. 基础 Pub/Sub 示例

```cpp
#include "qaultra/ipc/broadcast_hub.hpp"

using namespace qaultra::ipc;

int main() {
    // 初始化 IceOryx 运行时
    DataBroadcaster::initialize_runtime("my_app");

    // 创建配置
    BroadcastConfig config;
    config.max_subscribers = 100;
    config.batch_size = 1000;

    // 创建广播器
    DataBroadcaster broadcaster(config, "market_data");

    // 发送数据
    struct TickData {
        char code[16];
        double price;
        double volume;
    };

    TickData tick = {"SH600000", 100.5, 1000.0};

    broadcaster.broadcast(
        reinterpret_cast<const uint8_t*>(&tick),
        sizeof(tick),
        1,
        MarketDataType::Tick
    );

    // 获取统计
    auto stats = broadcaster.get_stats();
    std::cout << "Sent: " << stats.blocks_sent << " blocks" << std::endl;

    return 0;
}
```

### 2. 订阅数据

```cpp
#include "qaultra/ipc/broadcast_hub.hpp"

using namespace qaultra::ipc;

int main() {
    // 初始化
    DataSubscriber::initialize_runtime("my_subscriber");

    // 创建订阅器
    BroadcastConfig config;
    DataSubscriber subscriber(config, "market_data");

    // 接收数据
    while (true) {
        auto data = subscriber.receive_nowait();
        if (data) {
            std::cout << "Received " << data->size() << " bytes" << std::endl;

            // 解析数据
            const TickData* tick = reinterpret_cast<const TickData*>(data->data());
            std::cout << "Code: " << tick->code
                      << " Price: " << tick->price << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
```

## 运行测试

### 基础功能测试

```bash
# 运行基础测试
./build/broadcast_basic_test

# 预期输出:
# === Test 1: Basic Pub/Sub ===
# ✓ Test 1 PASSED
# === Test 2: Batch Broadcast ===
# ✓ Test 2 PASSED
# ...
# ===== ALL TESTS PASSED =====
```

### 大规模压力测试

```bash
# 设置系统限制
ulimit -n 65536
sudo sysctl -w kernel.shmmax=8589934592

# 运行大规模测试
./build/broadcast_massive_scale_test

# 预期输出:
# 📊 Test 1: 500 Subscribers + 1,000,000 Market Ticks
# ...
# Average throughput: 520000 ticks/sec
# ✓ Test 1 completed
```

## 性能配置

### 预设配置

```cpp
// 高性能配置
auto config = BroadcastConfig::high_performance();
// max_subscribers: 1500
// batch_size: 20000
// buffer_depth: 1000
// memory_pool_size_mb: 2048

// 低延迟配置
auto config = BroadcastConfig::low_latency();
// max_subscribers: 100
// batch_size: 1000
// buffer_depth: 100

// 大规模配置
auto config = BroadcastConfig::massive_scale();
// max_subscribers: 2000
// batch_size: 50000
// buffer_depth: 2000
// memory_pool_size_mb: 4096
```

### 自定义配置

```cpp
BroadcastConfig config;

// 订阅者管理
config.max_subscribers = 1000;

// 性能调优
config.batch_size = 10000;       // 批量大小
config.buffer_depth = 500;       // 队列深度
config.memory_pool_size_mb = 1024; // 内存池

// IceOryx 特定
config.service_name = "QAULTRA";
config.instance_name = "Broadcast";
config.queue_capacity = 1000;

// 验证配置
if (!config.validate()) {
    std::cerr << "Invalid configuration!" << std::endl;
}
```

## 统计监控

### 获取实时统计

```cpp
auto stats = broadcaster.get_stats();

std::cout << "Blocks sent: " << stats.blocks_sent << std::endl;
std::cout << "Records sent: " << stats.records_sent << std::endl;
std::cout << "Throughput: " << stats.throughput_records_per_sec() << " rec/s" << std::endl;
std::cout << "Throughput: " << stats.throughput_mb_per_sec() << " MB/s" << std::endl;
std::cout << "Success rate: " << stats.success_rate() << "%" << std::endl;
std::cout << "Avg latency: " << stats.avg_latency_ns / 1000.0 << " μs" << std::endl;
std::cout << "Max latency: " << stats.max_latency_ns / 1000.0 << " μs" << std::endl;
```

### 订阅者统计

```cpp
auto recv_stats = subscriber.get_receive_stats();

std::cout << "Blocks received: " << recv_stats.blocks_received << std::endl;
std::cout << "Records received: " << recv_stats.records_received << std::endl;
std::cout << "Bytes received: " << recv_stats.bytes_received << std::endl;
std::cout << "Missed samples: " << recv_stats.missed_samples << std::endl;
```

## 高级用法

### 多流管理

```cpp
BroadcastManager manager(config);

// 创建多个流
auto stock_broadcaster = manager.create_broadcaster("stock_data");
auto futures_broadcaster = manager.create_broadcaster("futures_data");
auto options_broadcaster = manager.create_broadcaster("options_data");

// 订阅特定流
auto stock_subscriber = manager.create_subscriber("stock_data");
auto futures_subscriber = manager.create_subscriber("futures_data");

// 获取所有统计
auto all_stats = manager.get_all_stats();
for (const auto& [name, stats] : all_stats) {
    std::cout << "Stream: " << name
              << " Throughput: " << stats.throughput_records_per_sec()
              << " rec/s" << std::endl;
}
```

### 零拷贝接收 (高级)

```cpp
// 接收指针，避免数据拷贝
auto block_ptr = subscriber.receive_block();
if (block_ptr) {
    const ZeroCopyMarketBlock* block = *block_ptr;

    // 直接访问共享内存
    const uint8_t* data = block->get_data();
    size_t count = block->record_count;

    // 处理数据...

    // 注意: block_ptr 超出作用域后，IceOryx 会自动释放
}
```

### 批量处理

```cpp
std::vector<TickData> ticks(10000);
// 填充数据...

size_t sent = broadcaster.broadcast_batch(
    reinterpret_cast<const uint8_t*>(ticks.data()),
    ticks.size() * sizeof(TickData),
    ticks.size(),
    MarketDataType::Tick
);

std::cout << "Sent " << sent << " / " << ticks.size() << " ticks" << std::endl;
```

## 性能调优

### 系统优化

```bash
# 1. 增加文件描述符限制
ulimit -n 65536

# 2. 增加共享内存限制
sudo sysctl -w kernel.shmmax=8589934592  # 8GB
sudo sysctl -w kernel.shmall=2097152

# 3. 设置 CPU 性能模式
sudo cpupower frequency-set -g performance

# 4. 禁用透明大页
echo never | sudo tee /sys/kernel/mm/transparent_hugepage/enabled
```

### 编译优化

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
      -DQAULTRA_USE_ICEORYX=ON \
      ..
```

## 故障排查

### 问题 1: IceOryx 未找到

**现象**:
```
CMake Warning: IceOryx not found. Zero-copy IPC features will be disabled.
```

**解决**:
```bash
# 检查 IceOryx 安装路径
ls /home/quantaxis/iceoryx/build/install/lib/cmake/

# 设置 CMAKE 变量
cmake -Diceoryx_DIR=/path/to/iceoryx/install/lib/cmake/iceoryx_posh ..
```

### 问题 2: 共享内存错误

**现象**:
```
Failed to create DataBroadcaster: shared memory allocation failed
```

**解决**:
```bash
# 检查共享内存限制
ipcs -l

# 增加限制
sudo sysctl -w kernel.shmmax=8589934592
```

### 问题 3: 订阅者无法接收

**现象**: 订阅者总是接收到 null

**检查**:
1. 确保 Publisher 和 Subscriber 使用相同的 stream_name
2. 确保 Publisher 调用了 `offer()`
3. 确保 Subscriber 调用了 `subscribe()`
4. 检查 IceOryx RouDi 守护进程是否运行

```bash
# 启动 RouDi (IceOryx 路由守护进程)
/home/quantaxis/iceoryx/build/install/bin/iox-roudi &
```

## 性能基准

### 单订阅者场景

| 指标 | 值 |
|------|-----|
| 吞吐量 | > 1M ticks/sec |
| 延迟 (P50) | ~3 μs |
| 延迟 (P99) | < 10 μs |
| CPU 使用 | < 20% |

### 500 订阅者场景

| 指标 | 值 |
|------|-----|
| 吞吐量 | > 500K ticks/sec |
| 每订阅者吞吐 | > 1K ticks/sec |
| 延迟 (P99) | < 10 μs |
| 内存使用 | < 1.5GB |

### 1000 订阅者场景

| 指标 | 值 |
|------|-----|
| 吞吐量 | > 300K ticks/sec |
| 每订阅者吞吐 | > 300 ticks/sec |
| 订阅者创建时间 | < 10s |

## 与 QARS Rust 版本对比

| 特性 | QARS Rust (iceoryx2) | QAULTRA C++ (iceoryx) |
|------|----------------------|------------------------|
| **底层库** | iceoryx2 (Rust) | iceoryx (C++) |
| **吞吐量** | 520K ticks/sec | 500K+ ticks/sec |
| **延迟** | P99 < 5 μs | P99 < 10 μs |
| **订阅者** | 1000+ | 1000+ |
| **成熟度** | ⭐⭐⭐⭐ 较新 | ⭐⭐⭐⭐⭐ 生产级 |
| **生态** | 新兴 | ROS 2, AUTOSAR |
| **API 对齐** | ✅ 100% API 兼容 | ✅ 完全对齐 |

## 最佳实践

1. **使用批量操作**: 批量发送可大幅提升吞吐量
2. **合理配置缓冲区**: `buffer_depth` 和 `batch_size` 需要根据场景调优
3. **监控统计信息**: 定期检查成功率和延迟指标
4. **启用零拷贝接收**: 对性能敏感场景使用 `receive_block()`
5. **系统调优**: 确保系统限制正确配置

## 进一步参考

- [IceOryx 官方文档](https://iceoryx.io/)
- [QARS Rust 版本文档](/home/quantaxis/qars2/docs/ICEORYX2_INTEGRATION.md)
- [性能测试指南](./BROADCAST_PERFORMANCE_CPP.md)
- [大规模测试指南](/home/quantaxis/qars2/docs/MASSIVE_SCALE_TESTING_GUIDE.md)

---

**创建时间**: 2025-10-01
**版本**: 1.0.0
**作者**: QAULTRA C++ Team
