#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace qaultra::protocol::mifi {

/// MIFI (Market Information Format Interface) implementation
/// 标准化市场数据格式接口

/**
 * @brief 市场类型枚举
 */
enum class MarketType {
    STOCK,          // 股票
    FUTURE,         // 期货
    OPTION,         // 期权
    FUND,           // 基金
    BOND,           // 债券
    INDEX,          // 指数
    FOREX,          // 外汇
    CRYPTO,         // 加密货币
    COMMODITY,      // 商品
    UNKNOWN         // 未知
};

/**
 * @brief 交易状态枚举
 */
enum class TradingStatus {
    TRADING,        // 正常交易
    HALT,           // 停牌
    SUSPENSION,     // 暂停交易
    PRE_OPEN,       // 开盘前
    CLOSED,         // 收盘
    AUCTION,        // 集合竞价
    UNKNOWN         // 未知状态
};

/**
 * @brief K线数据结构 - MIFI标准
 */
struct Kline {
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码
    std::string datetime;               // 时间戳 (ISO 8601格式)
    MarketType market_type;             // 市场类型

    // OHLCV数据
    double open = 0.0;                  // 开盘价
    double high = 0.0;                  // 最高价
    double low = 0.0;                   // 最低价
    double close = 0.0;                 // 收盘价
    double volume = 0.0;                // 成交量
    double amount = 0.0;                // 成交额

    // 附加信息
    double pre_close = 0.0;             // 前收盘价
    double settle_price = 0.0;          // 结算价（期货）
    double pre_settle = 0.0;            // 前结算价（期货）
    double limit_up = 0.0;              // 涨停价
    double limit_down = 0.0;            // 跌停价
    int64_t open_interest = 0;          // 持仓量（期货）

    // 统计数据
    int64_t trade_count = 0;            // 成交笔数
    double avg_price = 0.0;             // 均价
    double turnover_rate = 0.0;         // 换手率（股票）

    /**
     * @brief 计算涨跌幅
     */
    double get_change_percent() const {
        if (pre_close == 0.0) return 0.0;
        return (close - pre_close) / pre_close * 100.0;
    }

    /**
     * @brief 计算涨跌额
     */
    double get_change_amount() const {
        return close - pre_close;
    }

    /**
     * @brief 计算振幅
     */
    double get_amplitude() const {
        if (pre_close == 0.0) return 0.0;
        return (high - low) / pre_close * 100.0;
    }

    /**
     * @brief 是否涨停
     */
    bool is_limit_up() const {
        if (limit_up == 0.0) return false;
        return std::abs(close - limit_up) < 1e-9;
    }

    /**
     * @brief 是否跌停
     */
    bool is_limit_down() const {
        if (limit_down == 0.0) return false;
        return std::abs(close - limit_down) < 1e-9;
    }

    nlohmann::json to_json() const;
    static Kline from_json(const nlohmann::json& j);
};

/**
 * @brief Tick数据结构 - 实时行情
 */
struct Tick {
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码
    std::string datetime;               // 时间戳
    MarketType market_type;             // 市场类型
    TradingStatus status;               // 交易状态

    // 价格数据
    double last_price = 0.0;            // 最新价
    double pre_close = 0.0;             // 前收盘价
    double open = 0.0;                  // 开盘价
    double high = 0.0;                  // 最高价
    double low = 0.0;                   // 最低价

    // 成交数据
    double volume = 0.0;                // 成交量
    double amount = 0.0;                // 成交额
    int64_t trade_count = 0;            // 成交笔数

    // 买卖盘数据（最多10档）
    std::vector<double> bid_prices;     // 买价
    std::vector<double> bid_volumes;    // 买量
    std::vector<double> ask_prices;     // 卖价
    std::vector<double> ask_volumes;    // 卖量

    // 期货特有数据
    double settle_price = 0.0;          // 结算价
    double pre_settle = 0.0;            // 前结算价
    int64_t open_interest = 0;          // 持仓量

    // 涨跌停价格
    double limit_up = 0.0;              // 涨停价
    double limit_down = 0.0;            // 跌停价

    /**
     * @brief 获取买一价
     */
    double get_bid1() const {
        return bid_prices.empty() ? 0.0 : bid_prices[0];
    }

    /**
     * @brief 获取卖一价
     */
    double get_ask1() const {
        return ask_prices.empty() ? 0.0 : ask_prices[0];
    }

    /**
     * @brief 获取买卖价差
     */
    double get_spread() const {
        return get_ask1() - get_bid1();
    }

    /**
     * @brief 获取中间价
     */
    double get_mid_price() const {
        return (get_bid1() + get_ask1()) / 2.0;
    }

    nlohmann::json to_json() const;
    static Tick from_json(const nlohmann::json& j);
};

/**
 * @brief 成交数据结构
 */
struct Transaction {
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码
    std::string datetime;               // 时间戳
    std::string trade_id;               // 成交编号

    double price = 0.0;                 // 成交价
    double volume = 0.0;                // 成交量
    std::string direction;              // 买卖方向: "BUY", "SELL", "UNKNOWN"

    nlohmann::json to_json() const;
    static Transaction from_json(const nlohmann::json& j);
};

/**
 * @brief 委托队列数据结构
 */
struct OrderQueue {
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码
    std::string datetime;               // 时间戳

    struct PriceLevel {
        double price = 0.0;             // 价格
        double volume = 0.0;            // 数量
        int64_t order_count = 0;        // 订单数
    };

    std::vector<PriceLevel> buy_queue;  // 买盘队列
    std::vector<PriceLevel> sell_queue; // 卖盘队列

    nlohmann::json to_json() const;
    static OrderQueue from_json(const nlohmann::json& j);
};

/**
 * @brief 市场状态信息
 */
struct MarketStatus {
    std::string exchange_id;            // 交易所代码
    std::string datetime;               // 时间戳
    TradingStatus status;               // 交易状态
    std::string message;                // 状态描述

    // 交易时段信息
    std::string session_begin;          // 交易开始时间
    std::string session_end;            // 交易结束时间
    std::string auction_begin;          // 集合竞价开始时间
    std::string auction_end;            // 集合竞价结束时间

    nlohmann::json to_json() const;
    static MarketStatus from_json(const nlohmann::json& j);
};

/**
 * @brief 合约信息
 */
struct InstrumentInfo {
    std::string instrument_id;          // 合约代码
    std::string exchange_id;            // 交易所代码
    std::string product_id;             // 品种代码
    std::string instrument_name;        // 合约名称
    MarketType market_type;             // 市场类型

    // 交易参数
    double price_tick = 0.0;            // 最小变动价位
    double lot_size = 1.0;              // 交易单位
    double multiplier = 1.0;            // 合约乘数

    // 保证金和手续费
    double margin_rate = 0.0;           // 保证金率
    double commission_rate = 0.0;       // 手续费率
    double min_commission = 0.0;        // 最小手续费

    // 涨跌停限制
    double limit_up_rate = 0.0;         // 涨停幅度
    double limit_down_rate = 0.0;       // 跌停幅度

    // 时间信息
    std::string list_date;              // 上市日期
    std::string expire_date;            // 到期日期
    std::string delivery_date;          // 交割日期

    // 状态信息
    bool is_trading = true;             // 是否可交易
    bool is_suspended = false;          // 是否停牌

    nlohmann::json to_json() const;
    static InstrumentInfo from_json(const nlohmann::json& j);
};

// 工具函数
namespace utils {
    /**
     * @brief 市场类型转字符串
     */
    std::string market_type_to_string(MarketType type);

    /**
     * @brief 字符串转市场类型
     */
    MarketType string_to_market_type(const std::string& str);

    /**
     * @brief 交易状态转字符串
     */
    std::string trading_status_to_string(TradingStatus status);

    /**
     * @brief 字符串转交易状态
     */
    TradingStatus string_to_trading_status(const std::string& str);

    /**
     * @brief 验证合约代码格式
     */
    bool validate_instrument_id(const std::string& instrument_id);

    /**
     * @brief 从合约代码获取市场类型
     */
    MarketType get_market_type_from_instrument(const std::string& instrument_id);

    /**
     * @brief 标准化时间格式
     */
    std::string standardize_datetime(const std::string& datetime);
}

} // namespace qaultra::protocol::mifi