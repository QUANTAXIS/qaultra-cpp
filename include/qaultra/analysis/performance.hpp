#pragma once

#include "../protocol/qifi.hpp"
#include "../simd/simd_math.hpp"

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <chrono>
#include <functional>

namespace qaultra::analysis {

/// 性能指标结构
struct PerformanceMetrics {
    // 收益指标
    double total_return = 0.0;                ///< 总收益率
    double annual_return = 0.0;               ///< 年化收益率
    double cumulative_return = 0.0;           ///< 累计收益率
    double excess_return = 0.0;               ///< 超额收益率

    // 风险指标
    double volatility = 0.0;                  ///< 波动率
    double downside_volatility = 0.0;         ///< 下行波动率
    double max_drawdown = 0.0;                ///< 最大回撤
    double max_drawdown_duration = 0.0;       ///< 最大回撤持续期
    double var_95 = 0.0;                      ///< 95% VaR
    double cvar_95 = 0.0;                     ///< 95% CVaR

    // 风险调整收益指标
    double sharpe_ratio = 0.0;                ///< 夏普比率
    double sortino_ratio = 0.0;               ///< 索提诺比率
    double calmar_ratio = 0.0;                ///< 卡玛比率
    double omega_ratio = 0.0;                 ///< 欧米伽比率
    double information_ratio = 0.0;           ///< 信息比率

    // 基准比较指标
    double alpha = 0.0;                       ///< Alpha系数
    double beta = 0.0;                        ///< Beta系数
    double tracking_error = 0.0;              ///< 跟踪误差
    double correlation = 0.0;                 ///< 相关系数
    double r_squared = 0.0;                   ///< R平方

    // 交易指标
    double win_rate = 0.0;                    ///< 胜率
    double profit_factor = 0.0;               ///< 盈亏比
    double kelly_ratio = 0.0;                 ///< 凯利比率
    double avg_win_rate = 0.0;                ///< 平均盈利率
    double avg_loss_rate = 0.0;               ///< 平均亏损率
    int total_trades = 0;                     ///< 总交易次数
    int winning_trades = 0;                   ///< 盈利交易次数
    int losing_trades = 0;                    ///< 亏损交易次数

    // 时间序列指标
    std::vector<double> rolling_returns;      ///< 滚动收益率
    std::vector<double> rolling_sharpe;       ///< 滚动夏普比率
    std::vector<double> rolling_volatility;   ///< 滚动波动率
    std::vector<double> drawdown_series;      ///< 回撤序列
    std::vector<double> underwater_curve;     ///< 水下曲线

    /// 转换为JSON格式
    nlohmann::json to_json() const;

    /// 从JSON格式创建
    static PerformanceMetrics from_json(const nlohmann::json& json);
};

/// 归因分析结果
struct AttributionAnalysis {
    // Brinson模型归因
    double asset_allocation_effect = 0.0;     ///< 资产配置效应
    double security_selection_effect = 0.0;   ///< 证券选择效应
    double interaction_effect = 0.0;          ///< 交互效应
    double total_active_return = 0.0;         ///< 总主动收益

    // 风格归因
    std::map<std::string, double> style_exposures;  ///< 风格暴露
    std::map<std::string, double> style_returns;    ///< 风格收益
    double specific_return = 0.0;             ///< 特异收益

    // 行业归因
    std::map<std::string, double> sector_weights;   ///< 行业权重
    std::map<std::string, double> sector_returns;   ///< 行业收益
    std::map<std::string, double> sector_attribution; ///< 行业归因

    /// 转换为JSON格式
    nlohmann::json to_json() const;
};

/// SIMD加速的高性能分析器
class PerformanceAnalyzer {
public:
    /// 构造函数
    explicit PerformanceAnalyzer();

    /// 析构函数
    ~PerformanceAnalyzer() = default;

    // 基础计算方法
    /// @{

    /// 设置无风险利率
    void set_risk_free_rate(double rate);

    /// 设置基准数据
    void set_benchmark(const std::vector<double>& benchmark_returns);

    /// 计算完整性能指标
    PerformanceMetrics calculate_performance(const std::vector<double>& returns,
                                            const std::vector<double>& equity_curve = {});

    /// 计算收益率指标
    PerformanceMetrics calculate_return_metrics(const std::vector<double>& returns);

    /// 计算风险指标
    PerformanceMetrics calculate_risk_metrics(const std::vector<double>& returns);

    /// 计算风险调整收益指标
    PerformanceMetrics calculate_risk_adjusted_metrics(const std::vector<double>& returns);

    /// @}

    // SIMD优化计算
    /// @{

    /// SIMD计算夏普比率
    double calculate_sharpe_ratio_simd(const std::vector<double>& returns);

    /// SIMD计算最大回撤
    double calculate_max_drawdown_simd(const std::vector<double>& equity_curve);

    /// SIMD计算波动率
    double calculate_volatility_simd(const std::vector<double>& returns, bool annualized = true);

    /// SIMD计算VaR
    double calculate_var_simd(const std::vector<double>& returns, double confidence_level = 0.05);

    /// SIMD计算相关系数
    double calculate_correlation_simd(const std::vector<double>& returns1,
                                     const std::vector<double>& returns2);

    /// @}

    // 高级分析方法
    /// @{

    /// 滚动窗口分析
    std::vector<PerformanceMetrics> rolling_analysis(const std::vector<double>& returns,
                                                     int window_size = 252);

    /// 分位数分析
    std::map<double, double> quantile_analysis(const std::vector<double>& returns);

    /// 尾部风险分析
    std::map<std::string, double> tail_risk_analysis(const std::vector<double>& returns);

    /// 压力测试
    std::map<std::string, PerformanceMetrics> stress_testing(
        const std::vector<double>& returns, const std::vector<std::vector<double>>& scenarios);

    /// @}

    // 基准比较分析
    /// @{

    /// 计算Alpha和Beta
    std::pair<double, double> calculate_alpha_beta(const std::vector<double>& returns);

    /// 跟踪误差计算
    double calculate_tracking_error(const std::vector<double>& returns);

    /// 信息比率计算
    double calculate_information_ratio(const std::vector<double>& returns);

    /// @}

    // 交易分析
    /// @{

    /// 分析交易记录
    PerformanceMetrics analyze_trades(const std::vector<protocol::QIFITrade>& trades);

    /// 计算交易成本分析
    std::map<std::string, double> analyze_transaction_costs(
        const std::vector<protocol::QIFITrade>& trades);

    /// 换手率分析
    std::vector<double> calculate_turnover_rate(const std::vector<protocol::QIFIAccount>& snapshots);

    /// @}

    // 归因分析
    /// @{

    /// Brinson归因分析
    AttributionAnalysis brinson_attribution(
        const std::map<std::string, double>& portfolio_weights,
        const std::map<std::string, double>& benchmark_weights,
        const std::map<std::string, double>& portfolio_returns,
        const std::map<std::string, double>& benchmark_returns);

    /// 风格归因分析
    AttributionAnalysis style_attribution(
        const std::vector<double>& portfolio_returns,
        const std::map<std::string, std::vector<double>>& factor_returns);

    /// @}

    // 可视化数据生成
    /// @{

    /// 生成权益曲线数据
    std::vector<std::pair<std::string, double>> generate_equity_curve_data(
        const std::vector<double>& returns, const std::vector<std::string>& dates = {});

    /// 生成回撤曲线数据
    std::vector<std::pair<std::string, double>> generate_drawdown_curve_data(
        const std::vector<double>& equity_curve, const std::vector<std::string>& dates = {});

    /// 生成收益分布数据
    std::map<double, int> generate_return_distribution_data(
        const std::vector<double>& returns, int bins = 50);

    /// @}

private:
    double risk_free_rate_ = 0.0;             ///< 无风险利率
    std::vector<double> benchmark_returns_;   ///< 基准收益率
    bool benchmark_set_ = false;              ///< 基准是否已设置

    /// 内部计算方法
    std::vector<double> calculate_excess_returns(const std::vector<double>& returns) const;
    std::vector<double> calculate_drawdown_series(const std::vector<double>& equity_curve) const;
    double calculate_sortino_ratio(const std::vector<double>& returns) const;
    double calculate_calmar_ratio(const std::vector<double>& returns,
                                 const std::vector<double>& equity_curve) const;
    double calculate_omega_ratio(const std::vector<double>& returns, double threshold = 0.0) const;
};

/// 风险指标计算器
class RiskCalculator {
public:
    /// VaR计算方法枚举
    enum class VaRMethod {
        HISTORICAL,                           ///< 历史模拟法
        PARAMETRIC,                           ///< 参数法
        MONTE_CARLO                           ///< 蒙特卡洛模拟法
    };

    /// 构造函数
    RiskCalculator() = default;

    /// 计算VaR
    double calculate_var(const std::vector<double>& returns, double confidence_level,
                        VaRMethod method = VaRMethod::HISTORICAL);

    /// 计算CVaR (条件VaR)
    double calculate_cvar(const std::vector<double>& returns, double confidence_level);

    /// 计算预期短缺
    double calculate_expected_shortfall(const std::vector<double>& returns, double confidence_level);

    /// GARCH模型波动率预测
    std::vector<double> garch_volatility_forecast(const std::vector<double>& returns, int forecast_days);

    /// 极值理论风险计算
    double extreme_value_theory_var(const std::vector<double>& returns, double confidence_level);

private:
    /// 蒙特卡洛VaR计算
    double monte_carlo_var(const std::vector<double>& returns, double confidence_level, int simulations = 10000);

    /// GARCH(1,1)参数估计
    std::tuple<double, double, double> estimate_garch_parameters(const std::vector<double>& returns);
};

/// 收益归因分析器
class AttributionAnalyzer {
public:
    /// 构造函数
    AttributionAnalyzer() = default;

    /// Brinson模型归因
    AttributionAnalysis brinson_attribution(
        const std::map<std::string, double>& portfolio_weights,
        const std::map<std::string, double>& benchmark_weights,
        const std::map<std::string, double>& asset_returns);

    /// Fama-French三因子归因
    std::map<std::string, double> fama_french_attribution(
        const std::vector<double>& portfolio_returns,
        const std::vector<double>& market_returns,
        const std::vector<double>& smb_returns,      ///< 小盘股减大盘股
        const std::vector<double>& hml_returns);     ///< 高账面市值比减低账面市值比

    /// Carhart四因子归因
    std::map<std::string, double> carhart_attribution(
        const std::vector<double>& portfolio_returns,
        const std::vector<double>& market_returns,
        const std::vector<double>& smb_returns,
        const std::vector<double>& hml_returns,
        const std::vector<double>& mom_returns);     ///< 动量因子

private:
    /// 回归分析
    std::vector<double> multiple_regression(const std::vector<double>& y,
                                           const std::vector<std::vector<double>>& X);
};

/// 性能基准测试工具
class PerformanceBenchmark {
public:
    /// 基准测试结果
    struct BenchmarkResult {
        std::string operation_name;           ///< 操作名称
        double avg_time_ns;                   ///< 平均时间(纳秒)
        double min_time_ns;                   ///< 最小时间(纳秒)
        double max_time_ns;                   ///< 最大时间(纳秒)
        double std_dev_ns;                    ///< 标准差(纳秒)
        double operations_per_second;         ///< 每秒操作数
        size_t iterations;                    ///< 迭代次数
    };

    /// 基准测试函数性能
    template<typename Func, typename... Args>
    BenchmarkResult benchmark_function(const std::string& name, Func&& func,
                                      size_t iterations, Args&&... args);

    /// 比较SIMD和标准实现
    void compare_simd_performance();

    /// 比较不同风险指标计算方法
    void compare_risk_calculations();

    /// 生成性能报告
    std::string generate_performance_report(const std::vector<BenchmarkResult>& results);

private:
    std::vector<BenchmarkResult> results_;
};

/// 实时性能监控器
class RealTimePerformanceMonitor {
public:
    /// 构造函数
    explicit RealTimePerformanceMonitor(std::shared_ptr<account::QA_Account> account);

    /// 析构函数
    ~RealTimePerformanceMonitor();

    /// 开始监控
    void start_monitoring();

    /// 停止监控
    void stop_monitoring();

    /// 获取实时指标
    PerformanceMetrics get_current_metrics() const;

    /// 设置指标更新回调
    void set_update_callback(std::function<void(const PerformanceMetrics&)> callback);

    /// 设置风险警告回调
    void set_risk_warning_callback(std::function<void(const std::string&, double)> callback);

    /// 设置风险阈值
    void set_risk_thresholds(double max_drawdown = 0.1, double min_sharpe = 0.5);

private:
    std::shared_ptr<account::QA_Account> account_;
    std::atomic<bool> monitoring_;
    std::thread monitor_thread_;

    std::function<void(const PerformanceMetrics&)> update_callback_;
    std::function<void(const std::string&, double)> risk_warning_callback_;

    double max_drawdown_threshold_;
    double min_sharpe_threshold_;

    mutable std::mutex metrics_mutex_;
    PerformanceMetrics current_metrics_;

    void monitoring_loop();
    void check_risk_thresholds(const PerformanceMetrics& metrics);
};

/// 多资产组合分析器
class PortfolioAnalyzer {
public:
    /// 构造函数
    PortfolioAnalyzer() = default;

    /// 设置资产收益率数据
    void set_asset_returns(const std::map<std::string, std::vector<double>>& asset_returns);

    /// 计算协方差矩阵
    std::vector<std::vector<double>> calculate_covariance_matrix();

    /// 有效前沿计算
    std::vector<std::pair<double, double>> calculate_efficient_frontier(int points = 100);

    /// 最优投资组合计算
    std::map<std::string, double> calculate_optimal_portfolio(double target_return);

    /// 风险平价投资组合
    std::map<std::string, double> calculate_risk_parity_portfolio();

    /// 最小方差投资组合
    std::map<std::string, double> calculate_minimum_variance_portfolio();

    /// 最大夏普比率投资组合
    std::map<std::string, double> calculate_maximum_sharpe_portfolio();

    /// 投资组合风险分解
    std::map<std::string, double> decompose_portfolio_risk(
        const std::map<std::string, double>& weights);

    /// Black-Litterman模型
    std::map<std::string, double> black_litterman_optimization(
        const std::map<std::string, double>& market_weights,
        const std::map<std::string, double>& views,
        const std::map<std::string, double>& view_confidence);

private:
    std::map<std::string, std::vector<double>> asset_returns_;
    std::vector<std::vector<double>> covariance_matrix_;
    std::vector<double> expected_returns_;

    /// 数值优化方法
    std::vector<double> solve_quadratic_programming(
        const std::vector<std::vector<double>>& Q,
        const std::vector<double>& c,
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b);
};

/// 工厂函数
namespace factory {
    /// 创建性能分析器
    std::unique_ptr<PerformanceAnalyzer> create_performance_analyzer();

    /// 创建风险计算器
    std::unique_ptr<RiskCalculator> create_risk_calculator();

    /// 创建归因分析器
    std::unique_ptr<AttributionAnalyzer> create_attribution_analyzer();

    /// 创建投资组合分析器
    std::unique_ptr<PortfolioAnalyzer> create_portfolio_analyzer();
}

} // namespace qaultra::analysis