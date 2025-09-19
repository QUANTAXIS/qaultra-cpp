#include "qaultra/analysis/performance_analyzer.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

namespace qaultra::analysis {

// ==================== TradePair 实现 ====================

TradePair::TradePair(const std::string& code, double amount, double open_price,
                     const std::string& open_date, bool is_buy_open)
    : code(code), open_date(open_date), is_buy_open(is_buy_open), amount(amount),
      open_price(open_price), close_price(0.0), pnl_ratio(0.0), pnl_money(0.0),
      hold_gap_days(0.0), commission(0.0) {
}

void TradePair::close_position(double close_price, const std::string& close_date,
                               const std::string& close_trade_id) {
    this->close_price = close_price;
    this->close_date = close_date;
    this->close_trade_id = close_trade_id;

    // 计算盈亏
    if (is_buy_open) {
        pnl_money = amount * (close_price - open_price);
    } else {
        pnl_money = amount * (open_price - close_price);
    }

    // 计算盈亏比例
    if (open_price > 0) {
        pnl_ratio = pnl_money / (amount * open_price);
    }

    // 计算持仓天数（简化实现）
    // 这里需要解析日期字符串，简化为固定值
    hold_gap_days = 1.0; // 占位符
}

double TradePair::get_return_rate() const {
    return pnl_ratio;
}

double TradePair::get_annual_return() const {
    if (hold_gap_days <= 0) return 0.0;
    return pnl_ratio * (365.0 / hold_gap_days);
}

bool TradePair::is_profitable() const {
    return pnl_money > 0.0;
}

nlohmann::json TradePair::to_json() const {
    nlohmann::json j;
    j["code"] = code;
    j["open_date"] = open_date;
    j["close_date"] = close_date;
    j["is_buy_open"] = is_buy_open;
    j["amount"] = amount;
    j["open_price"] = open_price;
    j["close_price"] = close_price;
    j["pnl_ratio"] = pnl_ratio;
    j["pnl_money"] = pnl_money;
    j["hold_gap_days"] = hold_gap_days;
    j["commission"] = commission;
    return j;
}

// ==================== RiskMetrics 实现 ====================

nlohmann::json RiskMetrics::to_json() const {
    nlohmann::json j;
    j["annual_return"] = annual_return;
    j["total_return"] = total_return;
    j["max_drawdown"] = max_drawdown;
    j["sharpe_ratio"] = sharpe_ratio;
    j["sortino_ratio"] = sortino_ratio;
    j["calmar_ratio"] = calmar_ratio;
    j["omega_ratio"] = omega_ratio;
    j["annual_volatility"] = annual_volatility;
    j["downside_risk"] = downside_risk;
    j["alpha"] = alpha;
    j["beta"] = beta;
    j["value_at_risk_95"] = value_at_risk_95;
    j["conditional_var_95"] = conditional_var_95;
    j["tail_ratio"] = tail_ratio;
    j["skew"] = skew;
    j["kurtosis"] = kurtosis;
    j["total_trades"] = total_trades;
    j["profitable_trades"] = profitable_trades;
    j["loss_trades"] = loss_trades;
    j["win_rate"] = win_rate;
    j["profit_loss_ratio"] = profit_loss_ratio;
    j["average_win"] = average_win;
    j["average_loss"] = average_loss;
    j["largest_win"] = largest_win;
    j["largest_loss"] = largest_loss;
    j["average_holding_period"] = average_holding_period;
    return j;
}

void RiskMetrics::print_report() const {
    std::cout << "\n===== 风险指标报告 =====" << std::endl;
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "总收益率: " << (total_return * 100) << "%" << std::endl;
    std::cout << "年化收益率: " << (annual_return * 100) << "%" << std::endl;
    std::cout << "最大回撤: " << (max_drawdown * 100) << "%" << std::endl;
    std::cout << "夏普比率: " << sharpe_ratio << std::endl;
    std::cout << "索提诺比率: " << sortino_ratio << std::endl;
    std::cout << "年化波动率: " << (annual_volatility * 100) << "%" << std::endl;
    std::cout << "Alpha: " << alpha << std::endl;
    std::cout << "Beta: " << beta << std::endl;
    std::cout << "95% VaR: " << (value_at_risk_95 * 100) << "%" << std::endl;
    std::cout << "偏度: " << skew << std::endl;
    std::cout << "峰度: " << kurtosis << std::endl;
    std::cout << "\n===== 交易统计 =====" << std::endl;
    std::cout << "总交易次数: " << total_trades << std::endl;
    std::cout << "盈利交易: " << profitable_trades << std::endl;
    std::cout << "亏损交易: " << loss_trades << std::endl;
    std::cout << "胜率: " << (win_rate * 100) << "%" << std::endl;
    std::cout << "盈亏比: " << profit_loss_ratio << std::endl;
    std::cout << "平均盈利: " << average_win << std::endl;
    std::cout << "平均亏损: " << average_loss << std::endl;
    std::cout << "最大盈利: " << largest_win << std::endl;
    std::cout << "最大亏损: " << largest_loss << std::endl;
    std::cout << "======================\n" << std::endl;
}

// ==================== PerformanceReport 实现 ====================

nlohmann::json PerformanceReport::to_json() const {
    nlohmann::json j;
    j["account_id"] = account_id;
    j["start_date"] = start_date;
    j["end_date"] = end_date;
    j["benchmark"] = benchmark;
    j["metrics"] = metrics.to_json();

    j["trade_pairs"] = nlohmann::json::array();
    for (const auto& trade : trade_pairs) {
        j["trade_pairs"].push_back(trade.to_json());
    }

    j["daily_returns"] = daily_returns;
    j["cumulative_returns"] = cumulative_returns;
    j["benchmark_returns"] = benchmark_returns;
    j["dates"] = dates;

    j["by_asset"] = nlohmann::json::object();
    for (const auto& [asset, asset_metrics] : by_asset) {
        j["by_asset"][asset] = asset_metrics.to_json();
    }

    j["sector_exposure"] = sector_exposure;
    j["style_exposure"] = style_exposure;

    return j;
}

bool PerformanceReport::save_to_file(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    try {
        auto json_data = to_json();
        file << json_data.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存报告失败: " << e.what() << std::endl;
        return false;
    }
}

// ==================== SingleAssetPerformanceAnalyzer 实现 ====================

SingleAssetPerformanceAnalyzer::SingleAssetPerformanceAnalyzer(const std::string& asset_code)
    : asset_code_(asset_code) {
}

void SingleAssetPerformanceAnalyzer::add_trade_pair(const TradePair& trade_pair) {
    std::lock_guard<std::mutex> lock(mutex_);
    trade_pairs_.push_back(trade_pair);
}

RiskMetrics SingleAssetPerformanceAnalyzer::calculate_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    RiskMetrics metrics;

    if (trade_pairs_.empty()) {
        return metrics;
    }

    // 基础交易统计
    metrics.total_trades = trade_pairs_.size();

    double total_pnl = 0.0;
    std::vector<double> returns;
    std::vector<double> profitable_trades;
    std::vector<double> loss_trades;
    double total_holding_days = 0.0;

    for (const auto& trade : trade_pairs_) {
        total_pnl += trade.pnl_money;
        returns.push_back(trade.get_return_rate());
        total_holding_days += trade.hold_gap_days;

        if (trade.is_profitable()) {
            profitable_trades.push_back(trade.pnl_money);
            metrics.profitable_trades++;
        } else {
            loss_trades.push_back(std::abs(trade.pnl_money));
            metrics.loss_trades++;
        }

        metrics.largest_win = std::max(metrics.largest_win, trade.pnl_money);
        metrics.largest_loss = std::min(metrics.largest_loss, trade.pnl_money);
    }

    // 计算交易相关指标
    metrics.win_rate = static_cast<double>(metrics.profitable_trades) / metrics.total_trades;
    metrics.average_holding_period = total_holding_days / metrics.total_trades;

    if (!profitable_trades.empty()) {
        metrics.average_win = std::accumulate(profitable_trades.begin(), profitable_trades.end(), 0.0)
                             / profitable_trades.size();
    }

    if (!loss_trades.empty()) {
        metrics.average_loss = std::accumulate(loss_trades.begin(), loss_trades.end(), 0.0)
                              / loss_trades.size();
    }

    if (metrics.average_loss > 0) {
        metrics.profit_loss_ratio = metrics.average_win / metrics.average_loss;
    }

    // 计算收益率相关指标
    if (!returns.empty()) {
        metrics.total_return = RiskCalculator::calculate_return(returns);
        metrics.annual_return = RiskCalculator::calculate_annual_return(returns);
        metrics.annual_volatility = RiskCalculator::calculate_volatility(returns);
        metrics.sharpe_ratio = RiskCalculator::calculate_sharpe_ratio(returns);
        metrics.sortino_ratio = RiskCalculator::calculate_sortino_ratio(returns);
        metrics.max_drawdown = RiskCalculator::calculate_max_drawdown(returns);
        metrics.value_at_risk_95 = RiskCalculator::calculate_value_at_risk(returns);
        metrics.conditional_var_95 = RiskCalculator::calculate_conditional_var(returns);
        metrics.skew = RiskCalculator::calculate_skewness(returns);
        metrics.kurtosis = RiskCalculator::calculate_kurtosis(returns);
    }

    return metrics;
}

double SingleAssetPerformanceAnalyzer::get_total_pnl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::accumulate(trade_pairs_.begin(), trade_pairs_.end(), 0.0,
                          [](double sum, const TradePair& trade) {
                              return sum + trade.pnl_money;
                          });
}

double SingleAssetPerformanceAnalyzer::get_total_return() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (trade_pairs_.empty()) return 0.0;

    double total_invested = 0.0;
    double total_pnl = 0.0;

    for (const auto& trade : trade_pairs_) {
        total_invested += trade.amount * trade.open_price;
        total_pnl += trade.pnl_money;
    }

    return total_invested > 0 ? total_pnl / total_invested : 0.0;
}

// ==================== PortfolioPerformanceAnalyzer 实现 ====================

PortfolioPerformanceAnalyzer::PortfolioPerformanceAnalyzer(const std::string& portfolio_id)
    : portfolio_id_(portfolio_id) {
}

void PortfolioPerformanceAnalyzer::set_benchmark(const std::vector<double>& benchmark_returns,
                                                const std::vector<std::string>& dates) {
    std::lock_guard<std::mutex> lock(mutex_);
    benchmark_returns_ = benchmark_returns;
    benchmark_dates_ = dates;
}

void PortfolioPerformanceAnalyzer::add_account_curve(const std::vector<double>& account_values,
                                                    const std::vector<std::string>& dates) {
    std::lock_guard<std::mutex> lock(mutex_);
    account_values_ = account_values;
    dates_ = dates;
}

void PortfolioPerformanceAnalyzer::add_trade_pair(const TradePair& trade_pair) {
    std::lock_guard<std::mutex> lock(mutex_);
    trade_pairs_.push_back(trade_pair);
    trade_pairs_by_asset_[trade_pair.code].push_back(trade_pair);
}

void PortfolioPerformanceAnalyzer::add_trade_pairs(const std::vector<TradePair>& trade_pairs) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& trade : trade_pairs) {
        trade_pairs_.push_back(trade);
        trade_pairs_by_asset_[trade.code].push_back(trade);
    }
}

RiskMetrics PortfolioPerformanceAnalyzer::calculate_portfolio_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    RiskMetrics metrics;

    // 计算组合收益率
    auto portfolio_returns = calculate_daily_returns();

    if (portfolio_returns.empty()) {
        return metrics;
    }

    // 基础收益指标
    metrics.total_return = RiskCalculator::calculate_return(account_values_);
    metrics.annual_return = RiskCalculator::calculate_annual_return(portfolio_returns);
    metrics.annual_volatility = RiskCalculator::calculate_volatility(portfolio_returns);
    metrics.max_drawdown = RiskCalculator::calculate_max_drawdown(calculate_cumulative_returns());

    // 风险比率
    metrics.sharpe_ratio = RiskCalculator::calculate_sharpe_ratio(portfolio_returns);
    metrics.sortino_ratio = RiskCalculator::calculate_sortino_ratio(portfolio_returns);
    metrics.calmar_ratio = RiskCalculator::calculate_calmar_ratio(portfolio_returns);
    metrics.omega_ratio = RiskCalculator::calculate_omega_ratio(portfolio_returns);

    // 下行风险
    metrics.downside_risk = RiskCalculator::calculate_downside_risk(portfolio_returns);
    metrics.value_at_risk_95 = RiskCalculator::calculate_value_at_risk(portfolio_returns);
    metrics.conditional_var_95 = RiskCalculator::calculate_conditional_var(portfolio_returns);

    // 高阶矩
    metrics.skew = RiskCalculator::calculate_skewness(portfolio_returns);
    metrics.kurtosis = RiskCalculator::calculate_kurtosis(portfolio_returns);
    metrics.tail_ratio = RiskCalculator::calculate_tail_ratio(portfolio_returns);

    // 相对指标（如果有基准）
    if (!benchmark_returns_.empty() && benchmark_returns_.size() == portfolio_returns.size()) {
        metrics.alpha = RiskCalculator::calculate_alpha(portfolio_returns, benchmark_returns_);
        metrics.beta = RiskCalculator::calculate_beta(portfolio_returns, benchmark_returns_);
    }

    // 交易统计
    metrics.total_trades = trade_pairs_.size();
    for (const auto& trade : trade_pairs_) {
        if (trade.is_profitable()) {
            metrics.profitable_trades++;
        } else {
            metrics.loss_trades++;
        }
    }

    if (metrics.total_trades > 0) {
        metrics.win_rate = static_cast<double>(metrics.profitable_trades) / metrics.total_trades;
    }

    return metrics;
}

std::unordered_map<std::string, RiskMetrics> PortfolioPerformanceAnalyzer::calculate_by_asset_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::unordered_map<std::string, RiskMetrics> results;

    for (const auto& [asset, trades] : trade_pairs_by_asset_) {
        SingleAssetPerformanceAnalyzer analyzer(asset);
        for (const auto& trade : trades) {
            analyzer.add_trade_pair(trade);
        }
        results[asset] = analyzer.calculate_metrics();
    }

    return results;
}

PerformanceReport PortfolioPerformanceAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(mutex_);

    PerformanceReport report;
    report.account_id = portfolio_id_;

    if (!dates_.empty()) {
        report.start_date = dates_.front();
        report.end_date = dates_.back();
    }

    report.metrics = calculate_portfolio_metrics();
    report.trade_pairs = trade_pairs_;
    report.daily_returns = calculate_daily_returns();
    report.cumulative_returns = calculate_cumulative_returns();
    report.benchmark_returns = benchmark_returns_;
    report.dates = dates_;
    report.by_asset = calculate_by_asset_metrics();

    return report;
}

std::vector<double> PortfolioPerformanceAnalyzer::calculate_daily_returns() const {
    if (account_values_.size() < 2) {
        return {};
    }

    std::vector<double> returns;
    returns.reserve(account_values_.size() - 1);

    for (size_t i = 1; i < account_values_.size(); ++i) {
        double ret = (account_values_[i] - account_values_[i-1]) / account_values_[i-1];
        returns.push_back(ret);
    }

    return returns;
}

std::vector<double> PortfolioPerformanceAnalyzer::calculate_cumulative_returns() const {
    auto daily_returns = calculate_daily_returns();
    if (daily_returns.empty()) {
        return {};
    }

    std::vector<double> cumulative;
    cumulative.reserve(daily_returns.size() + 1);
    cumulative.push_back(1.0); // 初始值为1

    for (double ret : daily_returns) {
        cumulative.push_back(cumulative.back() * (1.0 + ret));
    }

    return cumulative;
}

double PortfolioPerformanceAnalyzer::calculate_tracking_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto portfolio_returns = calculate_daily_returns();
    return RiskCalculator::calculate_tracking_error(portfolio_returns, benchmark_returns_);
}

double PortfolioPerformanceAnalyzer::calculate_information_ratio() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto portfolio_returns = calculate_daily_returns();
    return RiskCalculator::calculate_information_ratio(portfolio_returns, benchmark_returns_);
}

std::vector<double> PortfolioPerformanceAnalyzer::calculate_excess_returns() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto portfolio_returns = calculate_daily_returns();
    return RiskCalculator::calculate_excess_returns(portfolio_returns, benchmark_returns_);
}

// ==================== RiskCalculator 实现 ====================

double RiskCalculator::calculate_return(const std::vector<double>& values) {
    if (values.size() < 2) return 0.0;
    return (values.back() - values.front()) / values.front();
}

double RiskCalculator::calculate_annual_return(const std::vector<double>& returns, int trading_days) {
    if (returns.empty()) return 0.0;

    double cumulative_return = 1.0;
    for (double ret : returns) {
        cumulative_return *= (1.0 + ret);
    }
    cumulative_return -= 1.0;

    double days = static_cast<double>(returns.size());
    return std::pow(1.0 + cumulative_return, static_cast<double>(trading_days) / days) - 1.0;
}

double RiskCalculator::calculate_volatility(const std::vector<double>& returns, bool annualized) {
    if (returns.empty()) return 0.0;

    double mean = calculate_mean(returns);
    double variance = calculate_variance(returns, mean);
    double volatility = std::sqrt(variance);

    return annualized ? volatility * std::sqrt(252.0) : volatility;
}

double RiskCalculator::calculate_max_drawdown(const std::vector<double>& cumulative_returns) {
    if (cumulative_returns.empty()) return 0.0;

    double max_value = cumulative_returns[0];
    double max_drawdown = 0.0;

    for (double value : cumulative_returns) {
        if (value > max_value) {
            max_value = value;
        }

        double drawdown = (max_value - value) / max_value;
        max_drawdown = std::max(max_drawdown, drawdown);
    }

    return max_drawdown;
}

double RiskCalculator::calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = calculate_mean(returns);
    double excess_return = mean_return - risk_free_rate / 252.0; // 日化无风险利率
    double volatility = calculate_volatility(returns, false);

    return volatility > 0 ? excess_return / volatility * std::sqrt(252.0) : 0.0;
}

double RiskCalculator::calculate_sortino_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = calculate_mean(returns);
    double excess_return = mean_return - risk_free_rate / 252.0;
    double downside_risk = calculate_downside_risk(returns);

    return downside_risk > 0 ? excess_return / downside_risk * std::sqrt(252.0) : 0.0;
}

double RiskCalculator::calculate_downside_risk(const std::vector<double>& returns, double threshold) {
    if (returns.empty()) return 0.0;

    double sum_squared_negative = 0.0;
    int negative_count = 0;

    for (double ret : returns) {
        if (ret < threshold) {
            double deviation = ret - threshold;
            sum_squared_negative += deviation * deviation;
            negative_count++;
        }
    }

    return negative_count > 0 ? std::sqrt(sum_squared_negative / negative_count) : 0.0;
}

double RiskCalculator::calculate_value_at_risk(const std::vector<double>& returns, double confidence) {
    if (returns.empty()) return 0.0;

    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());

    size_t index = static_cast<size_t>(sorted_returns.size() * confidence);
    if (index >= sorted_returns.size()) index = sorted_returns.size() - 1;

    return -sorted_returns[index]; // VaR为正值
}

double RiskCalculator::calculate_conditional_var(const std::vector<double>& returns, double confidence) {
    if (returns.empty()) return 0.0;

    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());

    size_t cutoff_index = static_cast<size_t>(sorted_returns.size() * confidence);
    if (cutoff_index == 0) cutoff_index = 1;

    double sum = 0.0;
    for (size_t i = 0; i < cutoff_index; ++i) {
        sum += sorted_returns[i];
    }

    return -sum / cutoff_index; // CVaR为正值
}

double RiskCalculator::calculate_skewness(const std::vector<double>& returns) {
    if (returns.size() < 3) return 0.0;

    double mean = calculate_mean(returns);
    double variance = calculate_variance(returns, mean);
    double std_dev = std::sqrt(variance);

    if (std_dev == 0) return 0.0;

    double sum_cubed_deviations = 0.0;
    for (double ret : returns) {
        double deviation = (ret - mean) / std_dev;
        sum_cubed_deviations += deviation * deviation * deviation;
    }

    return sum_cubed_deviations / returns.size();
}

double RiskCalculator::calculate_kurtosis(const std::vector<double>& returns) {
    if (returns.size() < 4) return 0.0;

    double mean = calculate_mean(returns);
    double variance = calculate_variance(returns, mean);
    double std_dev = std::sqrt(variance);

    if (std_dev == 0) return 0.0;

    double sum_fourth_deviations = 0.0;
    for (double ret : returns) {
        double deviation = (ret - mean) / std_dev;
        double dev_squared = deviation * deviation;
        sum_fourth_deviations += dev_squared * dev_squared;
    }

    return sum_fourth_deviations / returns.size() - 3.0; // 超额峰度
}

double RiskCalculator::calculate_calmar_ratio(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;

    double annual_return = calculate_annual_return(returns);

    // 计算累计收益率用于最大回撤计算
    std::vector<double> cumulative_returns;
    cumulative_returns.push_back(1.0);

    for (double ret : returns) {
        cumulative_returns.push_back(cumulative_returns.back() * (1.0 + ret));
    }

    double max_dd = calculate_max_drawdown(cumulative_returns);

    return max_dd > 0 ? annual_return / max_dd : 0.0;
}

double RiskCalculator::calculate_omega_ratio(const std::vector<double>& returns, double threshold) {
    if (returns.empty()) return 0.0;

    double gains = 0.0;
    double losses = 0.0;

    for (double ret : returns) {
        if (ret > threshold) {
            gains += (ret - threshold);
        } else {
            losses += (threshold - ret);
        }
    }

    return losses > 0 ? gains / losses : 0.0;
}

double RiskCalculator::calculate_alpha(const std::vector<double>& portfolio_returns,
                                     const std::vector<double>& benchmark_returns,
                                     double risk_free_rate) {
    if (portfolio_returns.size() != benchmark_returns.size() || portfolio_returns.empty()) {
        return 0.0;
    }

    double beta = calculate_beta(portfolio_returns, benchmark_returns);
    double portfolio_mean = calculate_mean(portfolio_returns);
    double benchmark_mean = calculate_mean(benchmark_returns);

    return portfolio_mean - (risk_free_rate / 252.0 + beta * (benchmark_mean - risk_free_rate / 252.0));
}

double RiskCalculator::calculate_beta(const std::vector<double>& portfolio_returns,
                                    const std::vector<double>& benchmark_returns) {
    if (portfolio_returns.size() != benchmark_returns.size() || portfolio_returns.empty()) {
        return 0.0;
    }

    double covariance = calculate_covariance(portfolio_returns, benchmark_returns);
    double benchmark_mean = calculate_mean(benchmark_returns);
    double benchmark_variance = calculate_variance(benchmark_returns, benchmark_mean);

    return benchmark_variance > 0 ? covariance / benchmark_variance : 0.0;
}

double RiskCalculator::calculate_tracking_error(const std::vector<double>& portfolio_returns,
                                              const std::vector<double>& benchmark_returns) {
    auto excess_returns = calculate_excess_returns(portfolio_returns, benchmark_returns);
    return calculate_volatility(excess_returns, true); // 年化跟踪误差
}

double RiskCalculator::calculate_information_ratio(const std::vector<double>& portfolio_returns,
                                                 const std::vector<double>& benchmark_returns) {
    auto excess_returns = calculate_excess_returns(portfolio_returns, benchmark_returns);
    if (excess_returns.empty()) return 0.0;

    double excess_mean = calculate_mean(excess_returns);
    double tracking_error = calculate_volatility(excess_returns, false);

    return tracking_error > 0 ? excess_mean / tracking_error * std::sqrt(252.0) : 0.0;
}

double RiskCalculator::calculate_tail_ratio(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;

    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());

    // 计算95%分位数和5%分位数
    size_t index_95 = static_cast<size_t>(sorted_returns.size() * 0.95);
    size_t index_5 = static_cast<size_t>(sorted_returns.size() * 0.05);

    if (index_95 >= sorted_returns.size()) index_95 = sorted_returns.size() - 1;
    if (index_5 >= sorted_returns.size()) index_5 = sorted_returns.size() - 1;

    double percentile_95 = sorted_returns[index_95];
    double percentile_5 = sorted_returns[index_5];

    return percentile_5 < 0 ? percentile_95 / std::abs(percentile_5) : 0.0;
}

std::vector<double> RiskCalculator::calculate_rolling_sharpe(const std::vector<double>& returns, int window) {
    std::vector<double> rolling_sharpe;

    if (returns.size() < static_cast<size_t>(window)) {
        return rolling_sharpe;
    }

    for (size_t i = window - 1; i < returns.size(); ++i) {
        std::vector<double> window_returns(returns.begin() + i - window + 1, returns.begin() + i + 1);
        double sharpe = calculate_sharpe_ratio(window_returns);
        rolling_sharpe.push_back(sharpe);
    }

    return rolling_sharpe;
}

std::vector<double> RiskCalculator::calculate_rolling_volatility(const std::vector<double>& returns, int window) {
    std::vector<double> rolling_vol;

    if (returns.size() < static_cast<size_t>(window)) {
        return rolling_vol;
    }

    for (size_t i = window - 1; i < returns.size(); ++i) {
        std::vector<double> window_returns(returns.begin() + i - window + 1, returns.begin() + i + 1);
        double vol = calculate_volatility(window_returns, true);
        rolling_vol.push_back(vol);
    }

    return rolling_vol;
}

std::vector<double> RiskCalculator::calculate_rolling_max_drawdown(const std::vector<double>& cumulative_returns, int window) {
    std::vector<double> rolling_dd;

    if (cumulative_returns.size() < static_cast<size_t>(window)) {
        return rolling_dd;
    }

    for (size_t i = window - 1; i < cumulative_returns.size(); ++i) {
        std::vector<double> window_values(cumulative_returns.begin() + i - window + 1, cumulative_returns.begin() + i + 1);
        double dd = calculate_max_drawdown(window_values);
        rolling_dd.push_back(dd);
    }

    return rolling_dd;
}

double RiskCalculator::calculate_covariance(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) {
        return 0.0;
    }

    double mean_x = calculate_mean(x);
    double mean_y = calculate_mean(y);

    double covariance = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        covariance += (x[i] - mean_x) * (y[i] - mean_y);
    }

    return covariance / x.size();
}

// 辅助函数实现
double RiskCalculator::calculate_mean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double RiskCalculator::calculate_variance(const std::vector<double>& values, double mean) {
    if (values.empty()) return 0.0;

    double sum_squared_deviations = 0.0;
    for (double value : values) {
        double deviation = value - mean;
        sum_squared_deviations += deviation * deviation;
    }

    return sum_squared_deviations / values.size();
}

std::vector<double> RiskCalculator::calculate_excess_returns(const std::vector<double>& portfolio_returns,
                                                           const std::vector<double>& benchmark_returns) {
    std::vector<double> excess_returns;
    size_t min_size = std::min(portfolio_returns.size(), benchmark_returns.size());
    excess_returns.reserve(min_size);

    for (size_t i = 0; i < min_size; ++i) {
        excess_returns.push_back(portfolio_returns[i] - benchmark_returns[i]);
    }

    return excess_returns;
}

} // namespace qaultra::analysis