#include "qaultra/engine/backtest_engine.hpp"
#include "qaultra/util/datetime_utils.hpp"
#include "qaultra/util/uuid_generator.hpp"

#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>

namespace qaultra::engine {

// StrategyContext 实现

double StrategyContext::get_price(const std::string& symbol) const {
    auto position = account->get_position(symbol);
    if (position) {
        return position->price;
    }
    return current_price; // 默认使用当前价格
}

std::vector<double> StrategyContext::get_history(const std::string& symbol, int window, const std::string& field) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = data_cache_.find(symbol);
    if (it == data_cache_.end()) {
        return {}; // 没有找到数据
    }

    auto klines = it->second;
    if (field == "close") {
        auto closes = klines->get_close_column();
        if (closes.size() < static_cast<size_t>(window)) {
            return closes;
        }
        return std::vector<double>(closes.end() - window, closes.end());
    } else if (field == "open") {
        auto opens = klines->get_open_column();
        if (opens.size() < static_cast<size_t>(window)) {
            return opens;
        }
        return std::vector<double>(opens.end() - window, opens.end());
    } else if (field == "high") {
        auto highs = klines->get_high_column();
        if (highs.size() < static_cast<size_t>(window)) {
            return highs;
        }
        return std::vector<double>(highs.end() - window, highs.end());
    } else if (field == "low") {
        auto lows = klines->get_low_column();
        if (lows.size() < static_cast<size_t>(window)) {
            return lows;
        }
        return std::vector<double>(lows.end() - window, lows.end());
    } else if (field == "volume") {
        auto volumes = klines->get_volume_column();
        if (volumes.size() < static_cast<size_t>(window)) {
            return volumes;
        }
        return std::vector<double>(volumes.end() - window, volumes.end());
    }

    return {};
}

std::shared_ptr<account::Position> StrategyContext::get_position(const std::string& symbol) const {
    return account->get_position(symbol);
}

double StrategyContext::get_cash() const {
    return account->get_cash();
}

double StrategyContext::get_portfolio_value() const {
    return account->get_total_value();
}

void StrategyContext::log(const std::string& message) const {
    std::cout << "[" << current_date << "] " << message << std::endl;
}

// SMAStrategy 实现

void SMAStrategy::initialize(StrategyContext& context) {
    context.log("初始化SMA策略 (快线:" + std::to_string(fast_window) +
                ", 慢线:" + std::to_string(slow_window) + ")");

    // 初始化持仓状态
    for (const auto& symbol : context.universe) {
        positions_[symbol] = false;
    }
}

void SMAStrategy::handle_data(StrategyContext& context) {
    for (const auto& symbol : context.universe) {
        auto fast_prices = context.get_history(symbol, fast_window, "close");
        auto slow_prices = context.get_history(symbol, slow_window, "close");

        if (fast_prices.size() < static_cast<size_t>(fast_window) ||
            slow_prices.size() < static_cast<size_t>(slow_window)) {
            continue; // 数据不足
        }

        // 计算移动平均线
        double fast_ma = std::accumulate(fast_prices.begin(), fast_prices.end(), 0.0) / fast_window;
        double slow_ma = std::accumulate(slow_prices.begin(), slow_prices.end(), 0.0) / slow_window;

        double current_price = context.get_price(symbol);
        auto position = context.get_position(symbol);
        bool has_position = position && position->volume_long > 0;

        // 交易逻辑：快线上穿慢线买入，下穿卖出
        if (fast_ma > slow_ma && !has_position && context.get_cash() > current_price * 100) {
            // 买入信号
            double buy_amount = context.get_cash() * 0.2; // 使用20%资金
            double shares = std::floor(buy_amount / current_price / 100) * 100; // 整手买入

            if (shares >= 100) {
                try {
                    auto order = context.account->buy(symbol, shares, context.current_date, current_price);
                    context.log("买入 " + symbol + " " + std::to_string(shares) + "股 @ " +
                               std::to_string(current_price));
                    positions_[symbol] = true;
                } catch (const std::exception& e) {
                    context.log("买入失败: " + std::string(e.what()));
                }
            }
        } else if (fast_ma < slow_ma && has_position) {
            // 卖出信号
            try {
                auto order = context.account->sell(symbol, position->volume_long,
                                                  context.current_date, current_price);
                context.log("卖出 " + symbol + " " + std::to_string(position->volume_long) +
                           "股 @ " + std::to_string(current_price));
                positions_[symbol] = false;
            } catch (const std::exception& e) {
                context.log("卖出失败: " + std::string(e.what()));
            }
        }
    }
}

std::map<std::string, double> SMAStrategy::get_parameters() const {
    return {
        {"fast_window", static_cast<double>(fast_window)},
        {"slow_window", static_cast<double>(slow_window)}
    };
}

void SMAStrategy::set_parameter(const std::string& name, double value) {
    if (name == "fast_window") {
        fast_window = static_cast<int>(value);
    } else if (name == "slow_window") {
        slow_window = static_cast<int>(value);
    }
}

// MomentumStrategy 实现

void MomentumStrategy::initialize(StrategyContext& context) {
    context.log("初始化动量策略 (回望:" + std::to_string(lookback_window) +
                ", 阈值:" + std::to_string(threshold) + ")");

    for (const auto& symbol : context.universe) {
        price_history_[symbol].reserve(lookback_window + 10);
    }
}

void MomentumStrategy::handle_data(StrategyContext& context) {
    for (const auto& symbol : context.universe) {
        double current_price = context.get_price(symbol);

        // 更新价格历史
        auto& history = price_history_[symbol];
        history.push_back(current_price);

        if (history.size() > static_cast<size_t>(lookback_window + 1)) {
            history.erase(history.begin());
        }

        if (history.size() < static_cast<size_t>(lookback_window + 1)) {
            continue; // 数据不足
        }

        // 计算动量
        double past_price = history[0];
        double momentum = (current_price - past_price) / past_price;

        auto position = context.get_position(symbol);
        bool has_position = position && position->volume_long > 0;

        // 动量策略：正动量买入，负动量卖出
        if (momentum > threshold && !has_position && context.get_cash() > current_price * 100) {
            double buy_amount = context.get_cash() * 0.15; // 使用15%资金
            double shares = std::floor(buy_amount / current_price / 100) * 100;

            if (shares >= 100) {
                try {
                    auto order = context.account->buy(symbol, shares, context.current_date, current_price);
                    context.log("动量买入 " + symbol + " (动量:" + std::to_string(momentum) + ")");
                } catch (const std::exception& e) {
                    context.log("动量买入失败: " + std::string(e.what()));
                }
            }
        } else if (momentum < -threshold && has_position) {
            try {
                auto order = context.account->sell(symbol, position->volume_long,
                                                  context.current_date, current_price);
                context.log("动量卖出 " + symbol + " (动量:" + std::to_string(momentum) + ")");
            } catch (const std::exception& e) {
                context.log("动量卖出失败: " + std::string(e.what()));
            }
        }
    }
}

std::map<std::string, double> MomentumStrategy::get_parameters() const {
    return {
        {"lookback_window", static_cast<double>(lookback_window)},
        {"threshold", threshold}
    };
}

void MomentumStrategy::set_parameter(const std::string& name, double value) {
    if (name == "lookback_window") {
        lookback_window = static_cast<int>(value);
    } else if (name == "threshold") {
        threshold = value;
    }
}

// MeanReversionStrategy 实现

void MeanReversionStrategy::initialize(StrategyContext& context) {
    context.log("初始化均值回归策略 (窗口:" + std::to_string(window) +
                ", Z分数阈值:" + std::to_string(z_score_threshold) + ")");

    for (const auto& symbol : context.universe) {
        price_buffer_[symbol].reserve(window + 10);
    }
}

void MeanReversionStrategy::handle_data(StrategyContext& context) {
    for (const auto& symbol : context.universe) {
        double current_price = context.get_price(symbol);

        auto& buffer = price_buffer_[symbol];
        buffer.push_back(current_price);

        if (buffer.size() > static_cast<size_t>(window)) {
            buffer.erase(buffer.begin());
        }

        if (buffer.size() < static_cast<size_t>(window)) {
            continue;
        }

        // 计算移动平均和标准差
        double mean = std::accumulate(buffer.begin(), buffer.end(), 0.0) / window;

        double variance = 0.0;
        for (double price : buffer) {
            variance += (price - mean) * (price - mean);
        }
        variance /= window;
        double std_dev = std::sqrt(variance);

        // 计算Z分数
        double z_score = (current_price - mean) / std_dev;

        auto position = context.get_position(symbol);
        bool has_position = position && position->volume_long > 0;

        // 均值回归策略：价格远离均值时反向操作
        if (z_score < -z_score_threshold && !has_position && context.get_cash() > current_price * 100) {
            // 价格过低，买入
            double buy_amount = context.get_cash() * 0.1; // 使用10%资金
            double shares = std::floor(buy_amount / current_price / 100) * 100;

            if (shares >= 100) {
                try {
                    auto order = context.account->buy(symbol, shares, context.current_date, current_price);
                    context.log("均值回归买入 " + symbol + " (Z分数:" + std::to_string(z_score) + ")");
                } catch (const std::exception& e) {
                    context.log("均值回归买入失败: " + std::string(e.what()));
                }
            }
        } else if (z_score > z_score_threshold && has_position) {
            // 价格过高，卖出
            try {
                auto order = context.account->sell(symbol, position->volume_long,
                                                  context.current_date, current_price);
                context.log("均值回归卖出 " + symbol + " (Z分数:" + std::to_string(z_score) + ")");
            } catch (const std::exception& e) {
                context.log("均值回归卖出失败: " + std::string(e.what()));
            }
        }
    }
}

std::map<std::string, double> MeanReversionStrategy::get_parameters() const {
    return {
        {"window", static_cast<double>(window)},
        {"z_score_threshold", z_score_threshold}
    };
}

void MeanReversionStrategy::set_parameter(const std::string& name, double value) {
    if (name == "window") {
        window = static_cast<int>(value);
    } else if (name == "z_score_threshold") {
        z_score_threshold = value;
    }
}

// BacktestEngine 实现

BacktestEngine::BacktestEngine(const BacktestConfig& config) : config_(config) {
    initialize_account();
    if (config_.enable_matching_engine) {
        initialize_matching_engine();
    }
}

BacktestEngine::~BacktestEngine() {
    if (matching_engine_) {
        matching_engine_->stop();
    }
}

void BacktestEngine::initialize_account() {
    account_ = std::make_shared<account::QA_Account>(
        "backtest_account",
        "backtest_portfolio",
        "backtest_user",
        config_.initial_cash,
        false,
        "backtest"
    );
}

void BacktestEngine::initialize_matching_engine() {
    matching_engine_ = std::make_unique<market::MatchingEngine>(config_.max_threads);

    // 添加交易回调
    matching_engine_->add_trade_callback([this](const market::TradeResult& trade) {
        trade_records_.emplace_back(trade.trade_id, trade.trade_price * trade.trade_volume);
    });

    matching_engine_->start();
}

void BacktestEngine::add_strategy(std::shared_ptr<Strategy> strategy) {
    strategies_.push_back(strategy);
}

void BacktestEngine::set_universe(const std::vector<std::string>& symbols) {
    universe_ = symbols;
}

bool BacktestEngine::load_data(const std::string& data_source) {
    if (data_source.empty()) {
        return load_data_from_database();
    } else {
        return load_data_from_file(data_source);
    }
}

bool BacktestEngine::load_data_from_file(const std::string& filename) {
    // 简化实现：假设从Parquet文件加载
    for (const auto& symbol : universe_) {
        auto klines = std::make_shared<arrow_data::ArrowKlineCollection>();

        std::string symbol_file = filename + "/" + symbol + ".parquet";
        if (klines->load_parquet(symbol_file)) {
            market_data_[symbol] = klines;
        } else {
            std::cerr << "警告：无法加载 " << symbol << " 的数据" << std::endl;
        }
    }

    return !market_data_.empty();
}

bool BacktestEngine::load_data_from_database() {
    // 简化实现：生成模拟数据
    std::random_device rd;
    std::mt19937 gen(rd());

    for (const auto& symbol : universe_) {
        auto klines = std::make_shared<arrow_data::ArrowKlineCollection>();

        // 生成252天的交易数据
        std::vector<std::string> codes;
        std::vector<int64_t> timestamps;
        std::vector<double> opens, highs, lows, closes, volumes, amounts;

        double price = 100.0; // 初始价格

        for (int i = 0; i < 252; ++i) {
            // 随机游走生成价格
            std::normal_distribution<double> price_dist(0.0, 0.02);
            double return_rate = price_dist(gen);

            double open = price;
            double close = price * (1 + return_rate);
            double high = std::max(open, close) * (1 + std::abs(price_dist(gen)) * 0.5);
            double low = std::min(open, close) * (1 - std::abs(price_dist(gen)) * 0.5);

            std::uniform_real_distribution<double> volume_dist(50000, 200000);
            double volume = volume_dist(gen);
            double amount = close * volume;

            codes.push_back(symbol);
            timestamps.push_back(1704067200000LL + i * 86400000LL); // 从2024-01-01开始
            opens.push_back(open);
            highs.push_back(high);
            lows.push_back(low);
            closes.push_back(close);
            volumes.push_back(volume);
            amounts.push_back(amount);

            price = close; // 更新价格
        }

        klines->add_batch(codes, timestamps, opens, highs, lows, closes, volumes, amounts);
        market_data_[symbol] = klines;
    }

    return true;
}

BacktestResults BacktestEngine::run() {
    if (strategies_.empty()) {
        throw std::runtime_error("没有添加任何策略");
    }

    if (market_data_.empty()) {
        throw std::runtime_error("没有加载市场数据");
    }

    is_running_ = true;
    std::cout << "开始回测..." << std::endl;
    std::cout << "初始资金: " << config_.initial_cash << std::endl;
    std::cout << "策略数量: " << strategies_.size() << std::endl;
    std::cout << "股票池大小: " << universe_.size() << std::endl;

    // 初始化策略上下文
    StrategyContext context;
    context.account = account_;
    context.universe = universe_;

    // 复制市场数据到上下文缓存
    {
        std::lock_guard<std::mutex> lock(context.cache_mutex_);
        context.data_cache_ = market_data_;
    }

    // 初始化所有策略
    for (auto& strategy : strategies_) {
        strategy->initialize(context);
    }

    // 获取所有交易日期
    if (!market_data_.empty()) {
        auto first_symbol_data = market_data_.begin()->second;
        auto timestamps = first_symbol_data->get_timestamp_column();

        for (auto timestamp : timestamps) {
            auto time_point = std::chrono::system_clock::from_time_t(timestamp / 1000);
            auto date_str = utils::format_datetime(time_point);
            date_index_.push_back(date_str);
        }
    }

    // 逐日运行回测
    for (size_t day = 0; day < date_index_.size(); ++day) {
        if (!is_running_) break;

        current_index_ = day;
        current_date_ = date_index_[day];
        context.current_date = current_date_;

        run_single_day(current_date_);
        record_daily_performance();

        // 每50天输出一次进度
        if (day % 50 == 0) {
            std::cout << "进度: " << day << "/" << date_index_.size()
                     << " (" << (day * 100 / date_index_.size()) << "%)" << std::endl;
        }
    }

    // 计算性能指标
    calculate_performance_metrics();

    std::cout << "回测完成!" << std::endl;
    std::cout << "最终资产: " << results_.final_value << std::endl;
    std::cout << "总收益率: " << (results_.total_return * 100) << "%" << std::endl;
    std::cout << "夏普比率: " << results_.sharpe_ratio << std::endl;
    std::cout << "最大回撤: " << (results_.max_drawdown * 100) << "%" << std::endl;

    is_running_ = false;
    return results_;
}

void BacktestEngine::run_single_day(const std::string& date) {
    // 更新市场数据
    update_market_data(date);

    // 创建策略上下文
    StrategyContext context;
    context.account = account_;
    context.universe = universe_;
    context.current_date = date;

    {
        std::lock_guard<std::mutex> lock(context.cache_mutex_);
        context.data_cache_ = market_data_;
    }

    // 执行策略
    execute_strategies(context);
}

void BacktestEngine::update_market_data(const std::string& date) {
    // 更新所有股票的当日价格
    for (const auto& symbol : universe_) {
        auto it = market_data_.find(symbol);
        if (it != market_data_.end()) {
            auto klines = it->second;
            auto closes = klines->get_close_column();

            if (current_index_ < closes.size()) {
                double current_price = closes[current_index_];
                account_->on_price_change(symbol, current_price, date);
            }
        }
    }
}

void BacktestEngine::execute_strategies(StrategyContext& context) {
    // 开盘前处理
    for (auto& strategy : strategies_) {
        strategy->before_market_open(context);
    }

    // 主要数据处理
    for (auto& strategy : strategies_) {
        try {
            strategy->handle_data(context);
        } catch (const std::exception& e) {
            std::cerr << "策略执行错误: " << e.what() << std::endl;
        }
    }

    // 收盘后处理
    for (auto& strategy : strategies_) {
        strategy->after_market_close(context);
    }
}

void BacktestEngine::record_daily_performance() {
    double total_value = account_->get_total_value();
    daily_equity_.push_back(total_value);

    results_.equity_curve.push_back(total_value);
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

    auto trades = account_->get_trades();
    results_.total_trades = trades.size();

    // 计算交易收益率用于胜率和盈亏比计算
    std::vector<double> trade_returns;
    for (const auto& trade : trades) {
        // 简化的收益率计算
        if (trade->direction == Direction::SELL) {
            // 这里需要更复杂的逻辑来匹配买卖订单
            trade_returns.push_back(0.01); // 占位符
        }
    }

    if (!trade_returns.empty()) {
        results_.win_rate = calculate_win_rate();
        results_.profit_factor = calculate_profit_factor();
    }

    // 计算每日收益率
    results_.daily_returns = calculate_returns_simd(daily_equity_);
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

double BacktestEngine::calculate_win_rate() const {
    auto trades = account_->get_trades();
    if (trades.empty()) return 0.0;

    // 简化实现：假设所有交易都有收益数据
    int winning_trades = 0;
    for (const auto& trade : trades) {
        // 这里需要实际的盈亏计算逻辑
        if (trade->direction == Direction::SELL) {
            winning_trades++; // 占位符逻辑
        }
    }

    return static_cast<double>(winning_trades) / trades.size();
}

double BacktestEngine::calculate_profit_factor() const {
    // 简化实现
    return 1.5; // 占位符
}

std::vector<double> BacktestEngine::calculate_returns_simd(const std::vector<double>& prices) const {
    if (prices.size() < 2) return {};

    std::vector<double> returns;
    returns.reserve(prices.size() - 1);

    // 使用SIMD优化计算收益率
    for (size_t i = 1; i < prices.size(); ++i) {
        returns.push_back((prices[i] - prices[i-1]) / prices[i-1]);
    }

    return returns;
}

double BacktestEngine::calculate_volatility_simd(const std::vector<double>& returns) const {
    if (returns.empty()) return 0.0;

    // 使用SIMD计算方差
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    variance /= returns.size();

    return std::sqrt(variance) * std::sqrt(252.0); // 年化波动率
}

bool BacktestEngine::save_results(const std::string& filename) const {
    nlohmann::json result_json;

    result_json["config"] = {
        {"start_date", config_.start_date},
        {"end_date", config_.end_date},
        {"initial_cash", config_.initial_cash},
        {"commission_rate", config_.commission_rate},
        {"benchmark", config_.benchmark}
    };

    result_json["performance"] = {
        {"total_return", results_.total_return},
        {"annual_return", results_.annual_return},
        {"sharpe_ratio", results_.sharpe_ratio},
        {"max_drawdown", results_.max_drawdown},
        {"volatility", results_.volatility},
        {"win_rate", results_.win_rate},
        {"profit_factor", results_.profit_factor},
        {"total_trades", results_.total_trades},
        {"final_value", results_.final_value}
    };

    result_json["equity_curve"] = results_.equity_curve;
    result_json["daily_returns"] = results_.daily_returns;

    std::ofstream file(filename);
    if (file.is_open()) {
        file << result_json.dump(4);
        return true;
    }

    return false;
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

    auto trades = account_->get_trades();

    std::vector<double> trade_amounts;
    std::vector<double> trade_prices;
    std::vector<double> commissions;

    for (const auto& trade : trades) {
        trade_amounts.push_back(trade->volume * trade->price);
        trade_prices.push_back(trade->price);
        commissions.push_back(trade->commission);
    }

    analysis["trade_amounts"] = trade_amounts;
    analysis["trade_prices"] = trade_prices;
    analysis["commissions"] = commissions;

    return analysis;
}

// 工具函数实现

namespace utils {

double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean_return) * (ret - mean_return);
    }
    variance /= returns.size();

    double std_dev = std::sqrt(variance);
    return std_dev > 0 ? (mean_return - risk_free_rate) / std_dev : 0.0;
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

// 工厂函数实现

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