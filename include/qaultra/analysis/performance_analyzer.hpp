#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <optional>
#include <functional>
#include <atomic>
#include <mutex>

#include <nlohmann/json.hpp>

namespace qaultra::analysis {

// 前向声明
struct TradePair;
struct RiskMetrics;
struct PerformanceReport;

/**
 * @brief 交易对结构 - 一次完整的开仓到平仓
 */
struct TradePair {
    std::string code;                    // 品种代码
    std::chrono::system_clock::time_point open_datetime;   // 开仓时间
    std::chrono::system_clock::time_point close_datetime;  // 平仓时间
    std::string open_date;               // 开仓日期字符串
    std::string close_date;              // 平仓日期字符串
    bool is_buy_open;                    // 是否买开
    double amount;                       // 交易数量
    double open_price;                   // 开仓价格
    double close_price;                  // 平仓价格
    std::string open_trade_id;           // 开仓交易ID
    std::string close_trade_id;          // 平仓交易ID
    double pnl_ratio;                    // 盈亏比例
    double pnl_money;                    // 盈亏金额
    double hold_gap_days;                // 持仓天数
    double commission;                   // 手续费

    // 计算方法
    double get_return_rate() const;
    double get_annual_return() const;
    bool is_profitable() const;
    nlohmann::json to_json() const;

    // 构造函数
    TradePair() = default;
    TradePair(const std::string& code, double amount, double open_price,
              const std::string& open_date, bool is_buy_open = true);

    // 平仓方法
    void close_position(double close_price, const std::string& close_date,
                       const std::string& close_trade_id = "");
};

/**
 * @brief 风险指标结构
 */
struct RiskMetrics {
    double annual_return = 0.0;          // 年化收益率
    double total_return = 0.0;           // 总收益率
    double max_drawdown = 0.0;           // 最大回撤
    double sharpe_ratio = 0.0;           // 夏普比率
    double sortino_ratio = 0.0;          // 索提诺比率
    double calmar_ratio = 0.0;           // 卡玛比率
    double omega_ratio = 0.0;            // 欧米茄比率
    double annual_volatility = 0.0;      // 年化波动率
    double downside_risk = 0.0;          // 下行风险
    double alpha = 0.0;                  // Alpha
    double beta = 0.0;                   // Beta
    double value_at_risk_95 = 0.0;       // 95% VaR
    double conditional_var_95 = 0.0;     // 95% CVaR
    double tail_ratio = 0.0;             // 尾部比率
    double skew = 0.0;                   // 偏度
    double kurtosis = 0.0;               // 峰度

    // 交易相关指标
    int total_trades = 0;                // 总交易次数
    int profitable_trades = 0;           // 盈利交易次数
    int loss_trades = 0;                 // 亏损交易次数
    double win_rate = 0.0;               // 胜率
    double profit_loss_ratio = 0.0;      // 盈亏比
    double average_win = 0.0;            // 平均盈利
    double average_loss = 0.0;           // 平均亏损
    double largest_win = 0.0;            // 最大盈利
    double largest_loss = 0.0;           // 最大亏损

    // 持仓相关
    double average_holding_period = 0.0; // 平均持仓天数

    nlohmann::json to_json() const;
    void print_report() const;
};

/**
 * @brief 性能报告结构
 */
struct PerformanceReport {
    std::string account_id;              // 账户ID
    std::string start_date;              // 开始日期
    std::string end_date;                // 结束日期
    std::string benchmark;               // 基准

    RiskMetrics metrics;                 // 风险指标
    std::vector<TradePair> trade_pairs;  // 交易对
    std::vector<double> daily_returns;   // 每日收益率
    std::vector<double> cumulative_returns; // 累计收益率
    std::vector<double> benchmark_returns;   // 基准收益率
    std::vector<std::string> dates;      // 日期序列

    // 分品种分析
    std::unordered_map<std::string, RiskMetrics> by_asset;

    // 行业/板块分析
    std::unordered_map<std::string, double> sector_exposure;
    std::unordered_map<std::string, double> style_exposure;

    nlohmann::json to_json() const;
    bool save_to_file(const std::string& filename) const;
};

/**
 * @brief 单个品种性能分析器
 */
class SingleAssetPerformanceAnalyzer {
public:
    explicit SingleAssetPerformanceAnalyzer(const std::string& asset_code);

    // 添加交易对
    void add_trade_pair(const TradePair& trade_pair);

    // 计算性能指标
    RiskMetrics calculate_metrics() const;

    // 获取所有交易对
    const std::vector<TradePair>& get_trade_pairs() const { return trade_pairs_; }

    // 统计信息
    double get_total_pnl() const;
    double get_total_return() const;
    int get_trade_count() const { return trade_pairs_.size(); }

private:
    std::string asset_code_;
    std::vector<TradePair> trade_pairs_;
    mutable std::mutex mutex_; // 线程安全
};

/**
 * @brief 投资组合性能分析器
 */
class PortfolioPerformanceAnalyzer {
public:
    explicit PortfolioPerformanceAnalyzer(const std::string& portfolio_id);

    // 设置基准
    void set_benchmark(const std::vector<double>& benchmark_returns,
                      const std::vector<std::string>& dates);

    // 添加账户净值曲线
    void add_account_curve(const std::vector<double>& account_values,
                          const std::vector<std::string>& dates);

    // 添加交易对
    void add_trade_pair(const TradePair& trade_pair);
    void add_trade_pairs(const std::vector<TradePair>& trade_pairs);

    // 计算综合性能指标
    RiskMetrics calculate_portfolio_metrics() const;

    // 分品种分析
    std::unordered_map<std::string, RiskMetrics> calculate_by_asset_metrics() const;

    // 生成完整报告
    PerformanceReport generate_report() const;

    // 比较分析
    double calculate_tracking_error() const;
    double calculate_information_ratio() const;
    std::vector<double> calculate_excess_returns() const;

    // 风险分解
    double calculate_active_risk() const;
    std::unordered_map<std::string, double> calculate_risk_attribution() const;

private:
    std::string portfolio_id_;
    std::vector<double> account_values_;
    std::vector<std::string> dates_;
    std::vector<double> benchmark_returns_;
    std::vector<std::string> benchmark_dates_;
    std::vector<TradePair> trade_pairs_;

    // 分品种数据
    std::unordered_map<std::string, std::vector<TradePair>> trade_pairs_by_asset_;

    mutable std::mutex mutex_;

    // 内部计算方法
    std::vector<double> calculate_daily_returns() const;
    std::vector<double> calculate_cumulative_returns() const;
    void update_trade_pairs_by_asset();
};

/**
 * @brief 高级风险计算工具类
 */
class RiskCalculator {
public:
    // 基础指标计算
    static double calculate_return(const std::vector<double>& values);
    static double calculate_annual_return(const std::vector<double>& returns, int trading_days = 252);
    static double calculate_volatility(const std::vector<double>& returns, bool annualized = true);
    static double calculate_max_drawdown(const std::vector<double>& cumulative_returns);

    // 风险比率
    static double calculate_sharpe_ratio(const std::vector<double>& returns,
                                        double risk_free_rate = 0.0);
    static double calculate_sortino_ratio(const std::vector<double>& returns,
                                         double risk_free_rate = 0.0);
    static double calculate_calmar_ratio(const std::vector<double>& returns);
    static double calculate_omega_ratio(const std::vector<double>& returns,
                                       double threshold = 0.0);

    // 相对指标
    static double calculate_alpha(const std::vector<double>& portfolio_returns,
                                 const std::vector<double>& benchmark_returns,
                                 double risk_free_rate = 0.0);
    static double calculate_beta(const std::vector<double>& portfolio_returns,
                                const std::vector<double>& benchmark_returns);
    static double calculate_tracking_error(const std::vector<double>& portfolio_returns,
                                          const std::vector<double>& benchmark_returns);
    static double calculate_information_ratio(const std::vector<double>& portfolio_returns,
                                             const std::vector<double>& benchmark_returns);

    // 下行风险指标
    static double calculate_downside_risk(const std::vector<double>& returns,
                                         double threshold = 0.0);
    static double calculate_value_at_risk(const std::vector<double>& returns,
                                         double confidence = 0.05);
    static double calculate_conditional_var(const std::vector<double>& returns,
                                           double confidence = 0.05);

    // 高阶矩
    static double calculate_skewness(const std::vector<double>& returns);
    static double calculate_kurtosis(const std::vector<double>& returns);
    static double calculate_tail_ratio(const std::vector<double>& returns);

    // 滚动指标
    static std::vector<double> calculate_rolling_sharpe(const std::vector<double>& returns,
                                                       int window = 252);
    static std::vector<double> calculate_rolling_volatility(const std::vector<double>& returns,
                                                           int window = 252);
    static std::vector<double> calculate_rolling_max_drawdown(const std::vector<double>& cumulative_returns,
                                                             int window = 252);

    // 辅助函数
    static std::vector<double> calculate_excess_returns(const std::vector<double>& portfolio_returns,
                                                       const std::vector<double>& benchmark_returns);

private:
    // 私有辅助函数
    static double calculate_mean(const std::vector<double>& values);
    static double calculate_variance(const std::vector<double>& values, double mean);
    static double calculate_covariance(const std::vector<double>& x, const std::vector<double>& y);
};

/**
 * @brief 基准比较分析器
 */
class BenchmarkComparator {
public:
    BenchmarkComparator(const std::vector<double>& portfolio_returns,
                       const std::vector<double>& benchmark_returns,
                       const std::vector<std::string>& dates);

    // 超额收益分析
    std::vector<double> get_excess_returns() const;
    double get_average_excess_return() const;
    double get_excess_return_volatility() const;

    // 相对性能指标
    double get_information_ratio() const;
    double get_tracking_error() const;

    // 上行/下行捕获比率
    double get_upside_capture_ratio() const;
    double get_downside_capture_ratio() const;

    // 时期分析
    struct PeriodAnalysis {
        std::string period_name;
        double portfolio_return;
        double benchmark_return;
        double excess_return;
        double portfolio_volatility;
        double benchmark_volatility;
    };

    std::vector<PeriodAnalysis> analyze_periods(const std::vector<std::pair<std::string, std::string>>& periods) const;

private:
    std::vector<double> portfolio_returns_;
    std::vector<double> benchmark_returns_;
    std::vector<std::string> dates_;
};

/**
 * @brief 交易分析工具
 */
class TradeAnalyzer {
public:
    explicit TradeAnalyzer(const std::vector<TradePair>& trade_pairs);

    // 基础统计
    struct TradeStatistics {
        int total_trades;
        int profitable_trades;
        int loss_trades;
        double win_rate;
        double average_win;
        double average_loss;
        double profit_loss_ratio;
        double largest_win;
        double largest_loss;
        double total_pnl;
        double average_holding_period;
    };

    TradeStatistics get_statistics() const;

    // 分品种分析
    std::unordered_map<std::string, TradeStatistics> analyze_by_asset() const;

    // 时间分析
    std::unordered_map<std::string, TradeStatistics> analyze_by_month() const;
    std::unordered_map<std::string, TradeStatistics> analyze_by_quarter() const;

    // 持仓时间分析
    std::vector<double> get_holding_period_distribution() const;

    // 盈亏分布
    std::vector<double> get_pnl_distribution() const;

private:
    std::vector<TradePair> trade_pairs_;

    TradeStatistics calculate_statistics(const std::vector<TradePair>& trades) const;
};

} // namespace qaultra::analysis