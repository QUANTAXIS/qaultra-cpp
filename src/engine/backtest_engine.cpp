#include "qaultra/engine/backtest_engine.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include <sstream>
#include <iomanip>

namespace qaultra::engine {

// ==================== StrategyContext 简化实现 ====================

double StrategyContext::get_price(const std::string& /* symbol */) const {
    // 简化实现：返回默认价格
    return current_price > 0 ? current_price : 100.0;
}

std::vector<double> StrategyContext::get_history(const std::string& /* symbol */, int window, const std::string& /* field */) const {
    // 简化实现：生成模拟历史数据
    std::vector<double> history;
    double base_price = 100.0;

    for (int i = 0; i < window; ++i) {
        double noise = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.04; // ±2%的随机波动
        history.push_back(base_price * (1.0 + noise));
    }

    return history;
}

std::shared_ptr<account::Position> StrategyContext::get_position(const std::string& /* symbol */) const {
    // 简化实现：返回空指针（暂无持仓）
    return nullptr;
}

double StrategyContext::get_cash() const {
    return 1000000.0; // 简化实现：固定现金
}

double StrategyContext::get_portfolio_value() const {
    return 1000000.0; // 简化实现：固定组合价值
}

void StrategyContext::log(const std::string& message) const {
    std::cout << "[" << current_date << "] " << message << std::endl;
}

// ==================== SMAStrategy 实现 ====================

void SMAStrategy::initialize(StrategyContext& context) {
    context.log("初始化SMA策略 (快线:" + std::to_string(fast_window_) +
                ", 慢线:" + std::to_string(slow_window_) + ")");

    // 初始化持仓状态
    for (const auto& symbol : context.universe) {
        positions_[symbol] = false;
    }
}

void SMAStrategy::handle_data(StrategyContext& context) {
    for (const auto& symbol : context.universe) {
        auto fast_prices = context.get_history(symbol, fast_window_, "close");
        auto slow_prices = context.get_history(symbol, slow_window_, "close");

        if (fast_prices.size() < static_cast<size_t>(fast_window_) ||
            slow_prices.size() < static_cast<size_t>(slow_window_)) {
            continue; // 数据不足
        }

        // 计算移动平均线
        double fast_ma = std::accumulate(fast_prices.begin(), fast_prices.end(), 0.0) / fast_window_;
        double slow_ma = std::accumulate(slow_prices.begin(), slow_prices.end(), 0.0) / slow_window_;

        double current_price = context.get_price(symbol);
        bool has_position = positions_[symbol];

        // 交易逻辑：快线上穿慢线买入，下穿卖出
        if (fast_ma > slow_ma && !has_position && context.get_cash() > current_price * 100) {
            // 买入信号
            context.log("买入信号 " + symbol + " 快线MA=" + std::to_string(fast_ma) +
                       " 慢线MA=" + std::to_string(slow_ma));
            positions_[symbol] = true;
        } else if (fast_ma < slow_ma && has_position) {
            // 卖出信号
            context.log("卖出信号 " + symbol + " 快线MA=" + std::to_string(fast_ma) +
                       " 慢线MA=" + std::to_string(slow_ma));
            positions_[symbol] = false;
        }
    }
}

std::map<std::string, double> SMAStrategy::get_parameters() const {
    return {
        {"fast_window", static_cast<double>(fast_window_)},
        {"slow_window", static_cast<double>(slow_window_)}
    };
}

void SMAStrategy::set_parameter(const std::string& name, double value) {
    if (name == "fast_window") {
        fast_window_ = static_cast<int>(value);
    } else if (name == "slow_window") {
        slow_window_ = static_cast<int>(value);
    }
}

// ==================== MomentumStrategy 实现 ====================

void MomentumStrategy::initialize(StrategyContext& context) {
    context.log("初始化动量策略 (回望:" + std::to_string(lookback_window_) +
                ", 阈值:" + std::to_string(threshold_) + ")");

    for (const auto& symbol : context.universe) {
        price_history_[symbol].reserve(lookback_window_ + 10);
    }
}

void MomentumStrategy::handle_data(StrategyContext& context) {
    for (const auto& symbol : context.universe) {
        double current_price = context.get_price(symbol);

        // 更新价格历史
        auto& history = price_history_[symbol];
        history.push_back(current_price);

        if (history.size() > static_cast<size_t>(lookback_window_ + 1)) {
            history.erase(history.begin());
        }

        if (history.size() < static_cast<size_t>(lookback_window_ + 1)) {
            continue; // 数据不足
        }

        // 计算动量
        double past_price = history[0];
        double momentum = (current_price - past_price) / past_price;

        context.log("动量计算 " + symbol + " 当前价格=" + std::to_string(current_price) +
                   " 历史价格=" + std::to_string(past_price) + " 动量=" + std::to_string(momentum));
    }
}

std::map<std::string, double> MomentumStrategy::get_parameters() const {
    return {
        {"lookback_window", static_cast<double>(lookback_window_)},
        {"threshold", threshold_}
    };
}

void MomentumStrategy::set_parameter(const std::string& name, double value) {
    if (name == "lookback_window") {
        lookback_window_ = static_cast<int>(value);
    } else if (name == "threshold") {
        threshold_ = value;
    }
}

// ==================== MeanReversionStrategy 实现 ====================

void MeanReversionStrategy::initialize(StrategyContext& context) {
    context.log("初始化均值回归策略 (窗口:" + std::to_string(window_) +
                ", Z分数阈值:" + std::to_string(z_score_threshold_) + ")");

    for (const auto& symbol : context.universe) {
        price_buffer_[symbol].reserve(window_ + 10);
    }
}

void MeanReversionStrategy::handle_data(StrategyContext& context) {
    for (const auto& symbol : context.universe) {
        double current_price = context.get_price(symbol);

        auto& buffer = price_buffer_[symbol];
        buffer.push_back(current_price);

        if (buffer.size() > static_cast<size_t>(window_)) {
            buffer.erase(buffer.begin());
        }

        if (buffer.size() < static_cast<size_t>(window_)) {
            continue;
        }

        // 计算移动平均和标准差
        double mean = std::accumulate(buffer.begin(), buffer.end(), 0.0) / window_;

        double variance = 0.0;
        for (double price : buffer) {
            variance += (price - mean) * (price - mean);
        }
        variance /= window_;
        double std_dev = std::sqrt(variance);

        // 计算Z分数
        double z_score = (current_price - mean) / std_dev;

        context.log("均值回归 " + symbol + " Z分数=" + std::to_string(z_score) +
                   " 均值=" + std::to_string(mean) + " 标准差=" + std::to_string(std_dev));
    }
}

std::map<std::string, double> MeanReversionStrategy::get_parameters() const {
    return {
        {"window", static_cast<double>(window_)},
        {"z_score_threshold", z_score_threshold_}
    };
}

void MeanReversionStrategy::set_parameter(const std::string& name, double value) {
    if (name == "window") {
        window_ = static_cast<int>(value);
    } else if (name == "z_score_threshold") {
        z_score_threshold_ = value;
    }
}

// ==================== BacktestEngine 简化实现 ====================

BacktestEngine::BacktestEngine(const BacktestConfig& config) : config_(config) {
    std::cout << "初始化回测引擎..." << std::endl;
    std::cout << "配置 - 开始日期: " << config_.start_date << std::endl;
    std::cout << "配置 - 结束日期: " << config_.end_date << std::endl;
    std::cout << "配置 - 初始资金: " << config_.initial_cash << std::endl;
}

BacktestEngine::~BacktestEngine() {
    // 简化析构函数
}

void BacktestEngine::add_strategy(std::shared_ptr<Strategy> strategy) {
    strategies_.push_back(strategy);
    std::cout << "添加策略，当前策略数量: " << strategies_.size() << std::endl;
}

void BacktestEngine::set_universe(const std::vector<std::string>& symbols) {
    universe_ = symbols;
    std::cout << "设置股票池，包含 " << universe_.size() << " 只股票" << std::endl;
    for (const auto& symbol : universe_) {
        std::cout << "  - " << symbol << std::endl;
    }
}

bool BacktestEngine::load_data(const std::string& data_source) {
    std::cout << "加载数据源: " << (data_source.empty() ? "默认模拟数据" : data_source) << std::endl;

    // 简化实现：生成一些日期
    date_index_ = {"2024-01-01", "2024-01-02", "2024-01-03", "2024-01-04", "2024-01-05"};

    return true;
}

BacktestResults BacktestEngine::run() {
    if (strategies_.empty()) {
        throw std::runtime_error("没有添加任何策略");
    }

    is_running_ = true;
    std::cout << "开始回测..." << std::endl;
    std::cout << "初始资金: " << config_.initial_cash << std::endl;
    std::cout << "策略数量: " << strategies_.size() << std::endl;
    std::cout << "股票池大小: " << universe_.size() << std::endl;

    // 初始化策略上下文
    StrategyContext context;
    context.universe = universe_;

    // 初始化所有策略
    for (auto& strategy : strategies_) {
        strategy->initialize(context);
    }

    // 逐日运行回测
    for (size_t day = 0; day < date_index_.size(); ++day) {
        if (!is_running_) break;

        current_index_ = day;
        current_date_ = date_index_[day];
        context.current_date = current_date_;
        context.current_price = 100.0 + day * 0.5; // 简单的价格趋势

        std::cout << "\n=== 交易日: " << current_date_ << " ===" << std::endl;

        // 执行策略
        for (auto& strategy : strategies_) {
            try {
                strategy->handle_data(context);
            } catch (const std::exception& e) {
                std::cerr << "策略执行错误: " << e.what() << std::endl;
            }
        }

        // 记录每日表现
        daily_equity_.push_back(config_.initial_cash * (1.0 + day * 0.001)); // 简单增长模式
    }

    // 计算性能指标
    calculate_performance_metrics();

    std::cout << "\n回测完成!" << std::endl;
    std::cout << "最终资产: " << results_.final_value << std::endl;
    std::cout << "总收益率: " << (results_.total_return * 100) << "%" << std::endl;
    std::cout << "夏普比率: " << results_.sharpe_ratio << std::endl;
    std::cout << "最大回撤: " << (results_.max_drawdown * 100) << "%" << std::endl;

    is_running_ = false;
    return results_;
}

void BacktestEngine::calculate_performance_metrics() {
    if (daily_equity_.size() < 2) {
        return;
    }

    results_.final_value = daily_equity_.back();
    results_.total_return = (results_.final_value - config_.initial_cash) / config_.initial_cash;
    results_.annual_return = calculate_annual_return();
    results_.sharpe_ratio = calculate_sharpe_ratio();
    results_.max_drawdown = calculate_max_drawdown();
    results_.volatility = calculate_volatility();

    results_.total_trades = 10; // 简化：假设有10笔交易
    results_.win_rate = 0.6;    // 简化：假设60%胜率
    results_.profit_factor = 1.5; // 简化：假设盈亏比1.5

    // 复制资产曲线
    results_.equity_curve = daily_equity_;

    // 计算每日收益率
    for (size_t i = 1; i < daily_equity_.size(); ++i) {
        double daily_return = (daily_equity_[i] - daily_equity_[i-1]) / daily_equity_[i-1];
        results_.daily_returns.push_back(daily_return);
    }
}

double BacktestEngine::calculate_sharpe_ratio() const {
    auto returns = calculate_returns_simd(daily_equity_);
    return utils::calculate_sharpe_ratio(returns, 0.0);
}

double BacktestEngine::calculate_max_drawdown() const {
    return utils::calculate_max_drawdown(daily_equity_);
}

double BacktestEngine::calculate_annual_return() const {
    if (daily_equity_.size() < 2) return 0.0;

    double total_return = (daily_equity_.back() - daily_equity_.front()) / daily_equity_.front();
    double days = static_cast<double>(daily_equity_.size() - 1);
    return std::pow(1.0 + total_return, 252.0 / days) - 1.0; // 年化
}

double BacktestEngine::calculate_volatility() const {
    auto returns = calculate_returns_simd(daily_equity_);
    return calculate_volatility_simd(returns);
}

std::vector<double> BacktestEngine::calculate_returns_simd(const std::vector<double>& prices) const {
    if (prices.size() < 2) return {};

    std::vector<double> returns;
    returns.reserve(prices.size() - 1);

    for (size_t i = 1; i < prices.size(); ++i) {
        returns.push_back((prices[i] - prices[i-1]) / prices[i-1]);
    }

    return returns;
}

double BacktestEngine::calculate_volatility_simd(const std::vector<double>& returns) const {
    if (returns.empty()) return 0.0;

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    variance /= returns.size();

    return std::sqrt(variance) * std::sqrt(252.0); // 年化波动率
}

bool BacktestEngine::save_results(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "总收益率," << results_.total_return << std::endl;
    file << "年化收益率," << results_.annual_return << std::endl;
    file << "夏普比率," << results_.sharpe_ratio << std::endl;
    file << "最大回撤," << results_.max_drawdown << std::endl;
    file << "波动率," << results_.volatility << std::endl;
    file << "胜率," << results_.win_rate << std::endl;
    file << "盈亏比," << results_.profit_factor << std::endl;
    file << "交易次数," << results_.total_trades << std::endl;
    file << "最终价值," << results_.final_value << std::endl;

    return true;
}

std::map<std::string, double> BacktestEngine::get_performance_summary() const {
    return {
        {"总收益率", results_.total_return},
        {"年化收益率", results_.annual_return},
        {"夏普比率", results_.sharpe_ratio},
        {"最大回撤", results_.max_drawdown},
        {"波动率", results_.volatility},
        {"胜率", results_.win_rate},
        {"盈亏比", results_.profit_factor},
        {"交易次数", static_cast<double>(results_.total_trades)},
        {"最终价值", results_.final_value}
    };
}

std::vector<std::pair<std::string, double>> BacktestEngine::plot_equity_curve() const {
    std::vector<std::pair<std::string, double>> curve_data;

    for (size_t i = 0; i < std::min(date_index_.size(), results_.equity_curve.size()); ++i) {
        curve_data.emplace_back(date_index_[i], results_.equity_curve[i]);
    }

    return curve_data;
}

std::map<std::string, std::vector<double>> BacktestEngine::get_trade_analysis() const {
    std::map<std::string, std::vector<double>> analysis;

    // 简化的交易分析数据
    analysis["trade_amounts"] = {10000, 15000, 12000, 8000, 20000};
    analysis["trade_prices"] = {100.5, 101.2, 99.8, 102.1, 98.5};
    analysis["commissions"] = {3.0, 4.5, 3.6, 2.4, 6.0};

    return analysis;
}

// ==================== Utils ====================

namespace utils {

double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double excess_return = mean_return - risk_free_rate / 252.0; // 日化

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean_return) * (ret - mean_return);
    }
    variance /= returns.size();

    double std_dev = std::sqrt(variance);
    return (std_dev > 0) ? excess_return / std_dev * std::sqrt(252.0) : 0.0;
}

double calculate_max_drawdown(const std::vector<double>& equity_curve) {
    if (equity_curve.empty()) return 0.0;

    double max_value = equity_curve[0];
    double max_drawdown = 0.0;

    for (double value : equity_curve) {
        if (value > max_value) {
            max_value = value;
        }

        double drawdown = (max_value - value) / max_value;
        if (drawdown > max_drawdown) {
            max_drawdown = drawdown;
        }
    }

    return max_drawdown;
}

double calculate_annual_return(const std::vector<double>& equity_curve, int trading_days) {
    if (equity_curve.size() < 2) return 0.0;

    double total_return = (equity_curve.back() - equity_curve.front()) / equity_curve.front();
    double days = static_cast<double>(equity_curve.size() - 1);

    return std::pow(1.0 + total_return, static_cast<double>(trading_days) / days) - 1.0;
}

double calculate_volatility(const std::vector<double>& returns, bool annualized) {
    if (returns.empty()) return 0.0;

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    variance /= returns.size();

    double volatility = std::sqrt(variance);
    return annualized ? volatility * std::sqrt(252.0) : volatility;
}

double calculate_win_rate(const std::vector<double>& trade_returns) {
    if (trade_returns.empty()) return 0.0;

    int winning_trades = std::count_if(trade_returns.begin(), trade_returns.end(),
                                      [](double ret) { return ret > 0; });

    return static_cast<double>(winning_trades) / trade_returns.size();
}

double calculate_profit_factor(const std::vector<double>& trade_returns) {
    if (trade_returns.empty()) return 0.0;

    double total_profit = 0.0;
    double total_loss = 0.0;

    for (double ret : trade_returns) {
        if (ret > 0) {
            total_profit += ret;
        } else {
            total_loss += std::abs(ret);
        }
    }

    return total_loss > 0 ? total_profit / total_loss : 0.0;
}

std::vector<double> calculate_rolling_sharpe(const std::vector<double>& returns, int window) {
    std::vector<double> rolling_sharpe;

    if (returns.size() < static_cast<size_t>(window)) {
        return rolling_sharpe;
    }

    for (size_t i = window - 1; i < returns.size(); ++i) {
        std::vector<double> window_returns(returns.begin() + i - window + 1,
                                          returns.begin() + i + 1);
        double sharpe = calculate_sharpe_ratio(window_returns);
        rolling_sharpe.push_back(sharpe);
    }

    return rolling_sharpe;
}

double calculate_beta(const std::vector<double>& strategy_returns,
                     const std::vector<double>& benchmark_returns) {
    if (strategy_returns.size() != benchmark_returns.size() || strategy_returns.empty()) {
        return 0.0;
    }

    double strategy_mean = std::accumulate(strategy_returns.begin(), strategy_returns.end(), 0.0)
                          / strategy_returns.size();
    double benchmark_mean = std::accumulate(benchmark_returns.begin(), benchmark_returns.end(), 0.0)
                           / benchmark_returns.size();

    double covariance = 0.0;
    double benchmark_variance = 0.0;

    for (size_t i = 0; i < strategy_returns.size(); ++i) {
        double strategy_diff = strategy_returns[i] - strategy_mean;
        double benchmark_diff = benchmark_returns[i] - benchmark_mean;
        covariance += strategy_diff * benchmark_diff;
        benchmark_variance += benchmark_diff * benchmark_diff;
    }

    return benchmark_variance > 0 ? covariance / benchmark_variance : 0.0;
}

double calculate_alpha(const std::vector<double>& strategy_returns,
                      const std::vector<double>& benchmark_returns,
                      double risk_free_rate) {
    if (strategy_returns.size() != benchmark_returns.size() || strategy_returns.empty()) {
        return 0.0;
    }

    double beta = calculate_beta(strategy_returns, benchmark_returns);

    double strategy_mean = std::accumulate(strategy_returns.begin(), strategy_returns.end(), 0.0)
                          / strategy_returns.size();
    double benchmark_mean = std::accumulate(benchmark_returns.begin(), benchmark_returns.end(), 0.0)
                           / benchmark_returns.size();

    return strategy_mean - (risk_free_rate + beta * (benchmark_mean - risk_free_rate));
}

} // namespace utils

// ==================== Factory ====================

namespace factory {

std::shared_ptr<Strategy> create_sma_strategy(int fast_window, int slow_window) {
    return std::make_shared<SMAStrategy>(fast_window, slow_window);
}

std::shared_ptr<Strategy> create_momentum_strategy(int lookback_window, double threshold) {
    return std::make_shared<MomentumStrategy>(lookback_window, threshold);
}

std::shared_ptr<Strategy> create_mean_reversion_strategy(int window, double z_score_threshold) {
    return std::make_shared<MeanReversionStrategy>(window, z_score_threshold);
}

} // namespace factory

} // namespace qaultra::engine