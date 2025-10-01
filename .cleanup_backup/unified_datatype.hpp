#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <variant>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace qaultra::data {

using time_point = std::chrono::system_clock::time_point;

/**
 * @brief C++17兼容的日期结构
 */
struct Date {
    int year;
    int month;  // 1-12
    int day;    // 1-31

    Date() : year(1970), month(1), day(1) {}
    Date(int y, int m, int d) : year(y), month(m), day(d) {}

    bool operator==(const Date& other) const {
        return year == other.year && month == other.month && day == other.day;
    }

    bool operator<(const Date& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        return day < other.day;
    }
};

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
    Currency,   // 外汇
    Unknown     // 未知
};

/**
 * @brief 数据频率枚举
 */
enum class Frequency {
    Tick,       // 逐笔
    OneMin,     // 1分钟
    FiveMin,    // 5分钟
    FifteenMin, // 15分钟
    ThirtyMin,  // 30分钟
    OneHour,    // 1小时
    Daily,      // 日线
    Weekly,     // 周线
    Monthly,    // 月线
    Quarterly,  // 季线
    Yearly      // 年线
};

/**
 * @brief 基础OHLCV数据接口
 */
struct IMarketData {
    virtual ~IMarketData() = default;
    virtual std::string get_code() const = 0;
    virtual time_point get_datetime() const = 0;
    virtual double get_open() const = 0;
    virtual double get_high() const = 0;
    virtual double get_low() const = 0;
    virtual double get_close() const = 0;
    virtual double get_volume() const = 0;
    virtual double get_turnover() const = 0;
    virtual nlohmann::json to_json() const = 0;
};

/**
 * @brief 统一的K线数据结构
 * 集成simple和full版本的最佳特性
 */
struct Kline : public IMarketData {
    // 基本信息
    std::string order_book_id;                      // 证券代码
    time_point datetime;                            // 日期时间
    MarketType market_type = MarketType::Stock;     // 市场类型
    Frequency frequency = Frequency::Daily;         // 数据频率

    // OHLCV数据
    double open = 0.0;                              // 开盘价
    double high = 0.0;                              // 最高价
    double low = 0.0;                               // 最低价
    double close = 0.0;                             // 收盘价
    double volume = 0.0;                            // 成交量
    double total_turnover = 0.0;                    // 成交额

    // 扩展信息
    double pre_close = 0.0;                         // 前收盘价
    double limit_up = 0.0;                          // 涨停价
    double limit_down = 0.0;                        // 跌停价
    int num_trades = 0;                             // 成交笔数
    bool suspended = false;                         // 是否停牌

    // 可选的技术指标缓存
    mutable std::optional<double> cached_change_percent;
    mutable std::optional<double> cached_amplitude;

    /**
     * @brief 构造函数
     */
    Kline() : datetime(std::chrono::system_clock::now()) {}

    Kline(const std::string& code,
                 const time_point& dt,
                 double o, double h, double l, double c,
                 double vol, double turnover = 0.0)
        : order_book_id(code), datetime(dt)
        , open(o), high(h), low(l), close(c)
        , volume(vol), total_turnover(turnover) {}

    // 实现IMarketData接口
    std::string get_code() const override { return order_book_id; }
    time_point get_datetime() const override { return datetime; }
    double get_open() const override { return open; }
    double get_high() const override { return high; }
    double get_low() const override { return low; }
    double get_close() const override { return close; }
    double get_volume() const override { return volume; }
    double get_turnover() const override { return total_turnover; }

    /**
     * @brief 计算涨跌幅
     */
    double get_change_percent() const;

    /**
     * @brief 计算振幅
     */
    double get_amplitude() const;

    /**
     * @brief 判断是否涨停
     */
    bool is_limit_up() const;

    /**
     * @brief 判断是否跌停
     */
    bool is_limit_down() const;

    /**
     * @brief 获取涨跌符号
     */
    std::string get_change_sign() const;

    /**
     * @brief 获取成交额
     */
    double get_amount() const { return total_turnover; }

    /**
     * @brief 检查数据有效性
     */
    bool is_valid() const;

    /**
     * @brief 格式化显示
     */
    std::string to_string() const;

    /**
     * @brief 比较运算符
     */
    bool operator==(const Kline& other) const;
    bool operator<(const Kline& other) const;

    /**
     * @brief JSON序列化
     */
    nlohmann::json to_json() const override;
    static Kline from_json(const nlohmann::json& j);

    /**
     * @brief 从简单Kline转换
     */
    static Kline from_simple_kline(const std::string& code,
                                         const std::string& datetime_str,
                                         double o, double h, double l, double c,
                                         double vol, double turnover = 0.0);

    /**
     * @brief 转换为简单格式
     */
    void to_simple_format(std::string& datetime_str,
                         double& o, double& h, double& l, double& c,
                         double& vol, double& turnover) const;
};

/**
 * @brief 中国股票日线数据 - 高精度版本
 */
struct StockCnDay : public IMarketData {
    Date date;                    // 日期
    std::string order_book_id;              // 证券代码
    float num_trades = 0.0f;                // 成交笔数
    float limit_up = 0.0f;                  // 涨停价
    float limit_down = 0.0f;                // 跌停价
    float open = 0.0f;                      // 开盘价
    float high = 0.0f;                      // 最高价
    float low = 0.0f;                       // 最低价
    float close = 0.0f;                     // 收盘价
    float volume = 0.0f;                    // 成交量
    float total_turnover = 0.0f;            // 成交额

    StockCnDay() = default;
    StockCnDay(const Date& d, const std::string& code,
               float trades, float up, float down,
               float o, float h, float l, float c, float vol, float turnover)
        : date(d), order_book_id(code), num_trades(trades)
        , limit_up(up), limit_down(down)
        , open(o), high(h), low(l), close(c)
        , volume(vol), total_turnover(turnover) {}

    // 实现IMarketData接口
    std::string get_code() const override { return order_book_id; }
    time_point get_datetime() const override;
    double get_open() const override { return static_cast<double>(open); }
    double get_high() const override { return static_cast<double>(high); }
    double get_low() const override { return static_cast<double>(low); }
    double get_close() const override { return static_cast<double>(close); }
    double get_volume() const override { return static_cast<double>(volume); }
    double get_turnover() const override { return static_cast<double>(total_turnover); }

    nlohmann::json to_json() const override;
    static StockCnDay from_json(const nlohmann::json& j);

    // 转换为统一格式
    Kline to_unified_kline() const;
    static StockCnDay from_unified_kline(const Kline& kline);
};

/**
 * @brief 中国股票分钟数据
 */
struct StockCn1Min : public IMarketData {
    time_point datetime;                    // 日期时间
    std::string order_book_id;              // 证券代码
    float open = 0.0f;                      // 开盘价
    float high = 0.0f;                      // 最高价
    float low = 0.0f;                       // 最低价
    float close = 0.0f;                     // 收盘价
    float volume = 0.0f;                    // 成交量
    float total_turnover = 0.0f;            // 成交额

    StockCn1Min() = default;
    StockCn1Min(const time_point& dt, const std::string& code,
                float o, float h, float l, float c, float vol, float turnover)
        : datetime(dt), order_book_id(code)
        , open(o), high(h), low(l), close(c)
        , volume(vol), total_turnover(turnover) {}

    // 实现IMarketData接口
    std::string get_code() const override { return order_book_id; }
    time_point get_datetime() const override { return datetime; }
    double get_open() const override { return static_cast<double>(open); }
    double get_high() const override { return static_cast<double>(high); }
    double get_low() const override { return static_cast<double>(low); }
    double get_close() const override { return static_cast<double>(close); }
    double get_volume() const override { return static_cast<double>(volume); }
    double get_turnover() const override { return static_cast<double>(total_turnover); }

    nlohmann::json to_json() const override;
    static StockCn1Min from_json(const nlohmann::json& j);

    // 转换为统一格式
    Kline to_unified_kline() const;
    static StockCn1Min from_unified_kline(const Kline& kline);
};

/**
 * @brief 中国期货分钟数据
 */
struct FutureCn1Min : public IMarketData {
    time_point datetime;                    // 日期时间
    std::string order_book_id;              // 证券代码
    float open = 0.0f;                      // 开盘价
    float high = 0.0f;                      // 最高价
    float low = 0.0f;                       // 最低价
    float close = 0.0f;                     // 收盘价
    float volume = 0.0f;                    // 成交量
    float total_turnover = 0.0f;            // 成交额
    float open_interest = 0.0f;             // 持仓量

    FutureCn1Min() = default;
    FutureCn1Min(const time_point& dt, const std::string& code,
                 float o, float h, float l, float c, float vol, float turnover, float oi = 0.0f)
        : datetime(dt), order_book_id(code)
        , open(o), high(h), low(l), close(c)
        , volume(vol), total_turnover(turnover), open_interest(oi) {}

    // 实现IMarketData接口
    std::string get_code() const override { return order_book_id; }
    time_point get_datetime() const override { return datetime; }
    double get_open() const override { return static_cast<double>(open); }
    double get_high() const override { return static_cast<double>(high); }
    double get_low() const override { return static_cast<double>(low); }
    double get_close() const override { return static_cast<double>(close); }
    double get_volume() const override { return static_cast<double>(volume); }
    double get_turnover() const override { return static_cast<double>(total_turnover); }

    double get_open_interest() const { return static_cast<double>(open_interest); }

    nlohmann::json to_json() const override;
    static FutureCn1Min from_json(const nlohmann::json& j);

    // 转换为统一格式
    Kline to_unified_kline() const;
    static FutureCn1Min from_unified_kline(const Kline& kline, float oi = 0.0f);
};

/**
 * @brief 多类型市场数据容器
 */
using MarketDataVariant = std::variant<Kline, StockCnDay, StockCn1Min, FutureCn1Min>;

/**
 * @brief 市场数据容器类
 */
class MarketDataContainer {
public:
    using DataVector = std::vector<MarketDataVariant>;
    using const_iterator = DataVector::const_iterator;
    using iterator = DataVector::iterator;

    MarketDataContainer() = default;
    explicit MarketDataContainer(const std::string& code) : code_(code) {}

    // 数据添加
    void add_data(const Kline& data);
    void add_data(const StockCnDay& data);
    void add_data(const StockCn1Min& data);
    void add_data(const FutureCn1Min& data);

    // 批量添加
    template<typename T>
    void add_batch(const std::vector<T>& data_list);

    // 数据访问
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    const MarketDataVariant& operator[](size_t index) const { return data_[index]; }
    const MarketDataVariant& at(size_t index) const { return data_.at(index); }

    // 迭代器
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }

    // 数据查询
    std::vector<MarketDataVariant> get_data_by_time_range(
        const time_point& start, const time_point& end) const;

    MarketDataVariant get_latest_data() const;
    std::optional<MarketDataVariant> find_by_datetime(const time_point& dt) const;

    // 数据转换
    std::vector<Kline> to_unified_klines() const;

    // 统计信息
    struct Statistics {
        size_t total_count = 0;
        time_point start_time;
        time_point end_time;
        double max_price = 0.0;
        double min_price = 0.0;
        double total_volume = 0.0;
        double total_turnover = 0.0;
    };

    Statistics get_statistics() const;

    // 数据排序
    void sort_by_datetime();

    // 清理和维护
    void clear() { data_.clear(); }
    void reserve(size_t capacity) { data_.reserve(capacity); }

    // 序列化
    nlohmann::json to_json() const;
    static MarketDataContainer from_json(const nlohmann::json& j);

    // 代码访问
    const std::string& get_code() const { return code_; }
    void set_code(const std::string& code) { code_ = code; }

private:
    std::string code_;
    DataVector data_;
};

/**
 * @brief 数据类型工具函数
 */
namespace utils {

    /**
     * @brief 时间转换工具
     */
    time_point string_to_time_point(const std::string& datetime_str);
    std::string time_point_to_string(const time_point& tp);
    Date time_point_to_ymd(const time_point& tp);
    time_point ymd_to_time_point(const Date& ymd);

    /**
     * @brief 市场类型识别
     */
    MarketType detect_market_type(const std::string& code);
    std::string market_type_to_string(MarketType type);
    MarketType string_to_market_type(const std::string& type_str);

    /**
     * @brief 频率转换
     */
    std::string frequency_to_string(Frequency freq);
    Frequency string_to_frequency(const std::string& freq_str);

    /**
     * @brief 数据验证
     */
    bool validate_ohlcv(double o, double h, double l, double c, double v);
    bool validate_kline_data(const IMarketData& data);

    /**
     * @brief 访问者模式辅助函数
     */
    template<typename Visitor>
    auto visit_market_data(const MarketDataVariant& data, Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), data);
    }

    /**
     * @brief 数据转换辅助
     */
    Kline variant_to_unified_kline(const MarketDataVariant& data);
    IMarketData& get_market_data_interface(MarketDataVariant& data);
    const IMarketData& get_market_data_interface(const MarketDataVariant& data);
}

} // namespace qaultra::data