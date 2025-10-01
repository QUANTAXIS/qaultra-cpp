# QAULTRA C++ 使用示例

**版本**: 1.0.0
**最后更新**: 2025-10-01

## 目录

- [基础示例](#基础示例)
- [账户管理](#账户管理)
- [市场系统](#市场系统)
- [回测示例](#回测示例)
- [IPC 零拷贝通信](#ipc-零拷贝通信)
- [数据库集成](#数据库集成)
- [高级用法](#高级用法)

---

## 基础示例

### Hello QAULTRA

最简单的示例 - 创建账户并下单：

```cpp
#include <qaultra/market/market_system.hpp>
#include <qaultra/account/qa_account.hpp>
#include <iostream>

int main() {
    using namespace qaultra;

    // 1. 创建市场系统
    auto market = std::make_shared<market::QAMarketSystem>();

    // 2. 注册账户
    market->register_account("my_account", 1000000.0);

    // 3. 获取账户
    auto account = market->get_account("my_account");

    // 4. 买入股票
    account->buy("000001.XSHE", 100, 10.5);

    // 5. 查询持仓
    auto positions = account->get_positions();
    for (const auto& [code, pos] : positions) {
        std::cout << "持仓 " << code << ": " << pos.volume << " 股\n";
    }

    // 6. 获取 QIFI 快照
    auto qifi = account->get_qifi();
    std::cout << "账户权益: " << qifi.account.balance << "\n";
    std::cout << "可用资金: " << qifi.account.available << "\n";

    return 0;
}
```

**编译运行**:
```bash
g++ -std=c++17 hello_qaultra.cpp -o hello_qaultra \
    -I../include -L../build -lqaultra -lpthread
./hello_qaultra
```

---

## 账户管理

### 股票账户操作

```cpp
#include <qaultra/market/market_system.hpp>
#include <iostream>

void stock_trading_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("stock_account", 1000000.0);
    auto account = market->get_account("stock_account");

    // 买入股票
    account->buy("000001.XSHE", 100, 10.0);   // 买入100股平安银行
    account->buy("600000.XSHG", 200, 8.5);    // 买入200股浦发银行

    // 卖出股票
    account->sell("000001.XSHE", 50, 10.5);   // 卖出50股

    // 查看持仓
    std::cout << "=== 持仓明细 ===\n";
    for (const auto& [code, pos] : account->get_positions()) {
        std::cout << code << ":\n"
                  << "  数量: " << pos.volume << "\n"
                  << "  成本: " << pos.cost_price << "\n"
                  << "  盈亏: " << pos.profit << "\n";
    }

    // 查看资金
    std::cout << "\n=== 资金状况 ===\n";
    std::cout << "现金: " << account->get_cash() << "\n";
    std::cout << "可用: " << account->get_available_cash() << "\n";
}
```

---

### 期货账户操作

```cpp
#include <qaultra/market/market_system.hpp>
#include <iostream>

void futures_trading_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("futures_account", 500000.0);
    auto account = market->get_account("futures_account");

    // 买入开仓（做多）
    account->buy_open("IF2410", 1, 4000.0);   // 买入1手沪深300

    // 卖出开仓（做空）
    account->sell_open("IC2410", 2, 6000.0);  // 卖出2手中证500

    // 平仓操作
    account->sell_close("IF2410", 1, 4050.0); // 平多头
    account->buy_close("IC2410", 1, 5950.0);  // 平空头

    // 平今仓
    account->sell_closetoday("IF2410", 1, 4100.0);

    // 查看持仓
    std::cout << "=== 期货持仓 ===\n";
    for (const auto& [code, pos] : account->get_positions()) {
        std::cout << code << ":\n"
                  << "  多头: " << pos.volume_long << "\n"
                  << "  空头: " << pos.volume_short << "\n"
                  << "  保证金: " << pos.margin << "\n"
                  << "  持仓盈亏: " << pos.position_profit << "\n";
    }
}
```

---

### 多账户管理

```cpp
void multi_account_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>(
        "/data/market", "portfolio_2025"
    );

    // 注册多个账户
    market->register_account("stock_account", 1000000.0);
    market->register_account("futures_account", 500000.0);
    market->register_account("options_account", 300000.0);

    // 获取所有账户名称
    auto account_names = market->get_account_names();
    std::cout << "管理 " << account_names.size() << " 个账户:\n";
    for (const auto& name : account_names) {
        auto account = market->get_account(name);
        std::cout << "  - " << name
                  << " (权益: " << account->get_cash() << ")\n";
    }

    // 分别操作不同账户
    auto stock_acc = market->get_account("stock_account");
    stock_acc->buy("000001.XSHE", 100, 10.0);

    auto futures_acc = market->get_account("futures_account");
    futures_acc->buy_open("IF2410", 1, 4000.0);
}
```

---

## 市场系统

### 时间管理

```cpp
void time_management_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("acc_001", 1000000.0);

    // 设置交易日期
    market->set_date("2025-01-02");
    std::cout << "当前日期: " << market->get_date() << "\n";

    // 模拟日内交易
    std::vector<std::string> time_points = {
        "2025-01-02 09:30:00",  // 开盘
        "2025-01-02 10:00:00",
        "2025-01-02 11:30:00",  // 午盘
        "2025-01-02 13:00:00",
        "2025-01-02 15:00:00"   // 收盘
    };

    auto account = market->get_account("acc_001");

    for (const auto& time : time_points) {
        market->set_datetime(time);
        std::cout << "\n时间: " << market->get_datetime() << "\n";

        // 根据时间执行不同策略
        if (time.find("09:30") != std::string::npos) {
            account->buy("000001.XSHE", 100, 10.0);
            std::cout << "  → 开盘买入\n";
        } else if (time.find("15:00") != std::string::npos) {
            account->sell("000001.XSHE", 100, 10.5);
            std::cout << "  → 收盘卖出\n";
        }
    }
}
```

---

### 订单调度

```cpp
void order_scheduling_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("acc_001", 1000000.0);

    // 创建订单
    market::MarketOrder order1;
    order1.code = "000001.XSHE";
    order1.amount = 100;
    order1.price = 10.0;
    order1.direction = "BUY";
    order1.offset = "OPEN";

    market::MarketOrder order2;
    order2.code = "600000.XSHG";
    order2.amount = 200;
    order2.price = 8.5;
    order2.direction = "BUY";
    order2.offset = "OPEN";

    // 添加到调度队列
    market->schedule_order("acc_001", order1, "开仓订单1");
    market->schedule_order("acc_001", order2, "开仓订单2");

    std::cout << "订单已加入队列\n";

    // 批量处理订单
    std::cout << "开始处理订单...\n";
    market->process_order_queue();
    std::cout << "订单处理完成\n";

    // 查看结果
    auto account = market->get_account("acc_001");
    std::cout << "持仓数量: " << account->get_positions().size() << "\n";
}
```

---

### QIFI 快照管理

```cpp
void qifi_snapshot_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("acc_001", 1000000.0);
    auto account = market->get_account("acc_001");

    // 模拟交易过程
    account->buy("000001.XSHE", 100, 10.0);
    market->snapshot_all_accounts();  // 快照 1

    account->buy("600000.XSHG", 200, 8.5);
    market->snapshot_all_accounts();  // 快照 2

    account->sell("000001.XSHE", 50, 10.5);
    market->snapshot_all_accounts();  // 快照 3

    // 获取快照历史
    auto snapshots = market->get_account_snapshots("acc_001");
    std::cout << "共 " << snapshots.size() << " 个快照:\n";

    for (size_t i = 0; i < snapshots.size(); ++i) {
        const auto& qifi = snapshots[i];
        std::cout << "\n快照 " << (i + 1) << ":\n"
                  << "  权益: " << qifi.account.balance << "\n"
                  << "  可用: " << qifi.account.available << "\n"
                  << "  持仓数: " << qifi.positions.size() << "\n"
                  << "  订单数: " << qifi.orders.size() << "\n";
    }
}
```

---

## 回测示例

### 简单均线策略回测

```cpp
#include <qaultra/market/market_system.hpp>
#include <vector>
#include <numeric>

// 计算简单移动平均
double sma(const std::vector<double>& prices, size_t period) {
    if (prices.size() < period) return 0.0;

    double sum = std::accumulate(
        prices.end() - period, prices.end(), 0.0
    );
    return sum / period;
}

void sma_strategy_backtest() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>(
        "/data/stock", "sma_portfolio"
    );
    market->register_account("strategy_account", 1000000.0);

    // 获取历史数据
    auto bars = market->get_stock_day(
        "000001.XSHE",
        "2024-01-01",
        "2024-12-31"
    );

    std::cout << "回测数据: " << bars.size() << " 天\n";

    std::vector<double> close_prices;
    auto account = market->get_account("strategy_account");

    // 遍历每一天
    for (size_t i = 0; i < bars.size(); ++i) {
        const auto& bar = bars[i];
        close_prices.push_back(bar.close);

        market->set_date(bar.date.to_string());

        if (i < 20) continue;  // 需要至少20天数据

        // 计算 MA5 和 MA20
        double ma5 = sma(close_prices, 5);
        double ma20 = sma(close_prices, 20);

        // 金叉买入，死叉卖出
        auto positions = account->get_positions();
        bool has_position = positions.find("000001.XSHE") != positions.end();

        if (ma5 > ma20 && !has_position) {
            // 金叉买入
            account->buy("000001.XSHE", 100, bar.close);
            std::cout << bar.date.to_string() << " 买入: " << bar.close << "\n";
        } else if (ma5 < ma20 && has_position) {
            // 死叉卖出
            account->sell("000001.XSHE", 100, bar.close);
            std::cout << bar.date.to_string() << " 卖出: " << bar.close << "\n";
        }

        // 保存快照
        market->snapshot_all_accounts();
    }

    // 输出回测结果
    auto qifi = account->get_qifi();
    std::cout << "\n=== 回测结果 ===\n";
    std::cout << "初始资金: 1000000.0\n";
    std::cout << "最终权益: " << qifi.account.balance << "\n";
    std::cout << "收益: " << (qifi.account.balance - 1000000.0) << "\n";
    std::cout << "收益率: "
              << ((qifi.account.balance - 1000000.0) / 1000000.0 * 100)
              << "%\n";
}
```

---

### 使用回测框架

```cpp
void backtest_framework_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>(
        "/data/stock", "backtest_2025"
    );
    market->register_account("bt_account", 1000000.0);

    // 定义策略函数
    auto my_strategy = [](market::QAMarketSystem& m) {
        auto account = m.get_account("bt_account");
        auto date = m.get_date();

        // 简单策略：每5天买入一次
        static int day_count = 0;
        if (++day_count % 5 == 0) {
            account->buy("000001.XSHE", 100, 10.0);
            std::cout << date << " 买入\n";
        }
    };

    // 运行回测
    market->run_backtest(
        "2024-01-01",
        "2024-12-31",
        my_strategy
    );

    // 查看结果
    auto snapshots = market->get_account_snapshots("bt_account");
    std::cout << "生成 " << snapshots.size() << " 个快照\n";
}
```

---

## IPC 零拷贝通信

### 数据发布者 (Publisher)

```cpp
#include <qaultra/ipc/broadcast_hub_v2.hpp>
#include <iostream>
#include <vector>

void ipc_publisher_example() {
    using namespace qaultra::ipc;

    // 配置
    BroadcastConfig config;
    config.max_subscribers = 100;
    config.batch_size = 1000;
    config.zero_copy_enabled = true;

    // 创建广播器
    DataBroadcaster broadcaster(config);

    // 模拟市场数据
    std::vector<uint8_t> tick_data(1000);
    for (size_t i = 0; i < tick_data.size(); ++i) {
        tick_data[i] = static_cast<uint8_t>(i % 256);
    }

    // 发送数据
    std::cout << "开始发送数据...\n";
    for (int i = 0; i < 1000; ++i) {
        broadcaster.broadcast(
            "market_stream",
            tick_data,
            tick_data.size(),
            MarketDataType::Tick
        );

        if (i % 100 == 0) {
            std::cout << "已发送 " << i << " 条数据\n";
        }
    }

    // 获取统计信息
    auto stats = broadcaster.get_stats();
    std::cout << "\n=== 发送统计 ===\n";
    std::cout << "总消息数: " << stats.total_messages << "\n";
    std::cout << "成功率: " << stats.success_rate << "%\n";
    std::cout << "吞吐量: " << stats.throughput_per_sec << " msg/sec\n";
}
```

---

### 数据订阅者 (Subscriber)

```cpp
#include <qaultra/ipc/broadcast_hub_v2.hpp>
#include <iostream>

void ipc_subscriber_example() {
    using namespace qaultra::ipc;

    BroadcastConfig config;
    config.max_subscribers = 100;

    // 创建订阅者
    DataSubscriber subscriber(config, "market_stream");

    std::cout << "开始接收数据...\n";
    int count = 0;

    while (count < 1000) {
        // 非阻塞接收
        auto data = subscriber.receive_nowait();

        if (data.has_value()) {
            count++;
            std::cout << "收到数据 " << count
                      << ", 大小: " << data->size() << "\n";
        }
    }

    std::cout << "接收完成，共 " << count << " 条数据\n";
}
```

---

### 跨语言通信 (C++ ↔ Rust)

**C++ 发布者**:
```cpp
#include <qaultra/ipc/cross_lang_data.hpp>

void cross_lang_publisher() {
    using namespace qaultra::ipc;

    CrossLangDataPublisher publisher("cross_lang_stream");

    // 发送跨语言数据
    CrossLangMarketData data;
    data.code = "000001.XSHE";
    data.price = 10.5;
    data.volume = 1000;
    data.timestamp = std::time(nullptr);

    publisher.publish(data);
    std::cout << "已发送跨语言数据\n";
}
```

**Rust 订阅者** (在 Rust 代码中):
```rust
use qaultra::ipc::CrossLangDataSubscriber;

fn cross_lang_subscriber() {
    let subscriber = CrossLangDataSubscriber::new("cross_lang_stream");

    while let Some(data) = subscriber.receive() {
        println!("收到数据: {} @ {} x {}",
                 data.code, data.price, data.volume);
    }
}
```

---

## 数据库集成

### MongoDB 保存 QIFI

```cpp
#include <qaultra/connector/mongodb_connector.hpp>
#include <qaultra/market/market_system.hpp>

void mongodb_save_example() {
    using namespace qaultra;

    // 创建 MongoDB 连接
    connector::MongoDBConnector db("mongodb://localhost:27017");

    // 创建市场系统和账户
    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("acc_001", 1000000.0);
    auto account = market->get_account("acc_001");

    // 交易操作
    account->buy("000001.XSHE", 100, 10.0);
    account->buy("600000.XSHG", 200, 8.5);

    // 获取 QIFI 并保存到 MongoDB
    auto qifi = account->get_qifi();
    db.save_qifi(qifi);

    std::cout << "QIFI 已保存到 MongoDB\n";
}
```

---

### MongoDB 查询 QIFI

```cpp
void mongodb_query_example() {
    using namespace qaultra;

    connector::MongoDBConnector db("mongodb://localhost:27017");

    // 查询指定日期范围的 QIFI
    auto qifis = db.query_qifi(
        "acc_001",
        "2025-01-01",
        "2025-01-31"
    );

    std::cout << "查询到 " << qifis.size() << " 条 QIFI 记录\n";

    for (const auto& qifi : qifis) {
        std::cout << "账户: " << qifi.account_cookie
                  << ", 权益: " << qifi.account.balance << "\n";
    }
}
```

---

## 高级用法

### 自定义市场数据源

```cpp
#include <qaultra/data/marketcenter.hpp>

class MyCustomMarketCenter : public qaultra::data::QAMarketCenter {
public:
    std::vector<qaultra::data::StockCnDay> get_stock_day(
        const std::string& code,
        const std::string& start,
        const std::string& end) override
    {
        std::vector<qaultra::data::StockCnDay> result;

        // 自定义数据加载逻辑
        // 例如：从 CSV 文件、Redis、或 API 加载
        result = load_from_custom_source(code, start, end);

        return result;
    }

private:
    std::vector<qaultra::data::StockCnDay> load_from_custom_source(
        const std::string& code,
        const std::string& start,
        const std::string& end)
    {
        // 实现自定义加载逻辑
        // ...
        return {};
    }
};

void custom_data_source_example() {
    auto custom_mc = std::make_shared<MyCustomMarketCenter>();
    auto market = std::make_shared<qaultra::market::QAMarketSystem>(custom_mc);

    // 使用自定义数据源
    auto bars = market->get_stock_day("000001.XSHE", "2024-01-01", "2024-12-31");
}
```

---

### 批量操作

```cpp
#include <qaultra/account/batch_operations.hpp>

void batch_operations_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("acc_001", 1000000.0);
    market->register_account("acc_002", 1000000.0);

    // 创建批量操作处理器
    account::BatchOrderProcessor processor;

    // 准备账户列表
    std::vector<std::shared_ptr<account::QA_Account>> accounts = {
        market->get_account("acc_001"),
        market->get_account("acc_002")
    };

    // 准备订单列表
    std::vector<account::Order> orders;
    // ... 填充订单

    // 批量下单
    size_t success_count = processor.batch_place_orders(accounts, orders);
    std::cout << "成功下单: " << success_count << "/" << orders.size() << "\n";
}
```

---

### 性能分析

```cpp
#include <qaultra/analysis/performance_analyzer.hpp>
#include <chrono>

void performance_analysis_example() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("acc_001", 1000000.0);
    auto account = market->get_account("acc_001");

    // 模拟交易
    account->buy("000001.XSHE", 100, 10.0);
    account->sell("000001.XSHE", 100, 10.5);

    // 性能分析
    analysis::PerformanceAnalyzer analyzer;
    auto qifi = account->get_qifi();

    auto metrics = analyzer.analyze(qifi);

    std::cout << "=== 性能指标 ===\n";
    std::cout << "总收益: " << metrics.total_return << "\n";
    std::cout << "夏普比率: " << metrics.sharpe_ratio << "\n";
    std::cout << "最大回撤: " << metrics.max_drawdown << "\n";
    std::cout << "胜率: " << metrics.win_rate << "%\n";
}
```

---

## 完整策略示例

### 动量策略回测

```cpp
#include <qaultra/market/market_system.hpp>
#include <algorithm>
#include <cmath>

class MomentumStrategy {
private:
    qaultra::market::QAMarketSystem& market_;
    std::string account_name_;
    int lookback_period_;

public:
    MomentumStrategy(qaultra::market::QAMarketSystem& market,
                    const std::string& account_name,
                    int lookback_period = 20)
        : market_(market)
        , account_name_(account_name)
        , lookback_period_(lookback_period) {}

    double calculate_momentum(const std::vector<double>& prices) {
        if (prices.size() < 2) return 0.0;
        return (prices.back() - prices.front()) / prices.front();
    }

    void on_bar(const qaultra::data::StockCnDay& bar,
                const std::vector<double>& price_history) {
        if (price_history.size() < lookback_period_) return;

        auto account = market_.get_account(account_name_);
        auto positions = account->get_positions();
        bool has_position = positions.find(bar.order_book_id) != positions.end();

        // 计算动量
        std::vector<double> recent_prices(
            price_history.end() - lookback_period_,
            price_history.end()
        );
        double momentum = calculate_momentum(recent_prices);

        // 交易逻辑
        if (momentum > 0.05 && !has_position) {
            // 正动量且无持仓，买入
            account->buy(bar.order_book_id, 100, bar.close);
            std::cout << bar.date.to_string() << " 买入 (动量=" << momentum << ")\n";
        } else if (momentum < -0.05 && has_position) {
            // 负动量且有持仓，卖出
            account->sell(bar.order_book_id, 100, bar.close);
            std::cout << bar.date.to_string() << " 卖出 (动量=" << momentum << ")\n";
        }
    }
};

void momentum_strategy_backtest() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>(
        "/data/stock", "momentum_portfolio"
    );
    market->register_account("strategy_acc", 1000000.0);

    MomentumStrategy strategy(*market, "strategy_acc", 20);

    // 获取数据
    auto bars = market->get_stock_day("000001.XSHE", "2024-01-01", "2024-12-31");

    std::vector<double> price_history;

    // 回测循环
    for (const auto& bar : bars) {
        price_history.push_back(bar.close);
        market->set_date(bar.date.to_string());
        strategy.on_bar(bar, price_history);
        market->snapshot_all_accounts();
    }

    // 结果分析
    auto account = market->get_account("strategy_acc");
    auto qifi = account->get_qifi();

    std::cout << "\n=== 回测结果 ===\n";
    std::cout << "最终权益: " << qifi.account.balance << "\n";
    std::cout << "收益率: "
              << ((qifi.account.balance - 1000000.0) / 1000000.0 * 100) << "%\n";
}
```

---

## 编译所有示例

创建 `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.16)
project(qaultra_examples)

set(CMAKE_CXX_STANDARD 17)

find_package(qaultra REQUIRED)

# Hello QAULTRA
add_executable(hello_qaultra hello_qaultra.cpp)
target_link_libraries(hello_qaultra qaultra::qaultra)

# 股票交易
add_executable(stock_trading stock_trading.cpp)
target_link_libraries(stock_trading qaultra::qaultra)

# 回测示例
add_executable(backtest_example backtest.cpp)
target_link_libraries(backtest_example qaultra::qaultra)

# IPC 示例
add_executable(ipc_pub_sub ipc_example.cpp)
target_link_libraries(ipc_pub_sub qaultra::qaultra)
```

**编译运行**:
```bash
mkdir build && cd build
cmake ..
make
./hello_qaultra
```

---

**更多示例**: 查看 `examples/` 目录
**API 文档**: [API_REFERENCE.md](API_REFERENCE.md)
**问题反馈**: [GitHub Issues](https://github.com/quantaxis/qaultra-cpp/issues)
