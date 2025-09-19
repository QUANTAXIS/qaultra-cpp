#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace qaultra::data {

/**
 * @brief 中国股票日线数据结构 - 完全匹配Rust StockCnDay
 */
struct StockCnDay {
    std::chrono::year_month_day date;       // 日期
    std::string order_book_id;              // 证券代码
    float num_trades;                       // 成交笔数
    float limit_up;                         // 涨停价
    float limit_down;                       // 跌停价
    float open;                             // 开盘价
    float high;                             // 最高价
    float low;                              // 最低价
    float close;                            // 收盘价
    float volume;                           // 成交量
    float total_turnover;                   // 成交额

    /**
     * @brief 构造函数
     */
    StockCnDay() = default;
    StockCnDay(const std::chrono::year_month_day& date,
               const std::string& order_book_id,
               float num_trades, float limit_up, float limit_down,
               float open, float high, float low, float close,
               float volume, float total_turnover)
        : date(date), order_book_id(order_book_id), num_trades(num_trades),
          limit_up(limit_up), limit_down(limit_down), open(open),
          high(high), low(low), close(close), volume(volume),
          total_turnover(total_turnover) {}

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static StockCnDay from_json(const nlohmann::json& j);
};

/**
 * @brief 中国股票分钟数据结构 - 完全匹配Rust StockCn1Min
 */
struct StockCn1Min {
    std::chrono::system_clock::time_point datetime;  // 日期时间
    std::string order_book_id;                       // 证券代码
    float open;                                      // 开盘价
    float high;                                      // 最高价
    float low;                                       // 最低价
    float close;                                     // 收盘价
    float volume;                                    // 成交量
    float total_turnover;                           // 成交额

    /**
     * @brief 构造函数
     */
    StockCn1Min() = default;
    StockCn1Min(const std::chrono::system_clock::time_point& datetime,
                const std::string& order_book_id,
                float open, float high, float low, float close,
                float volume, float total_turnover)
        : datetime(datetime), order_book_id(order_book_id),
          open(open), high(high), low(low), close(close),
          volume(volume), total_turnover(total_turnover) {}

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static StockCn1Min from_json(const nlohmann::json& j);
};

/**
 * @brief 中国期货分钟数据结构 - 完全匹配Rust FutureCn1Min
 */
struct FutureCn1Min {
    std::chrono::system_clock::time_point datetime;  // 日期时间
    std::chrono::year_month_day trading_date;        // 交易日期
    std::string order_book_id;                       // 合约代码
    float open_interest;                             // 持仓量
    float open;                                      // 开盘价
    float high;                                      // 最高价
    float low;                                       // 最低价
    float close;                                     // 收盘价
    float volume;                                    // 成交量
    float total_turnover;                           // 成交额

    /**
     * @brief 构造函数
     */
    FutureCn1Min() = default;
    FutureCn1Min(const std::chrono::system_clock::time_point& datetime,
                 const std::chrono::year_month_day& trading_date,
                 const std::string& order_book_id,
                 float open_interest, float open, float high, float low,
                 float close, float volume, float total_turnover)
        : datetime(datetime), trading_date(trading_date), order_book_id(order_book_id),
          open_interest(open_interest), open(open), high(high), low(low),
          close(close), volume(volume), total_turnover(total_turnover) {}

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static FutureCn1Min from_json(const nlohmann::json& j);
};

/**
 * @brief 中国期货日线数据结构 - 完全匹配Rust FutureCnDay
 */
struct FutureCnDay {
    std::chrono::year_month_day date;       // 日期
    std::string order_book_id;              // 合约代码
    float limit_up;                         // 涨停价
    float limit_down;                       // 跌停价
    float open_interest;                    // 持仓量
    float prev_settlement;                  // 前结算价
    float settlement;                       // 结算价
    float open;                             // 开盘价
    float high;                             // 最高价
    float low;                              // 最低价
    float close;                            // 收盘价
    float volume;                           // 成交量
    float total_turnover;                   // 成交额

    /**
     * @brief 构造函数
     */
    FutureCnDay() = default;
    FutureCnDay(const std::chrono::year_month_day& date,
                const std::string& order_book_id,
                float limit_up, float limit_down, float open_interest,
                float prev_settlement, float settlement,
                float open, float high, float low, float close,
                float volume, float total_turnover)
        : date(date), order_book_id(order_book_id), limit_up(limit_up),
          limit_down(limit_down), open_interest(open_interest),
          prev_settlement(prev_settlement), settlement(settlement),
          open(open), high(high), low(low), close(close),
          volume(volume), total_turnover(total_turnover) {}

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static FutureCnDay from_json(const nlohmann::json& j);
};

/**
 * @brief K线数据结构 - 完全匹配Rust Kline
 */
struct Kline {
    std::string order_book_id;              // 证券/合约代码
    double open = 0.0;                      // 开盘价
    double close = 0.0;                     // 收盘价
    double high = 0.0;                      // 最高价
    double low = 0.0;                       // 最低价
    double volume = 0.0;                    // 成交量
    double limit_up = 0.0;                  // 涨停价
    double limit_down = 0.0;                // 跌停价
    double total_turnover = 0.0;            // 成交额
    double split_coefficient_to = 0.0;      // 拆股系数
    double dividend_cash_before_tax = 0.0;  // 税前分红

    /**
     * @brief 构造函数
     */
    Kline() = default;
    Kline(const std::string& order_book_id,
          double open, double close, double high, double low,
          double volume, double limit_up, double limit_down,
          double total_turnover,
          double split_coefficient_to = 0.0,
          double dividend_cash_before_tax = 0.0)
        : order_book_id(order_book_id), open(open), close(close),
          high(high), low(low), volume(volume), limit_up(limit_up),
          limit_down(limit_down), total_turnover(total_turnover),
          split_coefficient_to(split_coefficient_to),
          dividend_cash_before_tax(dividend_cash_before_tax) {}

    /**
     * @brief 比较操作符
     */
    bool operator==(const Kline& other) const {
        return order_book_id == other.order_book_id &&
               std::abs(open - other.open) < 1e-9 &&
               std::abs(close - other.close) < 1e-9 &&
               std::abs(high - other.high) < 1e-9 &&
               std::abs(low - other.low) < 1e-9 &&
               std::abs(volume - other.volume) < 1e-9 &&
               std::abs(total_turnover - other.total_turnover) < 1e-9;
    }

    bool operator!=(const Kline& other) const {
        return !(*this == other);
    }

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static Kline from_json(const nlohmann::json& j);

    /**
     * @brief 获取价格变化百分比
     */
    double get_change_percent() const {
        if (open == 0.0) return 0.0;
        return (close - open) / open * 100.0;
    }

    /**
     * @brief 获取价格变化金额
     */
    double get_change_amount() const {
        return close - open;
    }

    /**
     * @brief 判断是否涨停
     */
    bool is_limit_up() const {
        return limit_up > 0.0 && std::abs(close - limit_up) < 1e-6;
    }

    /**
     * @brief 判断是否跌停
     */
    bool is_limit_down() const {
        return limit_down > 0.0 && std::abs(close - limit_down) < 1e-6;
    }

    /**
     * @brief 计算换手率（需要传入流通股本）
     */
    double get_turnover_rate(double circulating_shares) const {
        if (circulating_shares <= 0.0) return 0.0;
        return volume / circulating_shares * 100.0;
    }
};

/**
 * @brief 数据类型工具函数命名空间
 */
namespace utils {
    /**
     * @brief 时间戳转换为字符串
     */
    std::string timestamp_to_string(const std::chrono::system_clock::time_point& tp);

    /**
     * @brief 字符串转换为时间戳
     */
    std::chrono::system_clock::time_point string_to_timestamp(const std::string& str);

    /**
     * @brief 日期转换为字符串
     */
    std::string date_to_string(const std::chrono::year_month_day& date);

    /**
     * @brief 字符串转换为日期
     */
    std::chrono::year_month_day string_to_date(const std::string& str);

    /**
     * @brief 计算两个时间点之间的交易日数
     */
    int trading_days_between(const std::chrono::year_month_day& start,
                           const std::chrono::year_month_day& end);

    /**
     * @brief 判断是否为交易日
     */
    bool is_trading_day(const std::chrono::year_month_day& date);

    /**
     * @brief 获取下一个交易日
     */
    std::chrono::year_month_day next_trading_day(const std::chrono::year_month_day& date);

    /**
     * @brief 获取上一个交易日
     */
    std::chrono::year_month_day prev_trading_day(const std::chrono::year_month_day& date);
}

} // namespace qaultra::data