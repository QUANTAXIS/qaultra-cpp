#pragma once

#include "../protocol/qifi.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>

namespace qaultra::account {

// Type definitions matching Rust implementation
using Price = double;
using Volume = double;
using Amount = double;
using Timestamp = std::chrono::system_clock::time_point;
using AssetId = std::string;

/// 订单状态枚举 - 完全匹配Rust实现
enum class OrderStatus : int {
    NEW = 0,                    // 新建订单
    PARTIAL_FILLED = 1,         // 部分成交
    FILLED = 2,                 // 全部成交
    REJECTED = 3,               // 被拒绝
    PENDING_CANCEL = 4,         // 待撤销
    CANCELLED = 5,              // 已撤销
    PENDING_NEW = 6,            // 待新建
    CALCULATED = 7,             // 已计算
    ACCEPTED = 8,               // 已接受
    SUSPENDED = 9,              // 暂停
    UNKNOWN = -1                // 未知状态
};

/// 交易方向枚举
enum class Direction : int {
    BUY = 1,                    // 买入
    SELL = -1                   // 卖出
};

/// 开平方向枚举 - 期货专用
enum class Offset : int {
    OPEN = 0,                   // 开仓
    CLOSE = 1,                  // 平仓
    CLOSETODAY = 2,             // 平今
    CLOSEYESTERDAY = 3          // 平昨
};

/// 订单类型枚举
enum class PriceType {
    LIMIT,                      // 限价单
    MARKET,                     // 市价单
    STOP,                       // 止损单
    STOP_LIMIT                  // 止损限价单
};

/**
 * @brief 交易订单 - 完全匹配Rust QIFI实现
 */
class Order {
public:
    // 基础字段 - 完全匹配Rust QIFI::Order
    std::string order_id;           // 订单编号
    std::string account_cookie;     // 账户编号
    std::string user_cookie;        // 用户编号
    std::string portfolio_cookie;   // 组合编号
    std::string instrument_id;      // 合约代码
    std::string secu_code;          // 证券代码
    std::string exchange_id;        // 交易所代码

    // 订单信息
    std::string direction = "BUY";              // 买卖方向 ("BUY"/"SELL")
    std::string offset = "OPEN";                // 开平方向 ("OPEN"/"CLOSE"/"CLOSETODAY")
    Volume volume_orign = 0.0;                  // 原始委托数量
    Price price_order = 0.0;                    // 委托价格
    std::string price_type = "LIMIT";           // 价格类型 ("LIMIT"/"MARKET")

    // 状态信息
    std::string status = "NEW";                 // 订单状态字符串
    Volume volume_left = 0.0;                   // 剩余数量
    Volume volume_fill = 0.0;                   // 成交数量
    Price price_fill = 0.0;                     // 成交价格
    Amount fee = 0.0;                           // 手续费
    Amount tax = 0.0;                           // 印花税

    // 时间字段
    std::string order_time;                     // 委托时间
    std::string cancel_time;                    // 撤单时间
    std::string trade_time;                     // 成交时间
    std::string last_update_time;               // 最后更新时间

    // 其他字段
    std::string reason;                         // 状态原因
    std::string error_message;                  // 错误信息
    int towards = 1;                            // 方向标识 (1:买入, -1:卖出)
    std::string exchange_order_id;              // 交易所订单号
    std::string market_type;                    // 市场类型

public:
    Order() = default;

    /// 构造函数 - 完全匹配Rust实现
    Order(const std::string& account,
          const std::string& code,
          int towards_val,
          const std::string& exchange,
          const std::string& order_time_str,
          double volume,
          double price,
          const std::string& order_id_str);

    /// 从QIFI格式创建订单
    static Order from_qifi(const protocol::qifi::Order& qifi_order);

    /// 转换为QIFI格式
    protocol::qifi::Order to_qifi() const;

    /// 订单操作方法
    void cancel();                              // 撤销订单
    void finish();                              // 完成订单
    void update(double filled_volume);          // 更新成交量
    void reject(const std::string& reason);     // 拒绝订单

    /// 状态查询方法
    bool is_finished() const;                   // 是否已完成
    bool is_active() const;                     // 是否活跃
    bool can_cancel() const;                    // 是否可撤销

    /// 计算方法
    double get_unfilled_volume() const;         // 获取未成交数量
    double get_filled_ratio() const;            // 获取成交比例
    double get_total_amount() const;            // 获取总金额
    double get_filled_amount() const;           // 获取成交金额

    /// 辅助方法
    OrderStatus get_status_enum() const;        // 获取状态枚举
    Direction get_direction_enum() const;       // 获取方向枚举
    std::string get_market_type() const;        // 获取市场类型

    /// 序列化方法
    nlohmann::json to_json() const;
    static Order from_json(const nlohmann::json& j);

    /// 生成唯一订单ID
    static std::string generate_order_id();

    /// 工具方法
    static std::string direction_to_string(Direction dir);
    static Direction string_to_direction(const std::string& dir);
    static std::string status_to_string(OrderStatus status);
    static OrderStatus string_to_status(const std::string& status);
    static int get_towards_from_direction(const std::string& direction, const std::string& offset);

private:
    void update_timestamp();
    void calculate_fee_and_tax();
};

/// 订单统计信息
struct OrderStats {
    size_t total_orders = 0;                    // 总订单数
    size_t active_orders = 0;                   // 活跃订单数
    size_t filled_orders = 0;                   // 已成交订单数
    size_t cancelled_orders = 0;                // 已撤销订单数
    size_t rejected_orders = 0;                 // 被拒绝订单数
    double total_volume = 0.0;                  // 总委托量
    double filled_volume = 0.0;                 // 总成交量
    double total_amount = 0.0;                  // 总金额
    double filled_amount = 0.0;                 // 总成交金额
    double total_fee = 0.0;                     // 总手续费
    double total_tax = 0.0;                     // 总印花税

    /// 计算统计数据
    void update(const Order& order);

    /// 重置统计数据
    void reset();

    /// 获取成交率
    double get_fill_rate() const;

    /// 获取成交金额占比
    double get_fill_amount_rate() const;

    /// 序列化
    nlohmann::json to_json() const;
};

/// 订单工厂函数 - 匹配Rust实现
namespace order_factory {
    /// 创建股票买入订单
    std::unique_ptr<Order> create_stock_buy_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);

    /// 创建股票卖出订单
    std::unique_ptr<Order> create_stock_sell_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);

    /// 创建期货买开订单
    std::unique_ptr<Order> create_future_buy_open_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);

    /// 创建期货卖开订单
    std::unique_ptr<Order> create_future_sell_open_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);

    /// 创建期货买平订单
    std::unique_ptr<Order> create_future_buy_close_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);

    /// 创建期货卖平订单
    std::unique_ptr<Order> create_future_sell_close_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);

    /// 创建期货买平今订单
    std::unique_ptr<Order> create_future_buy_closetoday_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);

    /// 创建期货卖平今订单
    std::unique_ptr<Order> create_future_sell_closetoday_order(
        const std::string& account_cookie,
        const std::string& code,
        double volume,
        double price,
        const std::string& order_time);
}

} // namespace qaultra::account