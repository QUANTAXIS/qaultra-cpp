# QAULTRA C++ API 参考文档

**版本**: 1.0.0
**最后更新**: 2025-10-01

## 目录

- [Market 模块](#market-模块)
- [Account 模块](#account-模块)
- [Data 模块](#data-模块)
- [Protocol 模块](#protocol-模块)
- [IPC 模块](#ipc-模块)
- [Connector 模块](#connector-模块)

---

## Market 模块

### QAMarketSystem

**命名空间**: `qaultra::market`
**头文件**: `<qaultra/market/market_system.hpp>`

市场系统主类，对标 Rust `QAMarket`，负责账户管理、时间管理、订单调度和回测执行。

#### 构造函数

```cpp
// 默认构造
QAMarketSystem();

// 从路径创建
QAMarketSystem(const std::string& data_path,
               const std::string& portfolio_name = "");

// 使用已有 MarketCenter
explicit QAMarketSystem(std::shared_ptr<data::QAMarketCenter> market_center);
```

**参数**:
- `data_path`: 市场数据路径
- `portfolio_name`: 组合名称
- `market_center`: 已创建的市场数据中心

**示例**:
```cpp
using namespace qaultra;

// 方式1: 从路径创建
auto market1 = std::make_shared<market::QAMarketSystem>(
    "/data/stock",
    "my_portfolio"
);

// 方式2: 使用已有 MarketCenter
auto mc = std::make_shared<data::QAMarketCenter>("/data");
auto market2 = std::make_shared<market::QAMarketSystem>(mc);
```

---

#### 账户管理

##### register_account

```cpp
void register_account(const std::string& account_name,
                     double init_cash = 1000000.0);
```

注册新账户。

**参数**:
- `account_name`: 账户名称（唯一标识）
- `init_cash`: 初始资金（默认 100 万）

**异常**:
- `std::runtime_error`: 如果账户已存在

**示例**:
```cpp
market->register_account("acc_001", 1000000.0);  // 100万初始资金
market->register_account("acc_002", 5000000.0);  // 500万初始资金
```

---

##### get_account

```cpp
std::shared_ptr<account::QA_Account> get_account(const std::string& account_name);

std::shared_ptr<const account::QA_Account> get_account(
    const std::string& account_name) const;
```

获取账户实例。

**参数**:
- `account_name`: 账户名称

**返回值**:
- `std::shared_ptr<QA_Account>`: 账户智能指针

**异常**:
- `std::runtime_error`: 如果账户不存在

**示例**:
```cpp
auto account = market->get_account("acc_001");
account->buy("000001.XSHE", 100, 10.5);
```

---

##### get_account_names

```cpp
std::vector<std::string> get_account_names() const;
```

获取所有已注册账户名称列表。

**返回值**:
- `std::vector<std::string>`: 账户名称列表

**示例**:
```cpp
auto names = market->get_account_names();
for (const auto& name : names) {
    std::cout << "账户: " << name << std::endl;
}
```

---

#### 时间管理

##### set_date

```cpp
void set_date(const std::string& date);
```

设置当前交易日期。

**参数**:
- `date`: 日期字符串，格式 "YYYY-MM-DD" (例如 "2025-10-01")

**示例**:
```cpp
market->set_date("2025-10-01");
```

---

##### set_datetime

```cpp
void set_datetime(const std::string& datetime);
```

设置当前时间（分钟级）。

**参数**:
- `datetime`: 时间字符串，格式 "YYYY-MM-DD HH:MM:SS"

**示例**:
```cpp
market->set_datetime("2025-10-01 09:30:00");  // 开盘时间
```

---

##### get_date / get_datetime

```cpp
const std::string& get_date() const;
const std::string& get_datetime() const;
```

获取当前日期/时间。

**返回值**:
- `const std::string&`: 日期或时间字符串

**示例**:
```cpp
std::cout << "当前日期: " << market->get_date() << std::endl;
std::cout << "当前时间: " << market->get_datetime() << std::endl;
```

---

#### 市场数据

##### get_stock_day

```cpp
std::vector<data::StockCnDay> get_stock_day(
    const std::string& code,
    const std::string& start_date,
    const std::string& end_date) const;
```

获取股票日线数据。

**参数**:
- `code`: 股票代码 (例如 "000001.XSHE")
- `start_date`: 开始日期 "YYYY-MM-DD"
- `end_date`: 结束日期 "YYYY-MM-DD"

**返回值**:
- `std::vector<StockCnDay>`: 日线数据列表

**示例**:
```cpp
auto bars = market->get_stock_day(
    "000001.XSHE",
    "2024-01-01",
    "2024-12-31"
);

for (const auto& bar : bars) {
    std::cout << bar.date.to_string() << " 收盘: " << bar.close << std::endl;
}
```

---

##### get_stock_min

```cpp
std::vector<data::StockCn1Min> get_stock_min(
    const std::string& code,
    const std::string& start_datetime,
    const std::string& end_datetime) const;
```

获取股票分钟数据。

**参数**:
- `code`: 股票代码
- `start_datetime`: 开始时间 "YYYY-MM-DD HH:MM:SS"
- `end_datetime`: 结束时间 "YYYY-MM-DD HH:MM:SS"

**返回值**:
- `std::vector<StockCn1Min>`: 分钟数据列表

---

#### 订单调度

##### schedule_order

```cpp
void schedule_order(const std::string& account_name,
                   const MarketOrder& order,
                   const std::string& label = "");
```

添加订单到调度队列。

**参数**:
- `account_name`: 账户名称
- `order`: 市场订单对象
- `label`: 订单标签（可选）

**示例**:
```cpp
market::MarketOrder order;
order.code = "000001.XSHE";
order.amount = 100;
order.price = 10.5;
order.direction = "BUY";
order.offset = "OPEN";

market->schedule_order("acc_001", order, "开仓");
```

---

##### process_order_queue

```cpp
void process_order_queue();
```

处理所有待处理订单。

**示例**:
```cpp
market->process_order_queue();  // 执行所有队列中的订单
```

---

#### QIFI 快照

##### snapshot_all_accounts

```cpp
void snapshot_all_accounts();
```

保存所有账户的 QIFI 快照到缓存。

**示例**:
```cpp
market->snapshot_all_accounts();
```

---

##### get_account_snapshots

```cpp
const std::vector<protocol::qifi::QIFI>& get_account_snapshots(
    const std::string& account_name) const;
```

获取账户的 QIFI 快照历史。

**参数**:
- `account_name`: 账户名称

**返回值**:
- `const std::vector<QIFI>&`: QIFI 快照列表

**示例**:
```cpp
auto snapshots = market->get_account_snapshots("acc_001");
for (const auto& qifi : snapshots) {
    std::cout << "权益: " << qifi.account.balance << std::endl;
}
```

---

#### 回测执行

##### run_backtest

```cpp
void run_backtest(const std::string& start_date,
                 const std::string& end_date,
                 std::function<void(QAMarketSystem&)> strategy_func);
```

运行回测（占位接口，待实现完整逻辑）。

**参数**:
- `start_date`: 开始日期
- `end_date`: 结束日期
- `strategy_func`: 策略回调函数

**示例**:
```cpp
market->run_backtest("2024-01-01", "2024-12-31",
    [](market::QAMarketSystem& m) {
        auto account = m.get_account("acc_001");
        // 策略逻辑
        account->buy("000001.XSHE", 100, 10.0);
    }
);
```

---

## Account 模块

### QA_Account

**命名空间**: `qaultra::account`
**头文件**: `<qaultra/account/qa_account.hpp>`

统一账户类，支持股票和期货交易。

#### 构造函数

```cpp
QA_Account(const std::string& account_cookie,
           const std::string& portfolio_cookie,
           const std::string& user_cookie,
           double init_cash,
           bool auto_reload = false);
```

**参数**:
- `account_cookie`: 账户ID
- `portfolio_cookie`: 组合ID
- `user_cookie`: 用户ID
- `init_cash`: 初始资金
- `auto_reload`: 是否自动重载数据

---

#### 股票交易

##### buy

```cpp
void buy(const std::string& code, double amount, double price);
```

买入股票。

**参数**:
- `code`: 股票代码
- `amount`: 买入数量
- `price`: 买入价格

**示例**:
```cpp
account->buy("000001.XSHE", 100, 10.5);  // 买入100股，价格10.5
```

---

##### sell

```cpp
void sell(const std::string& code, double amount, double price);
```

卖出股票。

**参数**:
- `code`: 股票代码
- `amount`: 卖出数量
- `price`: 卖出价格

**示例**:
```cpp
account->sell("000001.XSHE", 100, 11.0);  // 卖出100股，价格11.0
```

---

#### 期货交易

##### buy_open

```cpp
void buy_open(const std::string& code, double amount, double price);
```

买入开仓（期货多头）。

---

##### sell_close

```cpp
void sell_close(const std::string& code, double amount, double price);
```

卖出平仓（平多头）。

---

##### sell_open

```cpp
void sell_open(const std::string& code, double amount, double price);
```

卖出开仓（期货空头）。

---

##### buy_close

```cpp
void buy_close(const std::string& code, double amount, double price);
```

买入平仓（平空头）。

---

##### buy_closetoday / sell_closetoday

```cpp
void buy_closetoday(const std::string& code, double amount, double price);
void sell_closetoday(const std::string& code, double amount, double price);
```

平今仓操作（仅期货）。

---

#### 查询接口

##### get_positions

```cpp
const std::unordered_map<std::string, QA_Position>& get_positions() const;
```

获取所有持仓。

**返回值**:
- `const std::unordered_map<std::string, QA_Position>&`: 持仓映射 (代码 → 持仓)

**示例**:
```cpp
auto positions = account->get_positions();
for (const auto& [code, pos] : positions) {
    std::cout << code << " 数量: " << pos.volume << std::endl;
}
```

---

##### get_qifi

```cpp
protocol::qifi::QIFI get_qifi() const;
```

获取账户的 QIFI 快照。

**返回值**:
- `QIFI`: 账户快照对象

**示例**:
```cpp
auto qifi = account->get_qifi();
std::cout << "账户权益: " << qifi.account.balance << std::endl;
std::cout << "可用资金: " << qifi.account.available << std::endl;
```

---

##### get_cash / get_available_cash

```cpp
double get_cash() const;
double get_available_cash() const;
```

获取当前资金/可用资金。

---

## Data 模块

### 数据类型

#### Date

**命名空间**: `qaultra::data`
**头文件**: `<qaultra/data/datatype.hpp>`

C++17 兼容的日期结构。

```cpp
struct Date {
    int year;
    int month;  // 1-12
    int day;    // 1-31

    Date();
    Date(int y, int m, int d);

    bool operator==(const Date& other) const;
    bool operator<(const Date& other) const;
    std::string to_string() const;  // 返回 "YYYY-MM-DD"
};
```

**示例**:
```cpp
data::Date date(2025, 10, 1);
std::cout << date.to_string() << std::endl;  // "2025-10-01"
```

---

#### StockCnDay

中国股票日线数据。

```cpp
struct StockCnDay {
    Date date;
    std::string order_book_id;
    float num_trades;
    float limit_up;
    float limit_down;
    float open;
    float high;
    float low;
    float close;
    float volume;
    float total_turnover;

    nlohmann::json to_json() const;
    static StockCnDay from_json(const nlohmann::json& j);
};
```

---

#### StockCn1Min

中国股票分钟数据。

```cpp
struct StockCn1Min {
    std::chrono::system_clock::time_point datetime;
    std::string order_book_id;
    float open;
    float high;
    float low;
    float close;
    float volume;
    float total_turnover;

    nlohmann::json to_json() const;
    static StockCn1Min from_json(const nlohmann::json& j);
};
```

---

#### Kline

通用 K 线数据。

```cpp
struct Kline {
    std::string order_book_id;
    double open;
    double close;
    double high;
    double low;
    double volume;
    double limit_up;
    double limit_down;
    double total_turnover;
    double split_coefficient_to;
    double dividend_cash_before_tax;

    // 辅助方法
    double get_change_percent() const;  // 涨跌幅
    double get_change_amount() const;   // 涨跌额
    bool is_limit_up() const;           // 是否涨停
    bool is_limit_down() const;         // 是否跌停
    double get_turnover_rate(double circulating_shares) const;  // 换手率
};
```

---

### 工具函数

#### utils 命名空间

```cpp
namespace qaultra::data::utils {

// 时间戳转换
std::string timestamp_to_string(const std::chrono::system_clock::time_point& tp);
std::chrono::system_clock::time_point string_to_timestamp(const std::string& str);

// 日期转换
std::string date_to_string(const Date& date);
Date string_to_date(const std::string& str);

// 交易日判断
bool is_trading_day(const Date& date);
Date next_trading_day(const Date& date);
Date prev_trading_day(const Date& date);
int trading_days_between(const Date& start, const Date& end);

}
```

---

## Protocol 模块

### QIFI 协议

**命名空间**: `qaultra::protocol::qifi`
**头文件**: `<qaultra/protocol/qifi.hpp>`

#### Account

```cpp
struct Account {
    std::string account_cookie;
    std::string portfolio_cookie;
    double balance;       // 账户权益
    double available;     // 可用资金
    double frozen;        // 冻结资金
    double market_value;  // 市值
};
```

---

#### QA_Position

```cpp
struct QA_Position {
    std::string instrument_id;
    double volume_long;           // 多头持仓
    double volume_short;          // 空头持仓
    double volume_long_today;     // 多头今仓
    double volume_short_today;    // 空头今仓
    double margin;                // 保证金
    double position_profit;       // 持仓盈亏
};
```

---

#### Order

```cpp
struct Order {
    std::string order_id;
    std::string instrument_id;
    double price;
    double volume;
    std::string direction;   // BUY/SELL
    std::string offset;      // OPEN/CLOSE/CLOSETODAY
    std::string status;      // PENDING/FILLED/CANCELLED
    std::string datetime;
};
```

---

#### QIFI

```cpp
struct QIFI {
    std::string account_cookie;
    Account account;
    std::unordered_map<std::string, QA_Position> positions;
    std::vector<Order> orders;
    std::vector<Trade> trades;

    nlohmann::json to_json() const;
    static QIFI from_json(const nlohmann::json& j);
};
```

---

## IPC 模块

### BroadcastHubV2

**命名空间**: `qaultra::ipc`
**头文件**: `<qaultra/ipc/broadcast_hub_v2.hpp>`

基于 iceoryx2 的零拷贝数据广播。

#### DataBroadcaster

```cpp
class DataBroadcaster {
public:
    DataBroadcaster(const BroadcastConfig& config);

    // 发布单条数据
    void broadcast(const std::string& stream_name,
                  const std::vector<uint8_t>& data,
                  size_t data_size,
                  MarketDataType data_type);

    // 批量发布
    void broadcast_batch(const std::string& stream_name,
                        const std::vector<uint8_t>& data,
                        size_t batch_size,
                        MarketDataType data_type);

    // 获取统计信息
    BroadcastStats get_stats() const;
};
```

---

#### DataSubscriber

```cpp
class DataSubscriber {
public:
    DataSubscriber(const BroadcastConfig& config,
                  const std::string& stream_name);

    // 非阻塞接收
    std::optional<std::vector<uint8_t>> receive_nowait();

    // 阻塞接收
    std::vector<uint8_t> receive();
};
```

---

## Connector 模块

### MongoDBConnector

**命名空间**: `qaultra::connector`
**头文件**: `<qaultra/connector/mongodb_connector.hpp>`

```cpp
class MongoDBConnector {
public:
    MongoDBConnector(const std::string& uri);

    // 保存 QIFI
    void save_qifi(const protocol::qifi::QIFI& qifi);

    // 查询 QIFI
    std::vector<protocol::qifi::QIFI> query_qifi(
        const std::string& account_cookie,
        const std::string& start_date,
        const std::string& end_date);
};
```

---

## 错误处理

### 异常类型

QAULTRA C++ 使用标准异常：

| 异常类型 | 使用场景 |
|---------|---------|
| `std::runtime_error` | 账户不存在、订单失败 |
| `std::invalid_argument` | 参数无效 |
| `std::out_of_range` | 索引越界 |

**示例**:
```cpp
try {
    auto account = market->get_account("non_exist");
} catch (const std::runtime_error& e) {
    std::cerr << "错误: " << e.what() << std::endl;
}
```

---

## 线程安全

| 类 | 线程安全性 | 说明 |
|----|----------|------|
| `QAMarketSystem` | 部分安全 | 读取操作安全，修改需外部同步 |
| `QA_Account` | 不安全 | 需外部同步 |
| `DataBroadcaster` | 完全安全 | 使用内部无锁结构 |
| `DataSubscriber` | 完全安全 | 可多线程订阅 |

---

## 性能提示

1. **预分配容器**:
```cpp
std::vector<data::StockCnDay> bars;
bars.reserve(10000);  // 预分配空间
```

2. **使用常量引用**:
```cpp
void process(const std::string& code);  // ✅ 避免拷贝
void process(std::string code);          // ❌ 会拷贝
```

3. **避免频繁创建账户**:
```cpp
// ✅ 复用账户实例
auto account = market->get_account("acc_001");
for (int i = 0; i < 1000; ++i) {
    account->buy(...);
}

// ❌ 每次都获取
for (int i = 0; i < 1000; ++i) {
    market->get_account("acc_001")->buy(...);
}
```

---

**维护者**: QUANTAXIS Team
**反馈**: [GitHub Issues](https://github.com/quantaxis/qaultra-cpp/issues)
