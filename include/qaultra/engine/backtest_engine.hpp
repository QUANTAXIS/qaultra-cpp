#pragma once

#include "../account/account_full.hpp"
#include "../market/match_engine.hpp"
#include "../arrow/arrow_kline.hpp"
#include "../protocol/qifi.hpp"
#include "../simd/simd_math.hpp"

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

namespace qaultra::engine {

/// 回测配置结构
struct BacktestConfig {
    std::string start_date = "2024-01-01";      ///< 开始日期
    std::string end_date = "2024-12-31";        ///< 结束日期
    double initial_cash = 1000000.0;            ///< 初始资金
    double commission_rate = 0.0025;            ///< 手续费率
    double slippage = 0.0001;                   ///< 滑点
    std::string benchmark = "000300";           ///< 基准指数
    std::string frequency = "1D";               ///< 数据频率 (1M, 5M, 15M, 1H, 1D)
    std::string market_data_source = "";        ///< 市场数据源路径
    std::string output_file = "backtest_results.json"; ///< 结果输出文件
    bool enable_matching_engine = false;        ///< 是否启用撮合引擎
    int max_threads = 4;                        ///< 最大线程数
};

/// 回测结果统计
struct BacktestResults {
    double total_return = 0.0;                  ///< 总收益率
    double annual_return = 0.0;                 ///< 年化收益率
    double sharpe_ratio = 0.0;                  ///< 夏普比率
    double max_drawdown = 0.0;                  ///< 最大回撤
    double volatility = 0.0;                    ///< 波动率
    double win_rate = 0.0;                      ///< 胜率
    double profit_factor = 0.0;                 ///< 盈亏比
    size_t total_trades = 0;                    ///< 总交易次数
    double final_value = 0.0;                   ///< 最终资产值

    std::vector<double> equity_curve;           ///< 资产曲线
    std::vector<std::string> trade_list;        ///< 交易记录
    std::vector<double> daily_returns;          ///< 每日收益率
    std::map<std::string, double> metrics;      ///< 其他指标
};

/// 策略上下文，为策略提供数据和交易接口
class StrategyContext {
public:
    std::string current_date;                   ///< 当前日期
    double current_price = 0.0;                 ///< 当前价格
    std::shared_ptr<account::QA_Account> account; ///< 账户对象
    std::vector<std::string> universe;          ///< 股票池

    /// 获取指定股票的当前价格
    double get_price(const std::string& symbol) const;

    /// 获取历史数据
    std::vector<double> get_history(const std::string& symbol, int window, const std::string& field = "close") const;

    /// 获取持仓信息
    std::shared_ptr<account::Position> get_position(const std::string& symbol) const;

    /// 获取可用资金
    double get_cash() const;

    /// 获取总资产价值
    double get_portfolio_value() const;

    /// 记录日志
    void log(const std::string& message) const;

private:
    mutable std::map<std::string, std::shared_ptr<arrow_data::ArrowKlineCollection>> data_cache_;
    mutable std::mutex cache_mutex_;
};

/// 策略基类
class Strategy {
public:
    virtual ~Strategy() = default;

    /// 策略初始化
    virtual void initialize(StrategyContext& context) = 0;

    /// 处理数据事件（每个时间点调用）
    virtual void handle_data(StrategyContext& context) = 0;

    /// 开盘前调用
    virtual void before_market_open(StrategyContext& context) {}

    /// 收盘后调用
    virtual void after_market_close(StrategyContext& context) {}

    /// 获取策略名称
    virtual std::string get_name() const = 0;

    /// 获取策略参数
    virtual std::map<std::string, double> get_parameters() const = 0;

    /// 设置策略参数
    virtual void set_parameter(const std::string& name, double value) = 0;
};

/// 简单移动平均线策略
class SMAStrategy : public Strategy {
public:
    int fast_window = 5;                        ///< 快速移动平均线窗口
    int slow_window = 20;                       ///< 慢速移动平均线窗口

    SMAStrategy(int fast, int slow) : fast_window(fast), slow_window(slow) {}

    void initialize(StrategyContext& context) override;
    void handle_data(StrategyContext& context) override;
    std::string get_name() const override { return "SMA策略"; }
    std::map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    std::map<std::string, bool> positions_;     ///< 持仓状态
};

/// 动量策略
class MomentumStrategy : public Strategy {
public:
    int lookback_window = 20;                   ///< 回望窗口
    double threshold = 0.02;                    ///< 动量阈值

    MomentumStrategy(int window, double thresh) : lookback_window(window), threshold(thresh) {}

    void initialize(StrategyContext& context) override;
    void handle_data(StrategyContext& context) override;
    std::string get_name() const override { return "动量策略"; }
    std::map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    std::map<std::string, std::vector<double>> price_history_;
};

/// 均值回归策略
class MeanReversionStrategy : public Strategy {
public:
    int window = 20;                            ///< 计算窗口
    double z_score_threshold = 2.0;             ///< Z分数阈值

    MeanReversionStrategy(int win, double thresh) : window(win), z_score_threshold(thresh) {}

    void initialize(StrategyContext& context) override;
    void handle_data(StrategyContext& context) override;
    std::string get_name() const override { return "均值回归策略"; }
    std::map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    std::map<std::string, std::vector<double>> price_buffer_;
};

/// 高性能回测引擎
class BacktestEngine {
public:
    explicit BacktestEngine(const BacktestConfig& config);
    ~BacktestEngine();

    /// 添加策略
    void add_strategy(std::shared_ptr<Strategy> strategy);

    /// 设置交易股票池
    void set_universe(const std::vector<std::string>& symbols);

    /// 加载市场数据
    bool load_data(const std::string& data_source);

    /// 运行回测
    BacktestResults run();

    /// 获取回测结果
    const BacktestResults& get_results() const { return results_; }

    /// 保存结果到文件
    bool save_results(const std::string& filename) const;

    /// 获取性能摘要
    std::map<std::string, double> get_performance_summary() const;

    /// 绘制权益曲线（返回图表数据）
    std::vector<std::pair<std::string, double>> plot_equity_curve() const;

    /// 获取详细交易分析
    std::map<std::string, std::vector<double>> get_trade_analysis() const;

private:
    BacktestConfig config_;                     ///< 回测配置
    BacktestResults results_;                   ///< 回测结果

    std::vector<std::shared_ptr<Strategy>> strategies_; ///< 策略列表
    std::vector<std::string> universe_;         ///< 股票池

    std::shared_ptr<account::QA_Account> account_;      ///< 回测账户
    std::shared_ptr<market::MatchingEngine> matching_engine_; ///< 撮合引擎

    // 市场数据
    std::map<std::string, std::shared_ptr<arrow_data::ArrowKlineCollection>> market_data_;

    // 回测状态
    std::atomic<bool> is_running_{false};
    std::string current_date_;
    size_t current_index_ = 0;

    // 性能统计
    std::vector<double> daily_equity_;          ///< 每日权益
    std::vector<std::string> date_index_;       ///< 日期索引
    std::vector<std::pair<std::string, double>> trade_records_; ///< 交易记录

    mutable std::mutex data_mutex_;             ///< 数据访问锁

    /// 内部方法
    void initialize_account();
    void initialize_matching_engine();
    bool load_data_from_file(const std::string& filename);
    bool load_data_from_database();

    void run_single_day(const std::string& date);
    void update_market_data(const std::string& date);
    void execute_strategies(StrategyContext& context);
    void record_daily_performance();

    void calculate_performance_metrics();
    double calculate_sharpe_ratio() const;
    double calculate_max_drawdown() const;
    double calculate_annual_return() const;
    double calculate_volatility() const;
    double calculate_win_rate() const;
    double calculate_profit_factor() const;

    /// SIMD优化的性能计算
    std::vector<double> calculate_returns_simd(const std::vector<double>& prices) const;
    double calculate_volatility_simd(const std::vector<double>& returns) const;
};

/// 回测工具函数
namespace utils {
    /// 计算夏普比率
    double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate = 0.0);

    /// 计算最大回撤
    double calculate_max_drawdown(const std::vector<double>& equity_curve);

    /// 计算年化收益率
    double calculate_annual_return(const std::vector<double>& equity_curve, int trading_days = 252);

    /// 计算波动率
    double calculate_volatility(const std::vector<double>& returns, bool annualized = true);

    /// 计算胜率
    double calculate_win_rate(const std::vector<double>& trade_returns);

    /// 计算盈亏比
    double calculate_profit_factor(const std::vector<double>& trade_returns);

    /// 计算滚动夏普比率
    std::vector<double> calculate_rolling_sharpe(const std::vector<double>& returns, int window);

    /// 计算Beta系数
    double calculate_beta(const std::vector<double>& strategy_returns,
                         const std::vector<double>& benchmark_returns);

    /// 计算Alpha系数
    double calculate_alpha(const std::vector<double>& strategy_returns,
                          const std::vector<double>& benchmark_returns,
                          double risk_free_rate = 0.0);
}

/// 策略工厂
namespace factory {
    /// 创建SMA策略
    std::shared_ptr<Strategy> create_sma_strategy(int fast_window, int slow_window);

    /// 创建动量策略
    std::shared_ptr<Strategy> create_momentum_strategy(int lookback_window, double threshold);

    /// 创建均值回归策略
    std::shared_ptr<Strategy> create_mean_reversion_strategy(int window, double z_score_threshold);
}

} // namespace qaultra::engine