#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>

// 前向声明
namespace qaultra {
namespace account {
    class QA_Account;
    class QA_Position;
}
namespace market {
    class MatchingEngine;
    struct TradeResult;
}
namespace arrow_data {
    class ArrowKlineCollection;
}
}

namespace qaultra::engine {

/**
 * @brief 回测配置结构
 */
struct BacktestConfig {
    std::string start_date = "2024-01-01";     // 开始日期
    std::string end_date = "2024-12-31";       // 结束日期
    double initial_cash = 1000000.0;           // 初始资金
    double commission_rate = 0.0003;           // 手续费率
    std::string benchmark = "000300.XSHG";     // 基准指数

    // 引擎配置
    bool enable_matching_engine = false;       // 是否启用撮合引擎
    int max_threads = 4;                       // 最大线程数

    // 其他配置
    bool enable_cache = true;                  // 是否启用缓存
    bool enable_logging = true;                // 是否启用日志
};

/**
 * @brief 回测结果结构
 */
struct BacktestResults {
    double total_return = 0.0;                 // 总收益率
    double annual_return = 0.0;                // 年化收益率
    double sharpe_ratio = 0.0;                 // 夏普比率
    double max_drawdown = 0.0;                 // 最大回撤
    double volatility = 0.0;                   // 波动率
    double win_rate = 0.0;                     // 胜率
    double profit_factor = 0.0;                // 盈亏比
    int total_trades = 0;                      // 总交易次数
    double final_value = 0.0;                  // 最终资产价值

    std::vector<double> equity_curve;          // 资产曲线
    std::vector<double> daily_returns;         // 每日收益率
};

/**
 * @brief 策略上下文
 */
struct StrategyContext {
    std::shared_ptr<account::QA_Account> account;
    std::vector<std::string> universe;        // 股票池
    std::string current_date;                 // 当前日期
    double current_price = 0.0;               // 当前价格

    // 数据缓存
    mutable std::mutex cache_mutex_;
    // std::unordered_map<std::string, std::shared_ptr<arrow_data::ArrowKlineCollection>> data_cache_; // 暂时注释掉

    // 接口方法
    double get_price(const std::string& symbol) const;
    std::vector<double> get_history(const std::string& symbol, int window, const std::string& field = "close") const;
    std::shared_ptr<account::QA_Position> get_position(const std::string& symbol) const;
    double get_cash() const;
    double get_portfolio_value() const;
    void log(const std::string& message) const;
};

/**
 * @brief 策略基类
 */
class Strategy {
public:
    virtual ~Strategy() = default;

    /**
     * @brief 策略初始化
     */
    virtual void initialize(StrategyContext& context) = 0;

    /**
     * @brief 处理数据
     */
    virtual void handle_data(StrategyContext& context) = 0;

    /**
     * @brief 开盘前处理
     */
    virtual void before_market_open(StrategyContext& /* context */) {}

    /**
     * @brief 收盘后处理
     */
    virtual void after_market_close(StrategyContext& /* context */) {}

    /**
     * @brief 获取策略参数
     */
    virtual std::map<std::string, double> get_parameters() const = 0;

    /**
     * @brief 设置策略参数
     */
    virtual void set_parameter(const std::string& name, double value) = 0;
};

/**
 * @brief 简单移动平均策略
 */
class SMAStrategy : public Strategy {
public:
    SMAStrategy(int fast_window = 5, int slow_window = 20)
        : fast_window_(fast_window), slow_window_(slow_window) {}

    void initialize(StrategyContext& context) override;
    void handle_data(StrategyContext& context) override;
    std::map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    int fast_window_;
    int slow_window_;
    std::unordered_map<std::string, bool> positions_;
};

/**
 * @brief 动量策略
 */
class MomentumStrategy : public Strategy {
public:
    MomentumStrategy(int lookback_window = 20, double threshold = 0.05)
        : lookback_window_(lookback_window), threshold_(threshold) {}

    void initialize(StrategyContext& context) override;
    void handle_data(StrategyContext& context) override;
    std::map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    int lookback_window_;
    double threshold_;
    std::unordered_map<std::string, std::vector<double>> price_history_;
};

/**
 * @brief 均值回归策略
 */
class MeanReversionStrategy : public Strategy {
public:
    MeanReversionStrategy(int window = 20, double z_score_threshold = 2.0)
        : window_(window), z_score_threshold_(z_score_threshold) {}

    void initialize(StrategyContext& context) override;
    void handle_data(StrategyContext& context) override;
    std::map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    int window_;
    double z_score_threshold_;
    std::unordered_map<std::string, std::vector<double>> price_buffer_;
};

/**
 * @brief 回测引擎主类
 */
class BacktestEngine {
public:
    explicit BacktestEngine(const BacktestConfig& config);
    ~BacktestEngine();

    /**
     * @brief 添加策略
     */
    void add_strategy(std::shared_ptr<Strategy> strategy);

    /**
     * @brief 设置股票池
     */
    void set_universe(const std::vector<std::string>& symbols);

    /**
     * @brief 加载数据
     */
    bool load_data(const std::string& data_source = "");

    /**
     * @brief 运行回测
     */
    BacktestResults run();

    /**
     * @brief 保存结果
     */
    bool save_results(const std::string& filename) const;

    /**
     * @brief 获取性能总结
     */
    std::map<std::string, double> get_performance_summary() const;

    /**
     * @brief 绘制资产曲线
     */
    std::vector<std::pair<std::string, double>> plot_equity_curve() const;

    /**
     * @brief 获取交易分析
     */
    std::map<std::string, std::vector<double>> get_trade_analysis() const;

private:
    BacktestConfig config_;
    BacktestResults results_;

    // 核心组件
    std::shared_ptr<account::QA_Account> account_;
    // std::unique_ptr<market::MatchingEngine> matching_engine_; // 暂时注释掉，避免不完整类型问题
    std::vector<std::shared_ptr<Strategy>> strategies_;

    // 数据管理
    std::vector<std::string> universe_;
    // std::unordered_map<std::string, std::shared_ptr<arrow_data::ArrowKlineCollection>> market_data_; // 暂时注释掉
    std::vector<std::string> date_index_;

    // 运行状态
    std::atomic<bool> is_running_{false};
    size_t current_index_ = 0;
    std::string current_date_;

    // 性能记录
    std::vector<double> daily_equity_;
    std::vector<std::pair<std::string, double>> trade_records_;

    // 内部方法
    void initialize_account();
    void initialize_matching_engine();
    bool load_data_from_file(const std::string& filename);
    bool load_data_from_database();
    void run_single_day(const std::string& date);
    void update_market_data(const std::string& date);
    void execute_strategies(StrategyContext& context);
    void record_daily_performance();
    void calculate_performance_metrics();

    // 性能计算
    double calculate_sharpe_ratio() const;
    double calculate_max_drawdown() const;
    double calculate_annual_return() const;
    double calculate_volatility() const;
    double calculate_win_rate() const;
    double calculate_profit_factor() const;

    // SIMD优化计算
    std::vector<double> calculate_returns_simd(const std::vector<double>& prices) const;
    double calculate_volatility_simd(const std::vector<double>& returns) const;
};

/**
 * @brief 性能分析工具函数
 */
namespace utils {
    double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate = 0.0);
    double calculate_max_drawdown(const std::vector<double>& equity_curve);
    double calculate_annual_return(const std::vector<double>& equity_curve, int trading_days = 252);
    double calculate_volatility(const std::vector<double>& returns, bool annualized = true);
    double calculate_win_rate(const std::vector<double>& trade_returns);
    double calculate_profit_factor(const std::vector<double>& trade_returns);
    std::vector<double> calculate_rolling_sharpe(const std::vector<double>& returns, int window);
    double calculate_beta(const std::vector<double>& strategy_returns, const std::vector<double>& benchmark_returns);
    double calculate_alpha(const std::vector<double>& strategy_returns, const std::vector<double>& benchmark_returns, double risk_free_rate = 0.0);
}

/**
 * @brief 策略工厂函数
 */
namespace factory {
    std::shared_ptr<Strategy> create_sma_strategy(int fast_window = 5, int slow_window = 20);
    std::shared_ptr<Strategy> create_momentum_strategy(int lookback_window = 20, double threshold = 0.05);
    std::shared_ptr<Strategy> create_mean_reversion_strategy(int window = 20, double z_score_threshold = 2.0);
}

} // namespace qaultra::engine