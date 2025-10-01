# QAULTRA C++ - 高性能量化交易系统

**基于 Rust QARS 的 C++ 实现，专注极致性能与跨语言互操作**

QAULTRA C++ 是 QARS (QUANTAXIS RS) 量化交易系统的 C++ 实现，与 Rust 核心保持架构对齐，专为超高性能算法交易、大规模回测和实时投资组合管理而设计。

[![构建状态](https://github.com/quantaxis/qaultra-cpp/workflows/构建和测试/badge.svg)](https://github.com/quantaxis/qaultra-cpp/actions)
[![许可证](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++标准](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)

## 📖 目录

- [核心特性](#核心特性)
- [系统架构](#系统架构)
- [快速开始](#快速开始)
- [模块详解](#模块详解)
- [性能基准](#性能基准)
- [文档](#文档)
- [许可证](#许可证)

## 🚀 核心特性

### 💨 零拷贝 IPC 架构

- **iceoryx/iceoryx2 双栈支持**: 高性能进程间通信，零拷贝数据传输
- **跨语言互操作**: C++ ↔ Rust ↔ Python 无缝数据交换
- **大规模并行回测**: 支持 1000+ 并发订阅者，吞吐量 > 500K msg/sec
- **微秒级延迟**: P99 延迟 < 10 μs，适合高频交易场景

**性能指标** (Massive Scale Testing):
- 吞吐量: 520K ticks/sec (500 订阅者)
- 延迟: P99 < 10 μs
- 成功率: > 99.9%
- 内存使用: < 2GB (100万行情数据)

### 📈 完整交易基础设施

#### 市场系统 (Market System)
- **QAMarketSystem**: 完全匹配 Rust `QAMarket` 的 C++ 实现
- **账户管理**: 多账户注册、资金管理、QIFI 协议支持
- **时间管理**: 交易日历、分钟级时间控制
- **订单调度**: 订单队列、目标持仓队列、批量处理
- **回测执行**: 事件驱动回测、策略回调、QIFI 快照

#### 账户系统 (Account System)
- **QA_Account**: 股票/期货统一账户
- **持仓管理**: 实时持仓跟踪、盈亏计算
- **订单管理**: 完整订单生命周期、风控检查
- **批量操作**: 并行批量下单、撤单、查询

#### 数据类型 (Data Types)
- **Rust 对齐**: 完全匹配 Rust 数据结构定义
- **StockCnDay**: 中国股票日线数据
- **StockCn1Min**: 中国股票分钟数据
- **FutureCn1Min/FutureCnDay**: 中国期货数据
- **Kline**: 通用 K 线数据结构

### 🔗 协议支持

- **QIFI**: 量化投资格式接口 - 标准化账户/持仓/订单格式
- **MIFI**: 市场信息格式接口 - 统一市场数据表示
- **TIFI**: 交易信息格式接口 - 标准化交易数据

### 🗄️ 数据库连接器

- **MongoDB**: 账户数据、历史快照存储 (可选)
- **ClickHouse**: 高性能时序数据存储 (规划中)
- **Apache Arrow**: 列式数据处理、零拷贝传输

## 🏗️ 系统架构

### 模块结构

```
qaultra-cpp/
├── include/qaultra/          # 头文件
│   ├── account/              # 账户系统
│   │   ├── qa_account.hpp    # 统一账户 (Stock + Futures)
│   │   ├── position.hpp      # 持仓管理
│   │   ├── order.hpp         # 订单管理
│   │   └── batch_operations.hpp  # 批量操作
│   ├── market/               # 市场系统
│   │   ├── market_system.hpp # 市场系统主类 (对标 Rust QAMarket)
│   │   ├── simmarket.hpp     # 模拟市场
│   │   └── match_engine.hpp  # 撮合引擎
│   ├── data/                 # 数据类型
│   │   ├── datatype.hpp      # Rust 匹配的基础数据类型
│   │   ├── kline.hpp         # K线数据
│   │   └── marketcenter.hpp  # 市场数据中心
│   ├── protocol/             # 协议定义
│   │   ├── qifi.hpp          # QIFI 协议
│   │   ├── mifi.hpp          # MIFI 协议
│   │   └── tifi.hpp          # TIFI 协议
│   ├── ipc/                  # IPC 模块
│   │   ├── broadcast_hub_v1.hpp  # IceOryx v1 广播
│   │   ├── broadcast_hub_v2.hpp  # iceoryx2 广播
│   │   └── cross_lang_data.hpp   # 跨语言数据结构
│   ├── connector/            # 数据库连接器
│   │   ├── database_connector.hpp
│   │   └── mongodb_connector.hpp
│   └── analysis/             # 性能分析
│       └── performance_analyzer.hpp
├── src/                      # 实现文件
├── tests/                    # 测试
├── examples/                 # 示例代码
└── docs/                     # 文档
```

### 设计原则

1. **Rust 为核心**: C++ 实现完全对标 Rust 版本架构
2. **零冗余**: 避免创建简化版或重复功能
3. **高性能**: 零拷贝、SIMD 优化、无锁并发
4. **C++17 兼容**: 使用广泛支持的标准，避免 C++20 依赖
5. **模块化**: 清晰的接口分离，支持可选编译

### 架构对比：C++ vs Rust

| 组件 | Rust (qars2/src) | C++ (qaultra-cpp) | 状态 |
|------|------------------|-------------------|------|
| 账户系统 | `qaaccount::QA_Account` | `account::QA_Account` | ✅ 完全对齐 |
| 市场系统 | `qamarket::QAMarket` | `market::QAMarketSystem` | ✅ 完全对齐 |
| 数据类型 | `qadata::StockCnDay` | `data::StockCnDay` | ✅ 完全对齐 |
| IPC 广播 | `qadata::DataBroadcaster` | `ipc::BroadcastHubV2` | ✅ iceoryx2 集成 |
| 协议 | `qaprotocol::qifi::QIFI` | `protocol::qifi::QIFI` | ✅ 完全对齐 |

## 📦 快速开始

### 系统要求

- **编译器**: GCC 9+ / Clang 10+ / MSVC 2019+
- **CMake**: 3.16+
- **依赖库**:
  - nlohmann_json
  - Google Test (可选，用于测试)
  - MongoDB C++ Driver (可选，`QAULTRA_USE_MONGODB=ON`)
  - Apache Arrow (可选，`QAULTRA_USE_ARROW=ON`)
  - IceOryx (可选，`QAULTRA_USE_ICEORYX=ON`)
  - iceoryx2 (可选，`QAULTRA_USE_ICEORYX2=ON`)

### 编译安装

```bash
# 克隆项目
git clone https://github.com/quantaxis/qaultra-cpp.git
cd qaultra-cpp

# 创建构建目录
mkdir build && cd build

# 配置项目 (基础版本)
cmake .. -DQAULTRA_BUILD_TESTS=ON

# 配置项目 (完整功能)
cmake .. \
  -DQAULTRA_BUILD_TESTS=ON \
  -DQAULTRA_USE_MONGODB=ON \
  -DQAULTRA_USE_ICEORYX2=ON \
  -DQAULTRA_USE_FULL_FEATURES=ON

# 编译
make -j$(nproc)

# 运行测试
./progressive_test
./protocol_test
./unified_account_test
```

### 快速示例

```cpp
#include <qaultra/market/market_system.hpp>
#include <qaultra/account/qa_account.hpp>

int main() {
    using namespace qaultra;

    // 创建市场系统
    auto market = std::make_shared<market::QAMarketSystem>(
        "/data/market",     // 数据路径
        "my_portfolio"      // 组合名称
    );

    // 注册账户
    market->register_account("account_001", 1000000.0);  // 100万初始资金

    // 获取账户
    auto account = market->get_account("account_001");

    // 下单
    account->buy("000001.XSHE", 100, 10.5);

    // 查询持仓
    auto positions = account->get_positions();
    for (const auto& [code, pos] : positions) {
        std::cout << "持仓: " << code << " 数量: " << pos.volume << std::endl;
    }

    // 获取 QIFI 快照
    auto qifi = account->get_qifi();
    std::cout << "账户权益: " << qifi.balance << std::endl;

    return 0;
}
```

## 📚 模块详解

### Account 模块 (`include/qaultra/account/`)

**核心类**:
- `QA_Account`: 统一账户，支持股票和期货交易
- `QA_Position`: 持仓管理，实时盈亏计算
- `Order`: 订单管理，完整生命周期跟踪
- `BatchOrderProcessor`: 批量操作处理器

**主要功能**:
- 多账户管理
- 买卖/开平仓操作
- 实时风控检查
- 盈亏计算
- QIFI 协议导出

### Market 模块 (`include/qaultra/market/`)

**核心类**:
- `QAMarketSystem`: 市场系统主类（对标 Rust `QAMarket`）
- `SimMarket`: 模拟市场
- `MatchEngine`: 订单撮合引擎

**主要功能**:
- 账户注册和管理
- 时间管理（交易日期/时间）
- 订单调度和队列
- 目标持仓管理
- 回测执行
- QIFI 快照管理

### Data 模块 (`include/qaultra/data/`)

**核心类型**:
- `Date`: C++17 兼容的日期结构
- `StockCnDay`: 中国股票日线数据
- `StockCn1Min`: 中国股票分钟数据
- `FutureCn1Min`: 中国期货分钟数据
- `FutureCnDay`: 中国期货日线数据
- `Kline`: 通用 K 线结构

**工具函数**:
- 时间戳/日期转换
- 交易日判断
- 下一个/上一个交易日计算

### IPC 模块 (`include/qaultra/ipc/`)

**核心类**:
- `BroadcastHubV1`: 基于 IceOryx (v1) 的数据广播
- `BroadcastHubV2`: 基于 iceoryx2 的数据广播
- `CrossLangData`: 跨语言数据结构

**关键特性**:
- 零拷贝共享内存传输
- 支持 1000+ 并发订阅者
- 微秒级延迟
- 批量数据传输优化

### Protocol 模块 (`include/qaultra/protocol/`)

**QIFI 协议** (`qifi.hpp`):
- 标准化账户数据格式
- 持仓、订单、成交数据结构
- JSON 序列化/反序列化

**MIFI 协议** (`mifi.hpp`):
- 市场数据标准格式
- 行情快照、逐笔成交

**TIFI 协议** (`tifi.hpp`):
- 交易数据交换格式

## 🔧 编译选项

### CMake 编译选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `QAULTRA_BUILD_TESTS` | ON | 构建测试程序 |
| `QAULTRA_BUILD_EXAMPLES` | OFF | 构建示例程序 |
| `QAULTRA_USE_ARROW` | OFF | 启用 Apache Arrow 支持 |
| `QAULTRA_USE_MONGODB` | OFF | 启用 MongoDB 连接器 |
| `QAULTRA_USE_ICEORYX` | ON | 启用 IceOryx (v1) IPC |
| `QAULTRA_USE_ICEORYX2` | ON | 启用 iceoryx2 IPC |
| `QAULTRA_USE_FULL_FEATURES` | OFF | 启用所有完整功能 |

### 构建配置

```bash
# 最小构建（仅核心功能）
cmake .. -DQAULTRA_BUILD_TESTS=OFF

# 完整构建（所有功能）
cmake .. \
  -DQAULTRA_BUILD_TESTS=ON \
  -DQAULTRA_BUILD_EXAMPLES=ON \
  -DQAULTRA_USE_MONGODB=ON \
  -DQAULTRA_USE_ARROW=ON \
  -DQAULTRA_USE_ICEORYX2=ON \
  -DQAULTRA_USE_FULL_FEATURES=ON

# 高性能构建（优化编译）
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

## 📊 性能基准

### IPC 性能 (iceoryx2)

| 测试场景 | 订阅者数 | 吞吐量 | 延迟 (P99) | 成功率 |
|---------|---------|--------|-----------|--------|
| 大规模并发 | 500 | 520K msg/sec | < 10 μs | 100% |
| 高并发 | 1000 | 350K msg/sec | < 15 μs | 99.9% |
| 持续发送 | 10 | 1.2M msg/sec | < 5 μs | 100% |
| 长时稳定性 | 50 | 800K msg/sec | < 8 μs | 99.99% |

### 账户操作性能

| 操作 | 延迟 | 吞吐量 |
|-----|------|--------|
| 下单 | < 1 μs | > 1M ops/sec |
| 持仓查询 | < 100 ns | > 10M ops/sec |
| 盈亏计算 | < 500 ns | > 2M ops/sec |
| QIFI 快照 | < 10 μs | > 100K ops/sec |

### 数据处理性能

| 操作 | 性能 |
|-----|------|
| K线数据解析 | > 5M rows/sec |
| 时间序列聚合 | > 1M rows/sec |
| 跨语言数据传输 | > 2GB/sec (零拷贝) |

## 📖 文档

- [架构文档](docs/ARCHITECTURE.md) - 详细架构设计
- [API 参考](docs/API_REFERENCE.md) - 完整 API 文档
- [编译指南](docs/BUILD_GUIDE.md) - 详细编译说明
- [示例代码](docs/EXAMPLES.md) - 使用示例
- [变更日志](CHANGELOG.md) - 版本变更记录
- [贡献指南](CONTRIBUTING.md) - 如何贡献代码

### IPC 专题文档

- [IPC 集成指南](docs/ICEORYX_INTEGRATION_CPP.md)
- [跨语言 IPC 状态](docs/CROSS_LANGUAGE_IPC_STATUS.md)
- [双栈 IPC 架构](docs/DUAL_STACK_IPC.md)
- [C++ vs Rust IPC 对比](docs/CPP_RUST_IPC_COMPARISON.md)

## 🧪 测试

### 运行测试

```bash
cd build

# 基础测试
./progressive_test           # 渐进式测试
./protocol_test              # 协议测试
./unified_account_test       # 统一账户测试

# 性能测试
./performance_analysis_test  # 性能分析测试

# 批量操作测试 (需要 GTest)
./batch_operations_test

# IPC 测试 (需要 iceoryx2)
./broadcast_basic_test       # 基础广播测试
./broadcast_massive_scale_test  # 大规模压力测试
```

### 测试覆盖

- ✅ 账户基础操作
- ✅ 持仓和订单管理
- ✅ QIFI/MIFI/TIFI 协议
- ✅ 批量操作并发安全
- ✅ 市场系统集成
- ✅ IPC 零拷贝传输
- ✅ 大规模压力测试

## 🔄 最近更新 (2025-10-01)

### 架构重构
- ✅ 删除 `unified_backtest_engine`，使用 `market_system` 替代
- ✅ 创建 `QAMarketSystem` 完全对齐 Rust `QAMarket`
- ✅ 数据类型清理，删除冗余 `datatype_simple.hpp`
- ✅ C++17 兼容性改进，使用自定义 `Date` 结构

### IPC 增强
- ✅ iceoryx2 集成，零拷贝数据广播
- ✅ 大规模压力测试 (500+ 订阅者，1M+ ticks)
- ✅ 跨语言数据交换 (C++ ↔ Rust ↔ Python)

### 性能优化
- ✅ 零拷贝共享内存传输
- ✅ 批量数据处理优化
- ✅ 并发无锁数据结构

详见 [CHANGELOG.md](CHANGELOG.md)

## 🤝 贡献

欢迎贡献代码、报告 Bug 或提出新功能建议！

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

详见 [贡献指南](CONTRIBUTING.md)

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 🙏 致谢

- **QUANTAXIS 团队**: 原始 Python/Rust 实现
- **IceOryx/iceoryx2 团队**: 高性能 IPC 中间件
- **Apache Arrow 团队**: 列式数据处理框架
- **nlohmann/json**: C++ JSON 库
- **Google Test**: C++ 测试框架

## 📮 联系方式

- **问题反馈**: [GitHub Issues](https://github.com/quantaxis/qaultra-cpp/issues)
- **讨论**: [GitHub Discussions](https://github.com/quantaxis/qaultra-cpp/discussions)
- **邮件**: quantaxis@qq.com

---

**QAULTRA C++ - 基于 Rust，为性能而生 🚀**
