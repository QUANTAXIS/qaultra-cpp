#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace qaultra::data {

using time_point = std::chrono::system_clock::time_point;

/**
 * @brief 市场类型枚举
 */
enum class MarketType {
    Stock,      // 股票
    Future,     // 期货
    Index,      // 指数
    Bond,       // 债券
    Option,     // 期权
    Fund,       // 基金
    Unknown     // 未知
};

/**
 * @brief K线数据结构 - 简化版C++17兼容
 */
struct Kline {
    std::string order_book_id;                      // 证券代码
    std::string datetime;                           // 日期时间字符串
    double open = 0.0;                              // 开盘价
    double close = 0.0;                             // 收盘价
    double high = 0.0;                              // 最高价
    double low = 0.0;                               // 最低价
    double volume = 0.0;                            // 成交量
    double total_turnover = 0.0;                    // 成交额
    double limit_up = 0.0;                          // 涨停价
    double limit_down = 0.0;                        // 跌停价
    double pre_close = 0.0;                         // 前收盘价
    bool suspended = false;                         // 是否停牌

    /**
     * @brief 默认构造函数
     */
    Kline() = default;

    /**
     * @brief 构造函数
     */
    Kline(const std::string& id, const std::string& dt, double o, double c,
          double h, double l, double v, double turnover)
        : order_book_id(id), datetime(dt), open(o), close(c),
          high(h), low(l), volume(v), total_turnover(turnover) {}

    /**
     * @brief 计算涨跌幅
     */
    double get_change_percent() const;

    /**
     * @brief 是否涨停
     */
    bool is_limit_up() const;

    /**
     * @brief 是否跌停
     */
    bool is_limit_down() const;

    /**
     * @brief 获取涨跌符号
     */
    std::string get_change_sign() const;

    /**
     * @brief 获取成交额
     */
    double get_amount() const;

    /**
     * @brief 计算振幅
     */
    double get_amplitude() const;

    /**
     * @brief 比较操作符
     */
    bool operator==(const Kline& other) const;

    /**
     * @brief JSON序列化
     */
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

/**
 * @brief 交易数据 - 简化版
 */
struct Trade {
    std::string order_book_id;
    std::string datetime;
    double price = 0.0;
    double volume = 0.0;
    std::string side;  // BUY/SELL

    Trade() = default;
    Trade(const std::string& id, const std::string& dt, double p, double v, const std::string& s)
        : order_book_id(id), datetime(dt), price(p), volume(v), side(s) {}
};

/**
 * @brief 委托数据 - 简化版
 */
struct Order {
    std::string order_book_id;
    std::string datetime;
    double price = 0.0;
    double volume = 0.0;
    std::string side;  // BUY/SELL

    Order() = default;
    Order(const std::string& id, const std::string& dt, double p, double v, const std::string& s)
        : order_book_id(id), datetime(dt), price(p), volume(v), side(s) {}
};

/**
 * @brief 实时行情数据 - 简化版
 */
struct Tick {
    std::string order_book_id;
    std::string datetime;
    double last_price = 0.0;
    double volume = 0.0;
    double total_turnover = 0.0;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double prev_close = 0.0;

    // 买卖盘数据
    std::vector<double> bid_prices;
    std::vector<double> bid_volumes;
    std::vector<double> ask_prices;
    std::vector<double> ask_volumes;

    Tick() = default;
};

/**
 * @brief 工具函数命名空间
 */
namespace utils {
    /**
     * @brief 获取当前日期字符串
     */
    std::string get_current_date_string();

    /**
     * @brief 获取当前日期时间字符串
     */
    std::string get_current_datetime_string();

    /**
     * @brief 检查是否为交易时间
     */
    bool is_trading_time(const std::string& time_str);

    /**
     * @brief 格式化价格
     */
    std::string format_price(double price, int precision = 2);

    /**
     * @brief 格式化成交量
     */
    std::string format_volume(double volume);

    /**
     * @brief 验证证券代码格式
     */
    bool validate_order_book_id(const std::string& order_book_id);

    /**
     * @brief 获取市场类型
     */
    MarketType get_market_type(const std::string& order_book_id);

} // namespace utils

} // namespace qaultra::data