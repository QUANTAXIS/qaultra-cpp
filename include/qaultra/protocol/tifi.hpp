#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace qaultra::protocol::tifi {

/// TIFI (Trading Information Format Interface) implementation
/// 标准化交易信息格式接口

/**
 * @brief 订单方向枚举
 */
enum class Direction {
    BUY,            // 买入
    SELL,           // 卖出
    UNKNOWN         // 未知
};

/**
 * @brief 开平标志枚举
 */
enum class Offset {
    OPEN,           // 开仓
    CLOSE,          // 平仓
    CLOSE_TODAY,    // 平今
    CLOSE_YESTERDAY,// 平昨
    FORCE_CLOSE,    // 强平
    UNKNOWN         // 未知
};

/**
 * @brief 订单状态枚举
 */
enum class OrderStatus {
    PENDING,        // 待成交
    PARTIAL_FILLED, // 部分成交
    FILLED,         // 全部成交
    CANCELLED,      // 已撤销
    REJECTED,       // 已拒绝
    EXPIRED,        // 已过期
    UNKNOWN         // 未知
};

/**
 * @brief 价格类型枚举
 */
enum class PriceType {
    LIMIT,          // 限价
    MARKET,         // 市价
    STOP,           // 止损
    STOP_LIMIT,     // 止损限价
    FAK,            // 立即成交剩余撤销
    FOK,            // 全部成交或撤销
    UNKNOWN         // 未知
};

/**
 * @brief 时间有效条件枚举
 */
enum class TimeCondition {
    IOC,            // 立即成交或撤销
    GTC,            // 撤销前有效
    GTD,            // 指定日期前有效
    DAY,            // 当日有效
    FAK,            // 立即成交剩余撤销
    FOK,            // 全部成交或撤销
    UNKNOWN         // 未知
};

/**
 * @brief 标准化订单结构 - TIFI格式
 */
struct Order {
    // 基本信息
    std::string order_id;               // 订单ID
    std::string account_id;             // 账户ID
    std::string user_id;                // 用户ID
    std::string strategy_id;            // 策略ID

    // 合约信息
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码
    std::string product_id;             // 品种代码

    // 交易信息
    Direction direction;                // 买卖方向
    Offset offset;                      // 开平标志
    double volume = 0.0;                // 委托数量
    double price = 0.0;                 // 委托价格
    PriceType price_type;               // 价格类型
    TimeCondition time_condition;       // 时间条件

    // 状态信息
    OrderStatus status;                 // 订单状态
    double volume_traded = 0.0;         // 成交数量
    double volume_left = 0.0;           // 剩余数量
    double avg_price = 0.0;             // 成交均价

    // 时间信息
    std::string insert_time;            // 委托时间
    std::string update_time;            // 更新时间
    std::string cancel_time;            // 撤销时间

    // 错误信息
    std::string error_code;             // 错误代码
    std::string error_message;          // 错误信息

    // 交易所信息
    std::string exchange_order_id;      // 交易所订单号
    std::string parent_order_id;        // 父订单号（算法交易）

    // 费用信息
    double commission = 0.0;            // 手续费
    double tax = 0.0;                   // 印花税

    /**
     * @brief 是否已完成（成交或取消）
     */
    bool is_finished() const {
        return status == OrderStatus::FILLED ||
               status == OrderStatus::CANCELLED ||
               status == OrderStatus::REJECTED ||
               status == OrderStatus::EXPIRED;
    }

    /**
     * @brief 获取成交比例
     */
    double get_fill_ratio() const {
        if (volume == 0.0) return 0.0;
        return volume_traded / volume;
    }

    nlohmann::json to_json() const;
    static Order from_json(const nlohmann::json& j);
};

/**
 * @brief 标准化成交结构 - TIFI格式
 */
struct Trade {
    // 基本信息
    std::string trade_id;               // 成交ID
    std::string order_id;               // 订单ID
    std::string account_id;             // 账户ID
    std::string user_id;                // 用户ID

    // 合约信息
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码

    // 成交信息
    Direction direction;                // 买卖方向
    Offset offset;                      // 开平标志
    double volume = 0.0;                // 成交数量
    double price = 0.0;                 // 成交价格

    // 时间信息
    std::string trade_time;             // 成交时间

    // 费用信息
    double commission = 0.0;            // 手续费
    double tax = 0.0;                   // 印花税

    // 交易所信息
    std::string exchange_trade_id;      // 交易所成交号

    nlohmann::json to_json() const;
    static Trade from_json(const nlohmann::json& j);
};

/**
 * @brief 持仓信息结构 - TIFI格式
 */
struct Position {
    // 基本信息
    std::string account_id;             // 账户ID
    std::string user_id;                // 用户ID

    // 合约信息
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码
    std::string product_id;             // 品种代码

    // 持仓数量
    double long_position = 0.0;         // 多头持仓
    double short_position = 0.0;        // 空头持仓
    double long_position_today = 0.0;   // 多头今仓
    double short_position_today = 0.0;  // 空头今仓
    double long_position_yesterday = 0.0; // 多头昨仓
    double short_position_yesterday = 0.0; // 空头昨仓

    // 成本和价格
    double long_avg_price = 0.0;        // 多头持仓均价
    double short_avg_price = 0.0;       // 空头持仓均价
    double last_price = 0.0;            // 最新价
    double pre_settle_price = 0.0;      // 前结算价
    double settle_price = 0.0;          // 结算价

    // 盈亏信息
    double position_pnl = 0.0;          // 持仓盈亏
    double close_pnl = 0.0;             // 平仓盈亏
    double realized_pnl = 0.0;          // 已实现盈亏
    double unrealized_pnl = 0.0;        // 未实现盈亏

    // 保证金信息
    double margin = 0.0;                // 占用保证金
    double margin_ratio = 0.0;          // 保证金率

    // 时间信息
    std::string update_time;            // 更新时间

    /**
     * @brief 获取净持仓
     */
    double get_net_position() const {
        return long_position - short_position;
    }

    /**
     * @brief 是否有持仓
     */
    bool has_position() const {
        return long_position > 0.0 || short_position > 0.0;
    }

    /**
     * @brief 计算持仓市值
     */
    double get_market_value() const {
        return std::abs(get_net_position()) * last_price;
    }

    nlohmann::json to_json() const;
    static Position from_json(const nlohmann::json& j);
};

/**
 * @brief 账户资金信息结构 - TIFI格式
 */
struct Account {
    // 基本信息
    std::string account_id;             // 账户ID
    std::string user_id;                // 用户ID
    std::string account_name;           // 账户名称
    std::string account_type;           // 账户类型

    // 资金信息
    double total_asset = 0.0;           // 总资产
    double available_cash = 0.0;        // 可用资金
    double frozen_cash = 0.0;           // 冻结资金
    double margin = 0.0;                // 占用保证金
    double position_value = 0.0;        // 持仓市值

    // 盈亏信息
    double realized_pnl = 0.0;          // 已实现盈亏
    double unrealized_pnl = 0.0;        // 未实现盈亏
    double total_pnl = 0.0;             // 总盈亏
    double close_pnl = 0.0;             // 平仓盈亏

    // 风险信息
    double risk_ratio = 0.0;            // 风险度
    double margin_ratio = 0.0;          // 保证金比例

    // 手续费信息
    double commission = 0.0;            // 手续费
    double tax = 0.0;                   // 印花税

    // 时间信息
    std::string trading_day;            // 交易日
    std::string update_time;            // 更新时间

    /**
     * @brief 计算净资产
     */
    double get_net_asset() const {
        return total_asset + unrealized_pnl;
    }

    /**
     * @brief 计算资金使用率
     */
    double get_cash_usage_ratio() const {
        if (total_asset == 0.0) return 0.0;
        return (total_asset - available_cash) / total_asset;
    }

    nlohmann::json to_json() const;
    static Account from_json(const nlohmann::json& j);
};

/**
 * @brief 风险指标结构 - TIFI格式
 */
struct RiskMetrics {
    std::string account_id;             // 账户ID
    std::string datetime;               // 计算时间

    // 收益指标
    double total_return = 0.0;          // 总收益率
    double annual_return = 0.0;         // 年化收益率
    double daily_return = 0.0;          // 日收益率

    // 风险指标
    double volatility = 0.0;            // 波动率
    double max_drawdown = 0.0;          // 最大回撤
    double sharpe_ratio = 0.0;          // 夏普比率
    double sortino_ratio = 0.0;         // 索提诺比率

    // 统计指标
    double win_rate = 0.0;              // 胜率
    double profit_loss_ratio = 0.0;     // 盈亏比
    int64_t total_trades = 0;           // 总交易数
    int64_t win_trades = 0;             // 盈利交易数

    nlohmann::json to_json() const;
    static RiskMetrics from_json(const nlohmann::json& j);
};

// 工具函数
namespace utils {
    /**
     * @brief 方向枚举转字符串
     */
    std::string direction_to_string(Direction dir);

    /**
     * @brief 字符串转方向枚举
     */
    Direction string_to_direction(const std::string& str);

    /**
     * @brief 开平标志转字符串
     */
    std::string offset_to_string(Offset offset);

    /**
     * @brief 字符串转开平标志
     */
    Offset string_to_offset(const std::string& str);

    /**
     * @brief 订单状态转字符串
     */
    std::string order_status_to_string(OrderStatus status);

    /**
     * @brief 字符串转订单状态
     */
    OrderStatus string_to_order_status(const std::string& str);

    /**
     * @brief 价格类型转字符串
     */
    std::string price_type_to_string(PriceType type);

    /**
     * @brief 字符串转价格类型
     */
    PriceType string_to_price_type(const std::string& str);

    /**
     * @brief 时间条件转字符串
     */
    std::string time_condition_to_string(TimeCondition condition);

    /**
     * @brief 字符串转时间条件
     */
    TimeCondition string_to_time_condition(const std::string& str);

    /**
     * @brief 生成唯一交易ID
     */
    std::string generate_trade_id();

    /**
     * @brief 验证订单有效性
     */
    bool validate_order(const Order& order);

    /**
     * @brief 计算手续费
     */
    double calculate_commission(const std::string& instrument_id, double volume, double price);
}

} // namespace qaultra::protocol::tifi