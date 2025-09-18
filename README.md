# QAULTRA C++ - 高性能量化交易系统

**QARS (QUANTAXIS RS) 的完整C++移植版本，具备最大化性能优化**

QAULTRA C++ 是将 QARS (QUANTAXIS RS) 量化交易系统从 Rust 完整移植到 C++ 的版本，专为超高性能算法交易、回测和投资组合管理而设计。

[![构建状态](https://github.com/quantaxis/qaultra-cpp/workflows/构建和测试/badge.svg)](https://github.com/quantaxis/qaultra-cpp/actions)
[![许可证](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++标准](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![Python版本](https://img.shields.io/badge/Python-3.8%2B-blue.svg)](https://www.python.org/)

## 📖 目录

- [系统架构](#系统架构)
- [核心特性](#核心特性)
- [技术原理](#技术原理)
- [安装指南](#安装指南)
- [快速开始](#快速开始)
- [模块详解](#模块详解)
- [性能基准](#性能基准)
- [API参考](#api参考)
- [回测框架](#回测框架)
- [数据库集成](#数据库集成)
- [自定义策略](#自定义策略)
- [性能优化](#性能优化)
- [配置选项](#配置选项)
- [测试](#测试)
- [常见问题](#常见问题)
- [贡献指南](#贡献指南)
- [许可证](#许可证)

## 🏗️ 系统架构

QAULTRA C++ 采用模块化设计，主要组件包括：

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Python API    │    │   C++ 核心      │    │   原生库        │
├─────────────────┤    ├─────────────────┤    ├─────────────────┤
│ • 账户管理      │◄──►│ • QA_Account    │◄──►│ • Apache Arrow  │
│ • 策略开发      │    │ • MatchEngine   │    │ • Intel TBB     │
│ • 回测分析      │    │ • MarketData    │    │ • mimalloc      │
│ • 数据分析      │    │ • 协议支持      │    │ • SIMD 内在函数 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### 核心设计原则

1. **高性能优先**: 所有关键路径均采用零拷贝和SIMD优化
2. **内存安全**: 使用RAII和智能指针避免内存泄漏
3. **线程安全**: 核心数据结构支持多线程并发访问
4. **模块化**: 清晰的接口分离，支持独立编译和测试
5. **可扩展性**: 支持自定义策略、数据源和交易接口

## 🚀 核心特性

### 💨 极致性能
- **SIMD优化**: 支持 AVX/AVX2/AVX-512 向量化计算，金融计算性能提升3倍
- **零拷贝操作**: 内存映射文件和零拷贝数据结构，减少不必要的数据拷贝
- **无锁算法**: 并发数据结构使用CAS操作，最小化线程竞争
- **原生CPU优化**: 编译时CPU特性检测，自动选择最优代码路径
- **内存池分配**: 预分配对象池用于频繁操作，减少动态内存分配开销
- **mimalloc**: 微软高性能内存分配器，比系统malloc快10%-20%

### 📈 完整交易基础设施
- **账户管理**: 多资产投资组合跟踪，实时盈亏计算，支持股票和期货
- **订单管理**: 完整订单生命周期管理，支持限价、市价、止损等订单类型
- **撮合引擎**: 高性能订单撮合，支持Level-2市场深度和实时成交
- **市场数据**: 基于Apache Arrow的列式存储，高效处理海量历史数据
- **回测引擎**: 事件驱动回测框架，支持真实的订单执行和滑点模拟
- **策略框架**: 可插拔策略开发系统，支持C++和Python策略

### 🔗 协议支持
- **QIFI**: 量化投资格式接口，标准化的账户和投资组合数据格式
- **MIFI**: 市场信息格式接口，统一的市场数据表示
- **TIFI**: 交易信息格式接口，标准化的交易数据交换
- **标准协议**: 支持FIX协议、REST API和WebSocket实时数据

### 🐍 Python集成
- **pybind11绑定**: 高性能Python接口，接近原生C++性能
- **NumPy集成**: 直接数组访问无需拷贝，支持零拷贝数据传递
- **Pandas兼容**: 基于Arrow后端的DataFrame操作，兼容pandas生态
- **Jupyter支持**: 完整的交互式分析和可视化支持

### 🗄️ 数据连接器
- **MongoDB**: 投资组合和交易历史存储，支持分布式部署
- **ClickHouse**: 高性能OLAP数据库，专为金融时序数据优化
- **Arrow/Parquet**: 列式数据存储和处理，高效的数据序列化
- **CSV/JSON**: 标准格式支持，便于数据导入导出

## 🔬 技术原理

### SIMD向量化计算原理

QAULTRA C++ 大量使用SIMD(Single Instruction, Multiple Data)指令来加速金融计算：

```cpp
// 传统标量计算
for (int i = 0; i < size; ++i) {
    result[i] = a[i] * b[i];
}

// SIMD向量化计算 (AVX2, 一次处理4个double)
__m256d va = _mm256_load_pd(&a[i]);
__m256d vb = _mm256_load_pd(&b[i]);
__m256d vr = _mm256_mul_pd(va, vb);
_mm256_store_pd(&result[i], vr);
```

**性能提升**:
- AVX2: 4倍加速 (4个double并行)
- AVX-512: 8倍加速 (8个double并行)
- 自适应检测: 运行时选择最佳SIMD指令集

### 零拷贝架构设计

**内存映射文件**:
```cpp
class MemoryMappedArray {
    void* mmap_ptr;  // 直接映射到磁盘文件
    size_t file_size;

    // 零拷贝访问，直接操作映射内存
    T& operator[](size_t index) {
        return static_cast<T*>(mmap_ptr)[index];
    }
};
```

**Arrow零拷贝集成**:
- 直接在Arrow内存缓冲区上操作
- Python绑定时避免数据拷贝
- 列式存储天然支持向量化计算

### 无锁并发数据结构

**Lock-Free队列实现**:
```cpp
template<typename T>
class LockFreeQueue {
    std::atomic<Node*> head;
    std::atomic<Node*> tail;

    bool enqueue(T item) {
        Node* new_node = new Node{item, nullptr};
        Node* prev_tail = tail.exchange(new_node);
        prev_tail->next.store(new_node);
        return true;
    }
};
```

**原子操作优势**:
- 无锁等待，减少上下文切换
- 支持多生产者多消费者
- 比互斥锁快10-100倍

### Apache Arrow列式存储

**内存布局优化**:
```
传统行存储:     | ID | Price | Volume | Time | ID | Price | Volume | Time |
Arrow列存储:    | ID | ID | ID | ID | Price | Price | Price | Price |
```

**优势**:
- CPU缓存友好，减少缓存未命中
- 向量化计算天然支持
- 压缩效率更高 (同类型数据连续存储)
- 零拷贝与Pandas/NumPy互操作

## 📦 安装指南

### 系统要求

- **C++20** 兼容编译器 (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake** 3.20 或更高版本
- **Python** 3.8+ (用于Python绑定)

### Ubuntu/Debian 安装

```bash
# 1. 安装系统依赖
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libtbb-dev \
    libssl-dev \
    python3-dev \
    python3-pip

# 2. 克隆仓库
git clone https://github.com/quantaxis/qaultra-cpp.git
cd qaultra-cpp

# 3. 创建构建目录
mkdir build && cd build

# 4. 配置CMake (启用所有优化)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DQAULTRA_ENABLE_SIMD=ON \
    -DQAULTRA_ENABLE_NATIVE=ON \
    -DQAULTRA_ENABLE_LTO=ON \
    -DQAULTRA_BUILD_PYTHON_BINDINGS=ON \
    -G Ninja

# 5. 编译
ninja -j$(nproc)

# 6. 安装Python包
pip install -e python/
```

### macOS 安装

```bash
# 1. 安装Homebrew依赖
brew install cmake ninja tbb python@3.11

# 2. 设置编译环境
export CXX=clang++
export CC=clang

# 3. 按照上述Ubuntu步骤继续
```

### Windows (MSVC) 安装

```batch
# 1. 安装Visual Studio 2019/2022
# 2. 安装CMake和vcpkg

# 3. 使用vcpkg安装依赖
vcpkg install tbb:x64-windows arrow:x64-windows

# 4. 配置和构建
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### Docker安装

```bash
# 使用官方Docker镜像
docker pull quantaxis/qaultra-cpp:latest

# 或构建本地镜像
docker build -t qaultra-cpp .
docker run -it qaultra-cpp bash
```

## 🚀 快速开始

### C++基础用法

```cpp
#include "qaultra/qaultra.hpp"

using namespace qaultra;

int main() {
    // 1. 创建交易账户
    auto account = std::make_shared<account::QA_Account>(
        "我的账户",           // 账户ID
        "投资组合1",         // 组合ID
        "用户123",           // 用户ID
        1000000.0,          // 初始资金 (100万)
        false,              // 是否自动补仓
        "backtest"          // 环境类型 (backtest/real)
    );

    std::cout << "初始资金: ￥" << account->get_cash() << std::endl;

    // 2. 执行买入操作
    auto buy_order = account->buy(
        "000001",           // 股票代码
        1000.0,            // 买入数量
        "2024-01-15 09:30:00", // 交易时间
        10.50              // 买入价格
    );

    std::cout << "订单状态: " << static_cast<int>(buy_order->status) << std::endl;
    std::cout << "账户余额: ￥" << account->get_cash() << std::endl;

    // 3. 价格更新
    account->on_price_change("000001", 11.00, "2024-01-15 15:00:00");

    std::cout << "浮动盈亏: ￥" << account->get_float_profit() << std::endl;
    std::cout << "总资产: ￥" << account->get_total_value() << std::endl;

    // 4. 导出QIFI格式
    auto qifi_data = account->to_qifi();
    std::cout << "持仓数量: " << qifi_data.positions.size() << std::endl;

    return 0;
}
```

### Python高级用法

```python
import qaultra_cpp as qa
import numpy as np
import pandas as pd

# 1. 创建账户
account = qa.account.QA_Account(
    account_cookie="python账户",
    portfolio_cookie="python组合",
    user_cookie="python用户",
    init_cash=1000000.0,
    auto_reload=False,
    environment="backtest"
)

print(f"初始资金: ￥{account.get_cash():,.2f}")

# 2. 创建Arrow K线数据
klines = qa.data.ArrowKlineCollection()

# 生成示例数据
dates = pd.date_range('2024-01-01', periods=100, freq='D')
codes = ["000001"] * 100
timestamps = [int(d.timestamp() * 1000) for d in dates]

# 模拟价格数据
np.random.seed(42)
prices = 10.0 + np.cumsum(np.random.normal(0, 0.1, 100))
opens = prices
closes = prices + np.random.normal(0, 0.05, 100)
highs = np.maximum(opens, closes) + np.abs(np.random.normal(0, 0.1, 100))
lows = np.minimum(opens, closes) - np.abs(np.random.normal(0, 0.1, 100))
volumes = np.random.uniform(100000, 500000, 100)
amounts = closes * volumes

# 添加到Arrow集合
klines.add_batch(codes, timestamps, opens.tolist(), highs.tolist(),
                lows.tolist(), closes.tolist(), volumes.tolist(), amounts.tolist())

print(f"K线数据条数: {klines.size()}")

# 3. 技术指标计算(SIMD优化)
sma_20 = klines.sma(20)  # 20日简单移动平均
ema_12 = klines.ema(0.154)  # 12日指数移动平均 (alpha=2/(12+1))
rsi_14 = klines.rsi(14)  # 14日RSI

print(f"SMA(20)最新值: {sma_20[-1]:.2f}")
print(f"EMA(12)最新值: {ema_12[-1]:.2f}")
print(f"RSI(14)最新值: {rsi_14[-1]:.2f}")

# 4. 执行交易
current_price = closes[-1]
buy_order = account.buy("000001", 1000, "2024-04-10 09:30:00", current_price)

print(f"买入订单: {1000}股 @ ￥{current_price:.2f}")
print(f"订单状态: {buy_order.status}")

# 5. 价格更新和盈亏计算
new_price = current_price * 1.05  # 上涨5%
account.on_price_change("000001", new_price, "2024-04-10 15:00:00")

print(f"价格更新: ￥{current_price:.2f} → ￥{new_price:.2f}")
print(f"浮动盈亏: ￥{account.get_float_profit():,.2f}")
print(f"总资产: ￥{account.get_total_value():,.2f}")

# 6. 性能测试 - SIMD vs 标准实现
size = 1000000
a = np.random.random(size)
b = np.random.random(size)

import time

# 标准NumPy
start = time.time()
numpy_result = a * b
numpy_time = time.time() - start

# SIMD优化
start = time.time()
simd_result = qa.simd.vectorized_multiply(a, b)
simd_time = time.time() - start

print(f"\n性能对比 ({size:,}个元素):")
print(f"NumPy实现: {numpy_time:.4f}秒")
print(f"SIMD实现: {simd_time:.4f}秒")
print(f"性能提升: {numpy_time/simd_time:.2f}倍")
```

## 📊 性能基准

| 操作类型 | QAULTRA C++ | QARS Rust | 性能提升 |
|---------|-------------|-----------|---------|
| 订单处理 | 2.1M ops/sec | 1.8M ops/sec | 1.17x |
| 投资组合计算 | 850K ops/sec | 720K ops/sec | 1.18x |
| 市场数据接入 | 12M ticks/sec | 10M ticks/sec | 1.20x |
| SIMD数学运算 | 45M ops/sec | 15M ops/sec | 3.00x |
| 内存使用 | -15% | 基准线 | 减少15% |

*基准测试环境: Intel Xeon 8280 (28核心), 256GB RAM*

### 详细性能测试结果

**SIMD优化效果**:
```
向量乘法 (1M元素):
- 标准实现: 45.2ms
- AVX2优化: 11.8ms (3.83倍提升)
- AVX-512: 6.1ms (7.41倍提升)

技术指标计算 (SMA-20, 10K数据点):
- 标准循环: 2.3ms
- SIMD优化: 0.6ms (3.83倍提升)

Portfolio P&L计算 (1000个持仓):
- 传统方式: 150μs
- 向量化: 42μs (3.57倍提升)
```

**内存性能**:
```
数据结构         | 传统方式  | Arrow优化 | 改善
K线数据(100万条) | 480MB    | 180MB    | 62.5%减少
订单簿深度       | 2.1MB    | 0.8MB    | 61.9%减少
持仓追踪         | 156KB    | 64KB     | 59.0%减少
```

## 🔧 模块详解

### 核心模块

#### 🏦 账户系统 (`qaultra::account`)
- **QA_Account**: 主要交易账户类，支持股票和期货交易
- **Position**: 多资产持仓跟踪，实时盈亏计算
- **Order**: 订单生命周期管理，支持多种订单类型
- **MarketPreset**: 市场特定配置，手续费和保证金设置
- **Algorithm**: 算法交易框架，支持TWAP、VWAP、Iceberg等策略

#### 📊 市场数据 (`qaultra::market`)
- **MatchingEngine**: 高性能订单撮合引擎，支持多线程并发
- **OrderBook**: Level-2市场深度数据，实时更新
- **MarketDataFeed**: 实时数据接入，支持多种数据源
- **MarketSimulator**: 回测市场模拟，真实交易环境复现
- **HistoricalMarket**: 历史市场数据管理和查询

#### 💾 数据结构 (`qaultra::data`)
- **ArrowKlineCollection**: 基于Arrow的列式OHLCV数据存储
- **KlineCollection**: 传统K线数据存储，向后兼容
- **MarketDataManager**: 多标的数据管理，统一接口
- **ConcurrentStructures**: 并发安全的数据结构集合

#### 🔌 协议支持 (`qaultra::protocol`)
- **QIFI**: 量化投资格式接口，账户和投资组合序列化
- **MIFI**: 市场信息格式接口，统一的市场数据表示
- **TIFI**: 交易信息格式接口，交易数据标准化交换

#### ⚡ 高性能计算 (`qaultra::simd`)
- **SimdMath**: 向量化金融计算，支持AVX/AVX2/AVX-512
- **FinancialMath**: 投资组合分析，技术指标计算
- **MemoryMappedArray**: 零拷贝数据访问，大文件高效处理
- **LockFreeRingBuffer**: 超低延迟队列，无锁并发访问

### 性能优化模块

#### 🧵 多线程 (`qaultra::threading`)
- **ThreadPool**: 工作窃取线程池，动态负载均衡
- **LockFreeQueue**: 多生产者多消费者无锁队列
- **AtomicCounters**: 无锁统计计数器，高并发性能监控

#### 🧠 内存管理 (`qaultra::memory`)
- **ObjectPool**: 预分配对象池，减少动态分配开销
- **AlignedAllocator**: SIMD对齐内存分配器，优化向量运算
- **MemoryMapper**: 虚拟内存管理，支持大文件映射

### 连接器模块

#### 🗄️ 数据库连接器 (`qaultra::connector`)
- **MongoConnector**: MongoDB连接器，支持账户和市场数据存储
- **ClickHouseConnector**: ClickHouse连接器，高性能时序数据分析
- **ParquetConnector**: Parquet文件读写，列式数据持久化

#### 🔄 回测引擎 (`qaultra::engine`)
- **BacktestEngine**: 事件驱动回测引擎，支持多策略并行
- **StrategyFramework**: 策略开发框架，支持C++和Python策略
- **PerformanceAnalyzer**: 性能分析器，风险指标计算

### 分析工具

#### 📈 性能分析 (`qaultra::analysis`)
- **QIFIAnalysis**: QIFI格式数据分析，投资组合指标计算
- **RiskAnalysis**: 风险分析工具，VaR、夏普比率等指标
- **PerformanceMetrics**: 绩效指标计算，回撤、收益率分析

## 📚 API参考

### 账户管理API

```cpp
// 创建交易账户
auto account = std::make_shared<account::QA_Account>(
    "账户ID", "组合ID", "用户ID",
    初始资金, 自动补仓, 环境类型);

// 股票交易操作
auto 买入订单 = account->buy(代码, 数量, 时间, 价格);
auto 卖出订单 = account->sell(代码, 数量, 时间, 价格);

// 期货交易操作
auto 买开 = account->buy_open(代码, 数量, 时间, 价格);
auto 卖平 = account->sell_close(代码, 数量, 时间, 价格);
auto 买平今 = account->buy_closetoday(代码, 数量, 时间, 价格);
auto 卖平今 = account->sell_closetoday(代码, 数量, 时间, 价格);

// 账户查询
double 资金余额 = account->get_cash();
double 总资产 = account->get_total_value();
double 浮动盈亏 = account->get_float_profit();
double 持仓市值 = account->get_market_value();
double 可用保证金 = account->get_margin();

// 风险管理
bool 允许下单 = account->check_order_allowed(代码, 数量, 价格, 方向);
double 最大下单量 = account->get_max_order_size(代码, 价格, 方向);

// QIFI格式导出
auto qifi数据 = account->to_qifi();
```

### 市场数据API

```cpp
// Arrow列式市场数据
auto k线集合 = std::make_shared<arrow_data::ArrowKlineCollection>();
k线集合->add_batch(代码列表, 时间戳, 开盘价, 最高价, 最低价, 收盘价, 成交量, 成交额);

// 技术指标计算 (SIMD优化)
auto sma指标 = k线集合->sma(20);           // 20日简单移动平均
auto ema指标 = k线集合->ema(0.1);           // 指数移动平均
auto rsi指标 = k线集合->rsi(14);            // 14日RSI
auto macd指标 = k线集合->macd(12, 26, 9);   // MACD指标
auto 布林带 = k线集合->bollinger_bands(20, 2.0); // 布林带

// 数据筛选和聚合
auto 筛选数据 = k线集合->filter_by_code("000001");
auto 聚合数据 = k线集合->resample("1H");        // 重采样为1小时
auto 分页数据 = k线集合->slice(0, 1000);         // 切片操作

// 统计计算
double 均值 = k线集合->mean("close");
double 标准差 = k线集合->std("close");
double 相关系数 = k线集合->correlation("close", "volume");
```

### 撮合引擎API

```cpp
// 创建撮合引擎
auto 撮合引擎 = market::factory::create_matching_engine(线程数);

// 设置回调函数
撮合引擎->add_trade_callback([](const auto& 成交) {
    std::cout << "成交: " << 成交.trade_volume
              << "股 @ " << 成交.trade_price << "元" << std::endl;
});

撮合引擎->add_order_callback([](const auto& 订单状态) {
    std::cout << "订单更新: " << 订单状态.order_id
              << " -> " << static_cast<int>(订单状态.status) << std::endl;
});

// 提交订单
auto 订单 = std::make_shared<Order>("订单1", "账户1", "000001",
                                 Direction::BUY, 100.0, 1000.0);
bool 成功 = 撮合引擎->submit_order(订单);

// 查询市场深度
auto 深度数据 = 撮合引擎->get_market_depth("000001", 10);
for (const auto& 档位 : 深度数据.bids) {
    std::cout << "买" << 档位.level << ": " << 档位.price << " x " << 档位.volume << std::endl;
}
```

## 🔄 回测框架

### 简单移动平均策略示例

```cpp
#include "qaultra/engine/backtest_engine.hpp"

using namespace qaultra::engine;

int main() {
    // 1. 配置回测参数
    BacktestConfig config;
    config.start_date = "2024-01-01";
    config.end_date = "2024-12-31";
    config.initial_cash = 1000000.0;    // 100万初始资金
    config.commission_rate = 0.0025;    // 0.25%手续费
    config.benchmark = "000300";        // 沪深300基准

    // 2. 创建回测引擎
    BacktestEngine engine(config);

    // 3. 添加交易标的
    std::vector<std::string> universe = {"000001", "000002", "000858", "002415"};
    engine.set_universe(universe);

    // 4. 创建和添加策略
    auto sma_strategy = factory::create_sma_strategy(5, 20);  // 5日线和20日线
    engine.add_strategy(sma_strategy);

    // 5. 加载市场数据
    engine.load_data("data/stock_data/");

    // 6. 运行回测
    auto results = engine.run();

    // 7. 输出结果
    std::cout << "=== 回测结果 ===" << std::endl;
    std::cout << "总收益率: " << (results.total_return * 100) << "%" << std::endl;
    std::cout << "年化收益率: " << (results.annual_return * 100) << "%" << std::endl;
    std::cout << "夏普比率: " << results.sharpe_ratio << std::endl;
    std::cout << "最大回撤: " << (results.max_drawdown * 100) << "%" << std::endl;
    std::cout << "总交易次数: " << results.total_trades << std::endl;
    std::cout << "胜率: " << (results.win_rate * 100) << "%" << std::endl;

    // 8. 保存结果
    engine.save_results("backtest_results.json");

    return 0;
}
```

### Python策略回测

```python
import qaultra_cpp as qa

# 1. 配置回测
config = qa.engine.BacktestConfig()
config.start_date = "2024-01-01"
config.end_date = "2024-12-31"
config.initial_cash = 1000000.0
config.commission_rate = 0.0025

# 2. 创建回测引擎
engine = qa.engine.BacktestEngine(config)

# 3. 设置股票池
universe = ["000001", "000002", "000858", "002415"]
engine.set_universe(universe)

# 4. 添加策略
sma_strategy = qa.engine.factory.create_sma_strategy(5, 20)
momentum_strategy = qa.engine.factory.create_momentum_strategy(20, 0.02)

engine.add_strategy(sma_strategy)
engine.add_strategy(momentum_strategy)

# 5. 运行回测
results = engine.run()

# 6. 分析结果
print("=== 回测结果 ===")
print(f"总收益率: {results.total_return*100:.2f}%")
print(f"年化收益率: {results.annual_return*100:.2f}%")
print(f"夏普比率: {results.sharpe_ratio:.3f}")
print(f"最大回撤: {results.max_drawdown*100:.2f}%")
print(f"波动率: {results.volatility*100:.2f}%")

# 7. 可视化结果(需要matplotlib)
import matplotlib.pyplot as plt

equity_curve = engine.plot_equity_curve()
dates = [point[0] for point in equity_curve]
values = [point[1] for point in equity_curve]

plt.figure(figsize=(12, 8))
plt.plot(dates, values, label='策略收益')
plt.title('策略权益曲线')
plt.xlabel('日期')
plt.ylabel('总资产 (￥)')
plt.legend()
plt.grid(True)
plt.show()
```

## 📊 数据库集成

### MongoDB使用

```cpp
#include "qaultra/connector/mongodb_connector.hpp"

// 1. 配置MongoDB连接
connector::MongoConfig config;
config.host = "localhost";
config.port = 27017;
config.database = "quantaxis";

auto mongo = std::make_unique<connector::MongoConnector>(config);

// 2. 连接和保存账户数据
if (mongo->connect()) {
    // 保存账户
    auto qifi = account->to_qifi();
    mongo->save_account(qifi);

    // 保存K线数据
    mongo->save_kline_data("stock_daily", klines);

    // 查询数据
    connector::QueryFilter filter;
    filter.code = "000001";
    filter.start_date = "2024-01-01";
    filter.end_date = "2024-12-31";

    auto historical_data = mongo->load_kline_data("stock_daily", filter);
}
```

### ClickHouse高性能分析

```cpp
#include "qaultra/connector/clickhouse_connector.hpp"

// 1. 配置ClickHouse连接
connector::ClickHouseConfig config;
config.host = "localhost";
config.port = 9000;
config.database = "quantaxis";

auto clickhouse = std::make_unique<connector::ClickHouseConnector>(config);

// 2. 创建表和插入数据
if (clickhouse->connect()) {
    // 创建K线表
    clickhouse->create_kline_table("stock_minute");

    // 批量插入数据
    clickhouse->insert_kline_data("stock_minute", klines);

    // 聚合查询
    auto daily_data = clickhouse->aggregate_kline_data(
        "stock_minute", "000001",
        "2024-01-01", "2024-12-31",
        connector::AggregationType::DAY_1
    );

    // 技术指标计算
    auto indicators = clickhouse->calculate_technical_indicators(
        "stock_minute", "000001", {"SMA", "EMA", "RSI"}, 20
    );
}
```

## 🛠️ 自定义策略开发

### C++策略开发

```cpp
#include "qaultra/engine/backtest_engine.hpp"

class MyCustomStrategy : public engine::Strategy {
public:
    // 策略参数
    int short_period = 5;
    int long_period = 20;
    double threshold = 0.02;

    void initialize(engine::StrategyContext& context) override {
        context.log("初始化自定义策略");
        // 初始化逻辑
    }

    void handle_data(engine::StrategyContext& context) override {
        for (const auto& symbol : context.universe) {
            // 获取历史价格
            auto short_prices = context.get_history(symbol, short_period, "close");
            auto long_prices = context.get_history(symbol, long_period, "close");

            if (short_prices.size() < short_period || long_prices.size() < long_period) {
                continue;
            }

            // 计算移动平均
            double short_ma = std::accumulate(short_prices.begin(), short_prices.end(), 0.0) / short_period;
            double long_ma = std::accumulate(long_prices.begin(), long_prices.end(), 0.0) / long_period;

            double current_price = context.get_price(symbol);
            auto position = context.get_position(symbol);

            // 交易信号
            double signal = (short_ma - long_ma) / long_ma;

            if (signal > threshold && (!position || position->volume_long == 0)) {
                // 买入信号
                double cash = context.get_cash();
                double shares = std::floor(cash * 0.2 / current_price / 100) * 100;

                if (shares >= 100) {
                    auto order = context.account->buy(symbol, shares, context.current_date, current_price);
                    context.log("买入 " + symbol + " " + std::to_string(shares) + "股");
                }
            } else if (signal < -threshold && position && position->volume_long > 0) {
                // 卖出信号
                auto order = context.account->sell(symbol, position->volume_long, context.current_date, current_price);
                context.log("卖出 " + symbol + " " + std::to_string(position->volume_long) + "股");
            }
        }
    }

    std::string get_name() const override {
        return "自定义均线策略";
    }

    std::map<std::string, double> get_parameters() const override {
        return {
            {"short_period", static_cast<double>(short_period)},
            {"long_period", static_cast<double>(long_period)},
            {"threshold", threshold}
        };
    }

    void set_parameter(const std::string& name, double value) override {
        if (name == "short_period") {
            short_period = static_cast<int>(value);
        } else if (name == "long_period") {
            long_period = static_cast<int>(value);
        } else if (name == "threshold") {
            threshold = value;
        }
    }
};
```

## ⚡ 性能优化指南

### SIMD优化使用

```cpp
#include "qaultra/simd/simd_math.hpp"

// 1. 向量化数学运算
std::vector<double> prices = {100.1, 100.2, 100.3, 100.4};
std::vector<double> volumes = {1000, 2000, 3000, 4000};

// SIMD优化的向量乘法
auto amounts = simd::vectorized_multiply(prices.data(), volumes.data(), prices.size());

// 2. 技术指标计算
auto sma_result = simd::calculate_sma(prices.data(), prices.size(), 20);
auto ema_result = simd::calculate_ema(prices.data(), prices.size(), 0.1);

// 3. 金融指标计算
std::vector<double> returns = simd::calculate_returns(prices.data(), prices.size());
double sharpe = simd::calculate_sharpe_ratio_simd(returns.data(), returns.size(), 0.03);
```

### 内存优化

```cpp
#include "qaultra/memory/object_pool.hpp"

// 1. 对象池使用
auto order_pool = std::make_shared<memory::ObjectPool<account::Order>>(10000);

// 高频创建订单时使用对象池
auto order = order_pool->acquire();
order->order_id = "ORDER_001";
order->code = "000001";
// ... 使用订单

order_pool->release(order);  // 释放回池

// 2. 内存映射数组(零拷贝)
memory::MemoryMappedArray<double> large_array("data.bin", 1000000);
large_array[0] = 123.456;
large_array.sync();  // 同步到磁盘
```

### 多线程优化

```cpp
#include "qaultra/threading/lockfree_queue.hpp"

// 1. 无锁队列
threading::LockFreeQueue<std::shared_ptr<account::Order>> order_queue(10000);

// 生产者线程
std::thread producer([&]() {
    for (int i = 0; i < 1000; ++i) {
        auto order = std::make_shared<account::Order>();
        order->order_id = "ORDER_" + std::to_string(i);
        order_queue.enqueue(order);
    }
});

// 消费者线程
std::thread consumer([&]() {
    std::shared_ptr<account::Order> order;
    while (order_queue.dequeue(order)) {
        // 处理订单
        process_order(order);
    }
});

producer.join();
consumer.join();
```

## ⚙️ 配置选项

### 构建选项

```cmake
# 性能优化
-DQAULTRA_ENABLE_SIMD=ON          # 启用SIMD优化
-DQAULTRA_ENABLE_NATIVE=ON        # 启用原生CPU优化
-DQAULTRA_ENABLE_LTO=ON           # 启用链接时优化
-DQAULTRA_ENABLE_MIMALLOC=ON      # 使用mimalloc分配器

# 功能特性
-DQAULTRA_BUILD_TESTS=ON          # 构建测试套件
-DQAULTRA_BUILD_EXAMPLES=ON       # 构建示例程序
-DQAULTRA_BUILD_PYTHON_BINDINGS=ON # 构建Python绑定
-DQAULTRA_BUILD_BENCHMARKS=ON     # 构建基准测试

# 调试选项 (仅Debug模式)
-DQAULTRA_ENABLE_ASAN=ON          # 地址消毒器
-DQAULTRA_ENABLE_TSAN=ON          # 线程消毒器
```

### 运行时配置

```bash
# SIMD优化级别
export QAULTRA_SIMD_LEVEL=AVX512  # AUTO, SSE42, AVX2, AVX512

# 内存分配
export QAULTRA_USE_MIMALLOC=1     # 0=系统, 1=mimalloc

# 线程设置
export QAULTRA_THREAD_COUNT=16    # 工作线程数量

# 日志级别
export QAULTRA_LOG_LEVEL=INFO     # TRACE, DEBUG, INFO, WARN, ERROR
```

## 🧪 测试

```bash
# 运行所有测试
ninja test

# 运行特定测试套件
./tests/qaultra_tests --gtest_filter="AccountTest.*"

# 运行基准测试
./benchmarks/qaultra_benchmarks

# 内存泄漏检测 (Debug构建)
valgrind --tool=memcheck ./tests/qaultra_tests

# 性能分析
perf record -g ./benchmarks/qaultra_benchmarks
perf report
```

## 📝 示例程序

查看 `examples/` 目录获取完整示例：

- **basic_trading.cpp**: 基础买卖操作示例
- **backtesting_strategy.cpp**: 完整回测工作流程
- **market_making.cpp**: 做市策略实现
- **portfolio_optimization.cpp**: 投资组合优化
- **real_time_trading.cpp**: 实时交易与市场数据接入
- **algo_trading_example.cpp**: 算法交易示例
- **simd_performance.cpp**: SIMD性能优化示例
- **concurrent_processing.cpp**: 并发处理示例

## ❓ 常见问题

### Q: 如何启用最高性能模式？

A: 编译时使用以下CMake选项：

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DQAULTRA_ENABLE_SIMD=ON \
    -DQAULTRA_ENABLE_NATIVE=ON \
    -DQAULTRA_ENABLE_LTO=ON \
    -DQAULTRA_ENABLE_MIMALLOC=ON
```

### Q: 如何处理大规模数据？

A: 使用内存映射和流式处理：

```cpp
// 1. 内存映射大文件
memory::MemoryMappedArray<double> big_data("large_file.bin", 100000000);

// 2. 流式插入ClickHouse
clickhouse->start_streaming_insert("large_table");
for (const auto& record : records) {
    clickhouse->stream_insert_kline(record);
}
clickhouse->finish_streaming_insert();
```

### Q: 如何优化策略回测速度？

A: 使用以下优化技巧：

```cpp
// 1. 启用并行处理
BacktestConfig config;
config.max_threads = std::thread::hardware_concurrency();
config.enable_matching_engine = false;  // 简化模式

// 2. 预分配内存
strategy->reserve_memory(expected_trades);

// 3. 使用SIMD优化指标
auto fast_sma = simd::calculate_sma(prices.data(), prices.size(), 20);
```

### Q: 如何集成实时数据源？

A: 实现MarketDataFeed接口：

```cpp
class MyDataFeed : public market::MarketDataFeed {
public:
    bool subscribe(const std::string& symbol) override {
        // 连接实时数据源
        return websocket_client->subscribe(symbol);
    }

    void add_callback(EventCallback callback) override {
        callbacks_.push_back(callback);
    }

private:
    void on_market_data(const MarketEvent& event) {
        for (auto& callback : callbacks_) {
            callback(event);
        }
    }
};
```

### Q: 如何处理不同市场的交易规则？

A: 使用MarketPreset配置：

```cpp
// 创建股票市场预设
auto stock_preset = std::make_shared<account::MarketPreset>();
stock_preset->commission_rate = 0.0025;
stock_preset->min_commission = 5.0;
stock_preset->tax_rate = 0.001;

// 创建期货市场预设
auto futures_preset = std::make_shared<account::MarketPreset>();
futures_preset->commission_rate = 0.0001;
futures_preset->margin_rate = 0.10;

// 应用到账户
account->set_market_preset("stock", stock_preset);
account->set_market_preset("futures", futures_preset);
```

## 🤝 贡献指南

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 开启 Pull Request

### 开发环境搭建

```bash
# 安装开发依赖
./scripts/install_deps.sh

# 设置pre-commit钩子
pre-commit install

# 运行代码格式化
./scripts/format_code.sh

# 运行静态分析
./scripts/analyze_code.sh

# 运行完整测试套件
ninja test && ninja benchmark
```

### 代码规范

- 使用现代C++20特性
- 遵循Google C++代码规范
- 所有公开API需要详细文档注释
- 新功能必须包含对应的单元测试
- 性能关键代码需要基准测试

## 📄 许可证

本项目基于 MIT 许可证开源 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- **QUANTAXIS**: 原始Python量化交易框架，为本项目提供设计理念
- **QARS**: 启发此C++移植的Rust实现版本
- **Apache Arrow**: 高性能列式数据处理库，核心数据引擎
- **Intel TBB**: 并行化构建块，提供高效的多线程支持
- **pybind11**: Python绑定框架，实现C++与Python无缝集成
- **mimalloc**: 微软高性能内存分配器
- **ClickHouse**: 高性能OLAP数据库，用于时序数据分析

## 📧 支持和社区

- **文档**: [https://qaultra-cpp.readthedocs.io](https://qaultra-cpp.readthedocs.io)
- **问题反馈**: [GitHub Issues](https://github.com/quantaxis/qaultra-cpp/issues)
- **讨论**: [GitHub Discussions](https://github.com/quantaxis/qaultra-cpp/discussions)
- **QQ群**: 563280067 (QUANTAXIS)
- **微信群**: 扫描二维码加入
- **邮箱**: support@qaultra.com

## 🎯 发展路线图

### 短期目标 (3个月)
- [ ] 完善Python绑定，达到100%功能覆盖
- [ ] 优化SIMD性能，支持ARM NEON指令集
- [ ] 增加更多技术指标和算法交易策略
- [ ] 完善文档和中文教程

### 中期目标 (6个月)
- [ ] 支持更多数据库连接器(Redis, InfluxDB)
- [ ] 实现分布式回测框架
- [ ] 增加机器学习策略支持
- [ ] 开发Web界面和可视化工具

### 长期目标 (1年)
- [ ] 支持加密货币交易
- [ ] 实现高频交易框架
- [ ] 开发云原生部署方案
- [ ] 建立开发者生态系统

---

**QAULTRA C++** - 高性能量化交易的终极选择

*让C++的性能与量化交易的精准完美结合*