#include "../../include/qaultra/engine/unified_backtest_engine.hpp"
// #include "../../include/qaultra/util/datetime_utils.hpp" // TODO: 创建此文件
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace qaultra::engine {

// ==================== UnifiedBacktestConfig 实现 ====================

nlohmann::json UnifiedBacktestConfig::to_json() const {
    nlohmann::json j;
    j["start_date"] = start_date;
    j["end_date"] = end_date;
    j["initial_cash"] = initial_cash;
    j["commission_rate"] = commission_rate;
    j["slippage"] = slippage;
    j["benchmark"] = benchmark;
    j["enable_matching_engine"] = enable_matching_engine;
    j["enable_parallel_processing"] = enable_parallel_processing;
    j["max_threads"] = max_threads;
    j["max_position_ratio"] = max_position_ratio;
    j["stop_loss_ratio"] = stop_loss_ratio;
    j["take_profit_ratio"] = take_profit_ratio;
    j["enable_cache"] = enable_cache;
    j["enable_logging"] = enable_logging;
    j["enable_performance_tracking"] = enable_performance_tracking;
    j["log_level"] = log_level;
    return j;
}

UnifiedBacktestConfig UnifiedBacktestConfig::from_json(const nlohmann::json& j) {
    UnifiedBacktestConfig config;
    if (j.contains("start_date")) config.start_date = j["start_date"];
    if (j.contains("end_date")) config.end_date = j["end_date"];
    if (j.contains("initial_cash")) config.initial_cash = j["initial_cash"];
    if (j.contains("commission_rate")) config.commission_rate = j["commission_rate"];
    if (j.contains("slippage")) config.slippage = j["slippage"];
    if (j.contains("benchmark")) config.benchmark = j["benchmark"];
    if (j.contains("enable_matching_engine")) config.enable_matching_engine = j["enable_matching_engine"];
    if (j.contains("enable_parallel_processing")) config.enable_parallel_processing = j["enable_parallel_processing"];
    if (j.contains("max_threads")) config.max_threads = j["max_threads"];
    if (j.contains("max_position_ratio")) config.max_position_ratio = j["max_position_ratio"];
    if (j.contains("stop_loss_ratio")) config.stop_loss_ratio = j["stop_loss_ratio"];
    if (j.contains("take_profit_ratio")) config.take_profit_ratio = j["take_profit_ratio"];
    if (j.contains("enable_cache")) config.enable_cache = j["enable_cache"];
    if (j.contains("enable_logging")) config.enable_logging = j["enable_logging"];
    if (j.contains("enable_performance_tracking")) config.enable_performance_tracking = j["enable_performance_tracking"];
    if (j.contains("log_level")) config.log_level = j["log_level"];
    return config;
}

// ==================== UnifiedBacktestResults 实现 ====================

nlohmann::json UnifiedBacktestResults::to_json() const {
    nlohmann::json j;
    j["total_return"] = total_return;
    j["annual_return"] = annual_return;
    j["sharpe_ratio"] = sharpe_ratio;
    j["sortino_ratio"] = sortino_ratio;
    j["calmar_ratio"] = calmar_ratio;
    j["max_drawdown"] = max_drawdown;
    j["volatility"] = volatility;
    j["downside_deviation"] = downside_deviation;
    j["win_rate"] = win_rate;
    j["profit_factor"] = profit_factor;
    j["total_trades"] = total_trades;
    j["winning_trades"] = winning_trades;
    j["losing_trades"] = losing_trades;
    j["final_value"] = final_value;
    j["max_single_trade_profit"] = max_single_trade_profit;
    j["max_single_trade_loss"] = max_single_trade_loss;
    j["beta"] = beta;
    j["alpha"] = alpha;
    j["var_95"] = var_95;
    j["cvar_95"] = cvar_95;
    j["information_ratio"] = information_ratio;
    j["equity_curve"] = equity_curve;
    j["daily_returns"] = daily_returns;
    j["benchmark_returns"] = benchmark_returns;
    j["trade_dates"] = trade_dates;
    j["strategy_metrics"] = strategy_metrics;
    return j;
}

UnifiedBacktestResults UnifiedBacktestResults::from_json(const nlohmann::json& j) {
    UnifiedBacktestResults results;
    if (j.contains("total_return")) results.total_return = j["total_return"];
    if (j.contains("annual_return")) results.annual_return = j["annual_return"];
    if (j.contains("sharpe_ratio")) results.sharpe_ratio = j["sharpe_ratio"];
    if (j.contains("sortino_ratio")) results.sortino_ratio = j["sortino_ratio"];
    if (j.contains("calmar_ratio")) results.calmar_ratio = j["calmar_ratio"];
    if (j.contains("max_drawdown")) results.max_drawdown = j["max_drawdown"];
    if (j.contains("volatility")) results.volatility = j["volatility"];
    if (j.contains("downside_deviation")) results.downside_deviation = j["downside_deviation"];
    if (j.contains("win_rate")) results.win_rate = j["win_rate"];
    if (j.contains("profit_factor")) results.profit_factor = j["profit_factor"];
    if (j.contains("total_trades")) results.total_trades = j["total_trades"];
    if (j.contains("winning_trades")) results.winning_trades = j["winning_trades"];
    if (j.contains("losing_trades")) results.losing_trades = j["losing_trades"];
    if (j.contains("final_value")) results.final_value = j["final_value"];
    if (j.contains("max_single_trade_profit")) results.max_single_trade_profit = j["max_single_trade_profit"];
    if (j.contains("max_single_trade_loss")) results.max_single_trade_loss = j["max_single_trade_loss"];
    if (j.contains("beta")) results.beta = j["beta"];
    if (j.contains("alpha")) results.alpha = j["alpha"];
    if (j.contains("var_95")) results.var_95 = j["var_95"];
    if (j.contains("cvar_95")) results.cvar_95 = j["cvar_95"];
    if (j.contains("information_ratio")) results.information_ratio = j["information_ratio"];
    if (j.contains("equity_curve")) results.equity_curve = j["equity_curve"];
    if (j.contains("daily_returns")) results.daily_returns = j["daily_returns"];
    if (j.contains("benchmark_returns")) results.benchmark_returns = j["benchmark_returns"];
    if (j.contains("trade_dates")) results.trade_dates = j["trade_dates"];
    if (j.contains("strategy_metrics")) results.strategy_metrics = j["strategy_metrics"];
    return results;
}

// ==================== EventEngine 实现 ====================

void EventEngine::register_handler(EventType type, EventHandler handler) {
    handlers_[type].push_back(handler);
}

void EventEngine::put_event(std::shared_ptr<Event> event) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    event_queue_.push(event);
}

void EventEngine::process_events() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!event_queue_.empty()) {
        auto event = event_queue_.front();
        event_queue_.pop();

        auto it = handlers_.find(event->type);
        if (it != handlers_.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }
}

void EventEngine::clear_events() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!event_queue_.empty()) {
        event_queue_.pop();
    }
}

void EventEngine::start() {
    running_ = true;
    processing_thread_ = std::thread(&EventEngine::process_events_loop, this);
}

void EventEngine::stop() {
    running_ = false;
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

void EventEngine::process_events_loop() {
    while (running_) {
        process_events();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ==================== UnifiedStrategyContext 实现 ====================

double UnifiedStrategyContext::get_price(const std::string& symbol) const {
    auto it = current_prices.find(symbol);
    return (it != current_prices.end()) ? it->second : 0.0;
}

std::vector<double> UnifiedStrategyContext::get_history(const std::string& symbol, int window, const std::string& field) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = data_cache_.find(symbol);
    if (it == data_cache_.end()) {
        return {};
    }

    auto container = it->second;
    auto unified_data = container->to_unified_klines();

    if (unified_data.size() < static_cast<size_t>(window)) {
        window = static_cast<int>(unified_data.size());
    }

    std::vector<double> result;
    result.reserve(window);

    for (int i = unified_data.size() - window; i < static_cast<int>(unified_data.size()); ++i) {
        if (field == "open") {
            result.push_back(unified_data[i].open);
        } else if (field == "high") {
            result.push_back(unified_data[i].high);
        } else if (field == "low") {
            result.push_back(unified_data[i].low);
        } else if (field == "close") {
            result.push_back(unified_data[i].close);
        } else if (field == "volume") {
            result.push_back(unified_data[i].volume);
        } else {
            result.push_back(unified_data[i].close); // 默认收盘价
        }
    }

    return result;
}

std::optional<account::Position> UnifiedStrategyContext::get_position(const std::string& symbol) const {
    return account->get_position(symbol);
}

double UnifiedStrategyContext::get_cash() const {
    return account->get_cash();
}

double UnifiedStrategyContext::get_total_value() const {
    return account->get_total_value();
}

void UnifiedStrategyContext::log(const std::string& message, const std::string& level) const {
    std::cout << "[" << current_date << "][" << level << "] " << message << std::endl;
}

// ==================== UnifiedSMAStrategy 实现 ====================

void UnifiedSMAStrategy::initialize(UnifiedStrategyContext& context) {
    context.log("初始化统一SMA策略 (快线:" + std::to_string(fast_window_) +
                ", 慢线:" + std::to_string(slow_window_) + ")");

    for (const auto& symbol : context.universe) {
        positions_[symbol] = false;
    }
}

void UnifiedSMAStrategy::handle_data(UnifiedStrategyContext& context) {
    for (const auto& symbol : context.universe) {
        auto fast_prices = context.get_history(symbol, fast_window_, "close");
        auto slow_prices = context.get_history(symbol, slow_window_, "close");

        if (fast_prices.size() < static_cast<size_t>(fast_window_) ||
            slow_prices.size() < static_cast<size_t>(slow_window_)) {
            continue;
        }

        double fast_ma = std::accumulate(fast_prices.begin(), fast_prices.end(), 0.0) / fast_window_;
        double slow_ma = std::accumulate(slow_prices.begin(), slow_prices.end(), 0.0) / slow_window_;

        double current_price = context.get_price(symbol);
        auto position = context.get_position(symbol);
        bool has_position = position && (position.value().volume_long > 0);

        if (fast_ma > slow_ma && !has_position && context.get_cash() > current_price * 100) {
            double buy_amount = context.get_cash() * 0.2;
            double shares = std::floor(buy_amount / current_price / 100) * 100;

            if (shares >= 100) {
                try {
                    context.account->buy(symbol, shares, current_price);
                    context.log("SMA买入 " + symbol + " " + std::to_string(shares) + "股");
                    positions_[symbol] = true;
                } catch (const std::exception& e) {
                    context.log("SMA买入失败: " + std::string(e.what()), "ERROR");
                }
            }
        } else if (fast_ma < slow_ma && has_position) {
            try {
                context.account->sell(symbol, position.value().volume_long, current_price);
                context.log("SMA卖出 " + symbol + " " + std::to_string(position.value().volume_long) + "股");
                positions_[symbol] = false;
            } catch (const std::exception& e) {
                context.log("SMA卖出失败: " + std::string(e.what()), "ERROR");
            }
        }
    }
}

std::unordered_map<std::string, double> UnifiedSMAStrategy::get_parameters() const {
    return {
        {"fast_window", static_cast<double>(fast_window_)},
        {"slow_window", static_cast<double>(slow_window_)}
    };
}

void UnifiedSMAStrategy::set_parameter(const std::string& name, double value) {
    if (name == "fast_window") {
        fast_window_ = static_cast<int>(value);
    } else if (name == "slow_window") {
        slow_window_ = static_cast<int>(value);
    }
}

// ==================== UnifiedMomentumStrategy 实现 ====================

void UnifiedMomentumStrategy::initialize(UnifiedStrategyContext& context) {
    context.log("初始化统一动量策略 (回望:" + std::to_string(lookback_window_) +
                ", 阈值:" + std::to_string(threshold_) + ")");

    for (const auto& symbol : context.universe) {
        price_history_[symbol].reserve(lookback_window_ + 10);
    }
}

void UnifiedMomentumStrategy::handle_data(UnifiedStrategyContext& context) {
    for (const auto& symbol : context.universe) {
        double current_price = context.get_price(symbol);

        auto& history = price_history_[symbol];
        history.push_back(current_price);

        if (history.size() > static_cast<size_t>(lookback_window_ + 1)) {
            history.erase(history.begin());
        }

        if (history.size() < static_cast<size_t>(lookback_window_ + 1)) {
            continue;
        }

        double past_price = history[0];
        double momentum = (current_price - past_price) / past_price;

        auto position = context.get_position(symbol);
        bool has_position = position && (position.value().volume_long > 0);

        if (momentum > threshold_ && !has_position && context.get_cash() > current_price * 100) {
            double buy_amount = context.get_cash() * 0.15;
            double shares = std::floor(buy_amount / current_price / 100) * 100;

            if (shares >= 100) {
                try {
                    context.account->buy(symbol, shares, current_price);
                    context.log("动量买入 " + symbol + " (动量:" + std::to_string(momentum) + ")");
                } catch (const std::exception& e) {
                    context.log("动量买入失败: " + std::string(e.what()), "ERROR");
                }
            }
        } else if (momentum < -threshold_ && has_position) {
            try {
                context.account->sell(symbol, position.value().volume_long, current_price);
                context.log("动量卖出 " + symbol + " (动量:" + std::to_string(momentum) + ")");
            } catch (const std::exception& e) {
                context.log("动量卖出失败: " + std::string(e.what()), "ERROR");
            }
        }
    }
}

std::unordered_map<std::string, double> UnifiedMomentumStrategy::get_parameters() const {
    return {
        {"lookback_window", static_cast<double>(lookback_window_)},
        {"threshold", threshold_}
    };
}

void UnifiedMomentumStrategy::set_parameter(const std::string& name, double value) {
    if (name == "lookback_window") {
        lookback_window_ = static_cast<int>(value);
    } else if (name == "threshold") {
        threshold_ = value;
    }
}

// ==================== UnifiedMeanReversionStrategy 实现 ====================

void UnifiedMeanReversionStrategy::initialize(UnifiedStrategyContext& context) {
    context.log("初始化统一均值回归策略 (窗口:" + std::to_string(window_) +
                ", Z分数阈值:" + std::to_string(z_score_threshold_) + ")");

    for (const auto& symbol : context.universe) {
        price_buffer_[symbol].reserve(window_ + 10);
    }
}

void UnifiedMeanReversionStrategy::handle_data(UnifiedStrategyContext& context) {
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

        double mean = std::accumulate(buffer.begin(), buffer.end(), 0.0) / window_;

        double variance = 0.0;
        for (double price : buffer) {
            variance += (price - mean) * (price - mean);
        }
        variance /= window_;
        double std_dev = std::sqrt(variance);

        if (std_dev == 0.0) continue;

        double z_score = (current_price - mean) / std_dev;

        auto position = context.get_position(symbol);
        bool has_position = position && (position.value().volume_long > 0);

        if (z_score < -z_score_threshold_ && !has_position && context.get_cash() > current_price * 100) {
            double buy_amount = context.get_cash() * 0.1;
            double shares = std::floor(buy_amount / current_price / 100) * 100;

            if (shares >= 100) {
                try {
                    context.account->buy(symbol, shares, current_price);
                    context.log("均值回归买入 " + symbol + " (Z分数:" + std::to_string(z_score) + ")");
                } catch (const std::exception& e) {
                    context.log("均值回归买入失败: " + std::string(e.what()), "ERROR");
                }
            }
        } else if (z_score > z_score_threshold_ && has_position) {
            try {
                context.account->sell(symbol, position.value().volume_long, current_price);
                context.log("均值回归卖出 " + symbol + " (Z分数:" + std::to_string(z_score) + ")");
            } catch (const std::exception& e) {
                context.log("均值回归卖出失败: " + std::string(e.what()), "ERROR");
            }
        }
    }
}

std::unordered_map<std::string, double> UnifiedMeanReversionStrategy::get_parameters() const {
    return {
        {"window", static_cast<double>(window_)},
        {"z_score_threshold", z_score_threshold_}
    };
}

void UnifiedMeanReversionStrategy::set_parameter(const std::string& name, double value) {
    if (name == "window") {
        window_ = static_cast<int>(value);
    } else if (name == "z_score_threshold") {
        z_score_threshold_ = value;
    }
}

// ==================== UnifiedDataManager 实现 ====================

bool UnifiedDataManager::load_data(const std::string& symbol, const std::string& start_date,
                                   const std::string& end_date, const std::string& frequency) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    // 简化实现：生成模拟数据
    auto container = std::make_shared<data::MarketDataContainer>(symbol);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> price_dist(0.0, 0.02);
    std::uniform_real_distribution<double> volume_dist(50000, 200000);

    double base_price = 100.0;

    // 生成252个交易日的数据
    for (int i = 0; i < 252; ++i) {
        data::UnifiedKline kline;
        kline.order_book_id = symbol;
        kline.datetime = std::chrono::system_clock::now() + std::chrono::hours(24 * i);

        double return_rate = price_dist(gen);
        kline.open = base_price;
        kline.close = base_price * (1 + return_rate);
        kline.high = std::max(kline.open, kline.close) * (1 + std::abs(price_dist(gen)) * 0.5);
        kline.low = std::min(kline.open, kline.close) * (1 - std::abs(price_dist(gen)) * 0.5);
        kline.volume = volume_dist(gen);
        kline.total_turnover = kline.close * kline.volume;

        container->add_data(kline);
        base_price = kline.close;

        // 生成日期索引
        auto time_t = std::chrono::system_clock::to_time_t(kline.datetime);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d");
        std::string date_str = ss.str();

        if (std::find(date_index_.begin(), date_index_.end(), date_str) == date_index_.end()) {
            date_index_.push_back(date_str);
        }
    }

    data_[symbol] = container;
    return true;
}

bool UnifiedDataManager::load_data_from_file(const std::string& filename) {
    // 简化实现：假设成功加载
    return true;
}

bool UnifiedDataManager::load_data_from_database() {
    // 简化实现：假设成功加载
    return true;
}

std::optional<data::UnifiedKline> UnifiedDataManager::get_data(const std::string& symbol, const std::string& date) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = data_.find(symbol);
    if (it == data_.end()) {
        return std::nullopt;
    }

    // 简化实现：返回第一个数据
    auto unified_data = it->second->to_unified_klines();
    if (!unified_data.empty()) {
        return unified_data[0];
    }

    return std::nullopt;
}

std::vector<data::UnifiedKline> UnifiedDataManager::get_data_range(const std::string& symbol,
                                                                   const std::string& start_date,
                                                                   const std::string& end_date) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = data_.find(symbol);
    if (it == data_.end()) {
        return {};
    }

    return it->second->to_unified_klines();
}

std::shared_ptr<data::MarketDataContainer> UnifiedDataManager::get_symbol_data(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = data_.find(symbol);
    if (it != data_.end()) {
        return it->second;
    }

    return nullptr;
}

void UnifiedDataManager::add_data(const std::string& symbol, const data::UnifiedKline& kline) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = data_.find(symbol);
    if (it == data_.end()) {
        auto container = std::make_shared<data::MarketDataContainer>(symbol);
        container->add_data(kline);
        data_[symbol] = container;
    } else {
        it->second->add_data(kline);
    }
}

void UnifiedDataManager::clear_data() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    data_.clear();
    date_index_.clear();
}

bool UnifiedDataManager::has_data(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return data_.find(symbol) != data_.end();
}

std::vector<std::string> UnifiedDataManager::get_available_symbols() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<std::string> symbols;
    for (const auto& pair : data_) {
        symbols.push_back(pair.first);
    }
    return symbols;
}

std::vector<std::string> UnifiedDataManager::get_date_index() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return date_index_;
}

void UnifiedDataManager::set_current_date(const std::string& date) {
    current_date_ = date;
}

std::string UnifiedDataManager::get_current_date() const {
    return current_date_;
}

// ==================== UnifiedBacktestEngine 实现 ====================

UnifiedBacktestEngine::UnifiedBacktestEngine(const UnifiedBacktestConfig& config) : config_(config) {
    if (!validate_configuration()) {
        throw std::runtime_error("无效的回测配置");
    }

    initialize_components();
    setup_event_handlers();

    log_message("统一回测引擎初始化完成");
}

UnifiedBacktestEngine::~UnifiedBacktestEngine() {
    stop();
    if (event_engine_) {
        event_engine_->stop();
    }
}

bool UnifiedBacktestEngine::validate_configuration() const {
    if (config_.initial_cash <= 0) {
        log_message("初始资金必须大于0", "ERROR");
        return false;
    }

    if (config_.commission_rate < 0) {
        log_message("手续费率不能为负", "ERROR");
        return false;
    }

    if (config_.start_date >= config_.end_date) {
        log_message("开始日期必须早于结束日期", "ERROR");
        return false;
    }

    return true;
}

void UnifiedBacktestEngine::initialize_components() {
    // 初始化账户
    account_ = std::make_shared<account::UnifiedAccount>(
        "unified_backtest_account",
        "unified_backtest_portfolio",
        "unified_backtest_user",
        config_.initial_cash,
        false
    );

    // 设置市场预设
    account::MarketPreset preset = account::MarketPreset::get_stock_preset();
    preset.buy_fee_ratio = config_.commission_rate;
    preset.sell_fee_ratio = config_.commission_rate;
    account_->set_market_preset(preset);

    // 初始化市场模拟器
    if (config_.enable_matching_engine) {
        market_sim_ = std::make_unique<market::simmarket::QASIMMarket>();
    }

    // 初始化事件引擎
    event_engine_ = std::make_shared<EventEngine>();

    // 初始化数据管理器
    data_manager_ = std::make_unique<UnifiedDataManager>();
}

void UnifiedBacktestEngine::setup_event_handlers() {
    using namespace std::placeholders;

    event_engine_->register_handler(EventType::MARKET_DATA,
        std::bind(&UnifiedBacktestEngine::on_market_data_event, this, _1));
    event_engine_->register_handler(EventType::ORDER,
        std::bind(&UnifiedBacktestEngine::on_order_event, this, _1));
    event_engine_->register_handler(EventType::TRADE,
        std::bind(&UnifiedBacktestEngine::on_trade_event, this, _1));
}

void UnifiedBacktestEngine::add_strategy(std::shared_ptr<UnifiedStrategy> strategy) {
    strategies_.push_back(strategy);
    log_message("添加策略: " + strategy->get_name() + ", 当前策略数量: " + std::to_string(strategies_.size()));
}

void UnifiedBacktestEngine::set_universe(const std::vector<std::string>& symbols) {
    universe_ = symbols;
    log_message("设置股票池，包含 " + std::to_string(universe_.size()) + " 只股票");
    for (const auto& symbol : universe_) {
        log_message("  - " + symbol);
    }
}

bool UnifiedBacktestEngine::load_data(const std::string& data_source) {
    log_message("加载数据源: " + (data_source.empty() ? "默认模拟数据" : data_source));

    if (data_source.empty()) {
        // 为每个股票加载模拟数据
        for (const auto& symbol : universe_) {
            if (!data_manager_->load_data(symbol, config_.start_date, config_.end_date)) {
                log_message("加载 " + symbol + " 数据失败", "ERROR");
                return false;
            }
        }
    } else {
        return data_manager_->load_data_from_file(data_source);
    }

    date_index_ = data_manager_->get_date_index();
    log_message("共加载 " + std::to_string(date_index_.size()) + " 个交易日的数据");

    return true;
}

UnifiedBacktestResults UnifiedBacktestEngine::run() {
    if (strategies_.empty()) {
        throw std::runtime_error("没有添加任何策略");
    }

    if (universe_.empty()) {
        throw std::runtime_error("没有设置股票池");
    }

    if (date_index_.empty()) {
        throw std::runtime_error("没有加载市场数据");
    }

    is_running_ = true;
    log_message("开始统一回测...");
    log_message("配置信息:");
    log_message("  初始资金: " + std::to_string(config_.initial_cash));
    log_message("  策略数量: " + std::to_string(strategies_.size()));
    log_message("  股票池大小: " + std::to_string(universe_.size()));
    log_message("  交易日期范围: " + config_.start_date + " 至 " + config_.end_date);
    log_message("  总交易日数: " + std::to_string(date_index_.size()));

    // 启动事件引擎
    event_engine_->start();

    // 初始化策略上下文
    UnifiedStrategyContext context;
    context.account = account_;
    context.universe = universe_;

    // 复制数据缓存到上下文
    {
        std::lock_guard<std::mutex> lock(context.cache_mutex_);
        for (const auto& symbol : universe_) {
            auto symbol_data = data_manager_->get_symbol_data(symbol);
            if (symbol_data) {
                context.data_cache_[symbol] = symbol_data;
            }
        }
    }

    // 初始化所有策略
    for (auto& strategy : strategies_) {
        strategy->initialize(context);
    }

    // 逐日运行回测
    for (size_t day = 0; day < date_index_.size() && is_running_; ++day) {
        current_index_ = day;
        current_date_ = date_index_[day];
        data_manager_->set_current_date(current_date_);

        run_single_day(current_date_);
        record_daily_performance();

        // 每50天输出一次进度
        if (day % 50 == 0 || day == date_index_.size() - 1) {
            double progress = (double(day + 1) / date_index_.size()) * 100.0;
            log_message("回测进度: " + std::to_string(day + 1) + "/" +
                       std::to_string(date_index_.size()) + " (" +
                       std::to_string(progress) + "%)");
        }
    }

    // 结束策略
    for (auto& strategy : strategies_) {
        strategy->finalize(context);
    }

    // 停止事件引擎
    event_engine_->stop();

    // 计算性能指标
    calculate_performance_metrics();

    log_message("统一回测完成!");
    log_message("最终资产: " + std::to_string(results_.final_value));
    log_message("总收益率: " + std::to_string(results_.total_return * 100) + "%");
    log_message("年化收益率: " + std::to_string(results_.annual_return * 100) + "%");
    log_message("夏普比率: " + std::to_string(results_.sharpe_ratio));
    log_message("索提诺比率: " + std::to_string(results_.sortino_ratio));
    log_message("最大回撤: " + std::to_string(results_.max_drawdown * 100) + "%");

    is_running_ = false;
    return results_;
}

void UnifiedBacktestEngine::run_single_day(const std::string& date) {
    // 更新市场数据
    update_market_data(date);

    // 创建策略上下文
    UnifiedStrategyContext context;
    context.account = account_;
    context.universe = universe_;
    context.current_date = date;
    context.current_prices.clear();

    // 获取当日价格
    for (const auto& symbol : universe_) {
        auto data_opt = data_manager_->get_data(symbol, date);
        if (data_opt) {
            context.current_prices[symbol] = data_opt.value().close;
        }
    }

    // 复制数据缓存
    {
        std::lock_guard<std::mutex> lock(context.cache_mutex_);
        for (const auto& symbol : universe_) {
            auto symbol_data = data_manager_->get_symbol_data(symbol);
            if (symbol_data) {
                context.data_cache_[symbol] = symbol_data;
            }
        }
    }

    // 执行策略
    execute_strategies(context);

    // 处理订单
    process_orders();
}

void UnifiedBacktestEngine::update_market_data(const std::string& date) {
    for (const auto& symbol : universe_) {
        auto data_opt = data_manager_->get_data(symbol, date);
        if (data_opt) {
            account_->update_market_data(symbol, data_opt.value().close);

            // 发送市场数据事件
            auto event = std::make_shared<MarketDataEvent>(data_opt.value());
            event_engine_->put_event(event);
        }
    }
}

void UnifiedBacktestEngine::execute_strategies(UnifiedStrategyContext& context) {
    // 开盘前处理
    for (auto& strategy : strategies_) {
        try {
            strategy->before_market_open(context);
        } catch (const std::exception& e) {
            log_message("策略 " + strategy->get_name() + " 开盘前处理错误: " + e.what(), "ERROR");
        }
    }

    // 主要数据处理
    for (auto& strategy : strategies_) {
        try {
            strategy->handle_data(context);
        } catch (const std::exception& e) {
            log_message("策略 " + strategy->get_name() + " 数据处理错误: " + e.what(), "ERROR");
        }
    }

    // 收盘后处理
    for (auto& strategy : strategies_) {
        try {
            strategy->after_market_close(context);
        } catch (const std::exception& e) {
            log_message("策略 " + strategy->get_name() + " 收盘后处理错误: " + e.what(), "ERROR");
        }
    }
}

void UnifiedBacktestEngine::process_orders() {
    // 简化实现：订单在账户中已经处理
    // 在真实实现中，这里会处理撮合引擎的订单
}

void UnifiedBacktestEngine::record_daily_performance() {
    std::lock_guard<std::mutex> lock(results_mutex_);

    double total_value = account_->get_total_value();
    daily_equity_.push_back(total_value);
    results_.equity_curve.push_back(total_value);
    trade_dates_.push_back(current_date_);

    // 记录账户状态变化
    account_->calculate_pnl();
}

void UnifiedBacktestEngine::calculate_performance_metrics() {
    std::lock_guard<std::mutex> lock(results_mutex_);

    if (daily_equity_.size() < 2) {
        log_message("数据不足，无法计算性能指标", "WARNING");
        return;
    }

    // 基本指标
    results_.final_value = daily_equity_.back();
    results_.total_return = (results_.final_value - config_.initial_cash) / config_.initial_cash;
    results_.annual_return = calculate_annual_return();
    results_.volatility = calculate_volatility();
    results_.max_drawdown = calculate_max_drawdown();
    results_.downside_deviation = calculate_downside_deviation();

    // 风险调整收益指标
    results_.sharpe_ratio = calculate_sharpe_ratio();
    results_.sortino_ratio = calculate_sortino_ratio();
    results_.calmar_ratio = calculate_calmar_ratio();

    // 计算每日收益率
    results_.daily_returns = calculate_returns(daily_equity_);

    // 交易统计
    auto filled_orders = account_->get_filled_orders();
    results_.total_trades = static_cast<int>(filled_orders.size());

    // 简化的胜率计算
    results_.win_rate = calculate_win_rate();
    results_.profit_factor = calculate_profit_factor();

    // 基准比较指标（如果有基准数据）
    results_.benchmark_returns = load_benchmark_data();
    if (!results_.benchmark_returns.empty() && results_.benchmark_returns.size() == results_.daily_returns.size()) {
        results_.beta = calculate_beta();
        results_.alpha = calculate_alpha();
        results_.information_ratio = calculate_information_ratio();
    }

    // 风险指标
    results_.var_95 = calculate_var_95();
    results_.cvar_95 = calculate_cvar_95();

    // 复制交易日期
    results_.trade_dates = trade_dates_;
}

double UnifiedBacktestEngine::calculate_sharpe_ratio() const {
    auto returns = calculate_returns(daily_equity_);
    return unified_utils::calculate_sharpe_ratio(returns, 0.0);
}

double UnifiedBacktestEngine::calculate_sortino_ratio() const {
    auto returns = calculate_returns(daily_equity_);
    return unified_utils::calculate_sortino_ratio(returns, 0.0);
}

double UnifiedBacktestEngine::calculate_calmar_ratio() const {
    double annual_return = calculate_annual_return();
    double max_dd = calculate_max_drawdown();
    return unified_utils::calculate_calmar_ratio(annual_return, max_dd);
}

double UnifiedBacktestEngine::calculate_max_drawdown() const {
    return unified_utils::calculate_max_drawdown(daily_equity_);
}

double UnifiedBacktestEngine::calculate_annual_return() const {
    return unified_utils::calculate_annual_return(daily_equity_, 252);
}

double UnifiedBacktestEngine::calculate_volatility() const {
    auto returns = calculate_returns(daily_equity_);
    return unified_utils::calculate_volatility(returns, true);
}

double UnifiedBacktestEngine::calculate_downside_deviation() const {
    auto returns = calculate_returns(daily_equity_);
    return unified_utils::calculate_downside_deviation(returns, 0.0);
}

double UnifiedBacktestEngine::calculate_win_rate() const {
    // 简化实现
    return 0.6; // 假设60%胜率
}

double UnifiedBacktestEngine::calculate_profit_factor() const {
    // 简化实现
    return 1.5; // 假设1.5盈亏比
}

double UnifiedBacktestEngine::calculate_beta() const {
    if (results_.benchmark_returns.empty()) return 0.0;
    return unified_utils::calculate_beta(results_.daily_returns, results_.benchmark_returns);
}

double UnifiedBacktestEngine::calculate_alpha() const {
    if (results_.benchmark_returns.empty()) return 0.0;
    return unified_utils::calculate_alpha(results_.daily_returns, results_.benchmark_returns, 0.0);
}

double UnifiedBacktestEngine::calculate_var_95() const {
    return unified_utils::calculate_var_95(results_.daily_returns);
}

double UnifiedBacktestEngine::calculate_cvar_95() const {
    return unified_utils::calculate_cvar_95(results_.daily_returns);
}

double UnifiedBacktestEngine::calculate_information_ratio() const {
    if (results_.benchmark_returns.empty()) return 0.0;
    return unified_utils::calculate_information_ratio(results_.daily_returns, results_.benchmark_returns);
}

std::vector<double> UnifiedBacktestEngine::calculate_returns(const std::vector<double>& prices) const {
    if (prices.size() < 2) return {};

    std::vector<double> returns;
    returns.reserve(prices.size() - 1);

    for (size_t i = 1; i < prices.size(); ++i) {
        returns.push_back((prices[i] - prices[i-1]) / prices[i-1]);
    }

    return returns;
}

std::vector<double> UnifiedBacktestEngine::load_benchmark_data() const {
    // 简化实现：生成基准数据
    std::vector<double> benchmark;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(0.0005, 0.015); // 年化8%收益，15%波动

    for (size_t i = 0; i < daily_equity_.size() - 1; ++i) {
        benchmark.push_back(dist(gen));
    }

    return benchmark;
}

void UnifiedBacktestEngine::log_message(const std::string& message, const std::string& level) const {
    if (config_.enable_logging) {
        std::cout << "[" << level << "][" << current_date_ << "] " << message << std::endl;
    }
}

bool UnifiedBacktestEngine::save_results(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(results_mutex_);

    nlohmann::json result_json;
    result_json["config"] = config_.to_json();
    result_json["results"] = results_.to_json();

    std::ofstream file(filename);
    if (file.is_open()) {
        file << result_json.dump(4);
        return true;
    }

    return false;
}

std::unordered_map<std::string, double> UnifiedBacktestEngine::get_performance_summary() const {
    std::lock_guard<std::mutex> lock(results_mutex_);

    return {
        {"总收益率", results_.total_return},
        {"年化收益率", results_.annual_return},
        {"夏普比率", results_.sharpe_ratio},
        {"索提诺比率", results_.sortino_ratio},
        {"卡尔玛比率", results_.calmar_ratio},
        {"最大回撤", results_.max_drawdown},
        {"波动率", results_.volatility},
        {"下行偏差", results_.downside_deviation},
        {"胜率", results_.win_rate},
        {"盈亏比", results_.profit_factor},
        {"交易次数", static_cast<double>(results_.total_trades)},
        {"最终价值", results_.final_value},
        {"Beta系数", results_.beta},
        {"Alpha系数", results_.alpha},
        {"VaR(95%)", results_.var_95},
        {"CVaR(95%)", results_.cvar_95},
        {"信息比率", results_.information_ratio}
    };
}

std::vector<std::pair<std::string, double>> UnifiedBacktestEngine::get_equity_curve() const {
    std::lock_guard<std::mutex> lock(results_mutex_);

    std::vector<std::pair<std::string, double>> curve_data;

    for (size_t i = 0; i < std::min(trade_dates_.size(), results_.equity_curve.size()); ++i) {
        curve_data.emplace_back(trade_dates_[i], results_.equity_curve[i]);
    }

    return curve_data;
}

std::unordered_map<std::string, std::vector<double>> UnifiedBacktestEngine::get_trade_analysis() const {
    std::lock_guard<std::mutex> lock(results_mutex_);

    std::unordered_map<std::string, std::vector<double>> analysis;

    auto filled_orders = account_->get_filled_orders();

    std::vector<double> trade_amounts;
    std::vector<double> trade_prices;
    std::vector<double> trade_volumes;

    for (const auto& order : filled_orders) {
        trade_amounts.push_back(order.volume_orign * order.price_order);
        trade_prices.push_back(order.price_order);
        trade_volumes.push_back(order.volume_orign);
    }

    analysis["trade_amounts"] = trade_amounts;
    analysis["trade_prices"] = trade_prices;
    analysis["trade_volumes"] = trade_volumes;

    return analysis;
}

void UnifiedBacktestEngine::set_on_trade_callback(std::function<void(const protocol::tifi::Trade&)> callback) {
    on_trade_callback_ = callback;
}

void UnifiedBacktestEngine::set_on_order_callback(std::function<void(const protocol::tifi::Order&)> callback) {
    on_order_callback_ = callback;
}

protocol::qifi::QIFI UnifiedBacktestEngine::get_current_account_state() const {
    return account_->get_qifi();
}

void UnifiedBacktestEngine::stop() {
    is_running_ = false;
    log_message("回测引擎停止");
}

void UnifiedBacktestEngine::on_market_data_event(std::shared_ptr<Event> event) {
    auto market_event = std::dynamic_pointer_cast<MarketDataEvent>(event);
    if (!market_event) return;

    // 处理市场数据事件
    // 在实际实现中，这里可以触发策略的市场数据处理
}

void UnifiedBacktestEngine::on_order_event(std::shared_ptr<Event> event) {
    auto order_event = std::dynamic_pointer_cast<OrderEvent>(event);
    if (!order_event) return;

    if (on_order_callback_) {
        on_order_callback_(order_event->order);
    }
}

void UnifiedBacktestEngine::on_trade_event(std::shared_ptr<Event> event) {
    auto trade_event = std::dynamic_pointer_cast<TradeEvent>(event);
    if (!trade_event) return;

    trade_history_.push_back(trade_event->trade);

    if (on_trade_callback_) {
        on_trade_callback_(trade_event->trade);
    }
}

// ==================== 工具函数实现 ====================

namespace unified_utils {

double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double excess_return = mean_return - risk_free_rate / 252.0; // 日化无风险利率

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean_return) * (ret - mean_return);
    }
    variance /= returns.size();

    double std_dev = std::sqrt(variance);
    return (std_dev > 0) ? excess_return / std_dev * std::sqrt(252.0) : 0.0;
}

double calculate_sortino_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double excess_return = mean_return - risk_free_rate / 252.0;

    double downside_variance = 0.0;
    int downside_count = 0;

    for (double ret : returns) {
        if (ret < risk_free_rate / 252.0) {
            double diff = ret - risk_free_rate / 252.0;
            downside_variance += diff * diff;
            downside_count++;
        }
    }

    if (downside_count == 0) return 0.0;

    downside_variance /= downside_count;
    double downside_deviation = std::sqrt(downside_variance);

    return (downside_deviation > 0) ? excess_return / downside_deviation * std::sqrt(252.0) : 0.0;
}

double calculate_calmar_ratio(double annual_return, double max_drawdown) {
    return (max_drawdown > 0) ? annual_return / max_drawdown : 0.0;
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

double calculate_downside_deviation(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double target_return = risk_free_rate / 252.0; // 日化目标收益
    double downside_variance = 0.0;
    int downside_count = 0;

    for (double ret : returns) {
        if (ret < target_return) {
            double diff = ret - target_return;
            downside_variance += diff * diff;
            downside_count++;
        }
    }

    if (downside_count == 0) return 0.0;

    downside_variance /= downside_count;
    return std::sqrt(downside_variance) * std::sqrt(252.0);
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

    return (total_loss > 0) ? total_profit / total_loss : 0.0;
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

    return (benchmark_variance > 0) ? covariance / benchmark_variance : 0.0;
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

    double daily_risk_free = risk_free_rate / 252.0;

    return strategy_mean - (daily_risk_free + beta * (benchmark_mean - daily_risk_free));
}

double calculate_var_95(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;

    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());

    size_t index = static_cast<size_t>(std::ceil(returns.size() * 0.05)) - 1;
    if (index >= sorted_returns.size()) index = sorted_returns.size() - 1;

    return -sorted_returns[index]; // VaR通常表示为正值
}

double calculate_cvar_95(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;

    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());

    size_t threshold_index = static_cast<size_t>(std::ceil(returns.size() * 0.05));
    if (threshold_index == 0) threshold_index = 1;

    double sum = 0.0;
    for (size_t i = 0; i < threshold_index; ++i) {
        sum += sorted_returns[i];
    }

    return -sum / threshold_index; // CVaR通常表示为正值
}

double calculate_information_ratio(const std::vector<double>& strategy_returns,
                                  const std::vector<double>& benchmark_returns) {
    if (strategy_returns.size() != benchmark_returns.size() || strategy_returns.empty()) {
        return 0.0;
    }

    std::vector<double> excess_returns;
    for (size_t i = 0; i < strategy_returns.size(); ++i) {
        excess_returns.push_back(strategy_returns[i] - benchmark_returns[i]);
    }

    double mean_excess = std::accumulate(excess_returns.begin(), excess_returns.end(), 0.0) / excess_returns.size();

    double variance = 0.0;
    for (double excess : excess_returns) {
        variance += (excess - mean_excess) * (excess - mean_excess);
    }
    variance /= excess_returns.size();

    double tracking_error = std::sqrt(variance) * std::sqrt(252.0);

    return (tracking_error > 0) ? (mean_excess * 252.0) / tracking_error : 0.0;
}

std::vector<std::string> generate_trading_dates(const std::string& start_date, const std::string& end_date) {
    // 简化实现：生成日期序列
    std::vector<std::string> dates;
    // 实际实现应该考虑节假日和周末
    dates.push_back(start_date);
    dates.push_back(end_date);
    return dates;
}

bool is_trading_day(const std::string& date) {
    // 简化实现：假设都是交易日
    return true;
}

std::string add_trading_days(const std::string& date, int days) {
    // 简化实现：返回原日期
    return date;
}

} // namespace unified_utils

// ==================== 工厂函数实现 ====================

namespace unified_factory {

std::shared_ptr<UnifiedStrategy> create_sma_strategy(int fast_window, int slow_window, const std::string& name) {
    return std::make_shared<UnifiedSMAStrategy>(fast_window, slow_window, name);
}

std::shared_ptr<UnifiedStrategy> create_momentum_strategy(int lookback_window, double threshold, const std::string& name) {
    return std::make_shared<UnifiedMomentumStrategy>(lookback_window, threshold, name);
}

std::shared_ptr<UnifiedStrategy> create_mean_reversion_strategy(int window, double z_score_threshold, const std::string& name) {
    return std::make_shared<UnifiedMeanReversionStrategy>(window, z_score_threshold, name);
}

std::shared_ptr<UnifiedStrategy> create_strategy_from_config(const nlohmann::json& config) {
    if (!config.contains("type")) {
        throw std::runtime_error("策略配置中缺少type字段");
    }

    std::string type = config["type"];
    std::string name = config.value("name", type);

    if (type == "sma") {
        int fast_window = config.value("fast_window", 5);
        int slow_window = config.value("slow_window", 20);
        return create_sma_strategy(fast_window, slow_window, name);
    } else if (type == "momentum") {
        int lookback_window = config.value("lookback_window", 20);
        double threshold = config.value("threshold", 0.05);
        return create_momentum_strategy(lookback_window, threshold, name);
    } else if (type == "mean_reversion") {
        int window = config.value("window", 20);
        double z_score_threshold = config.value("z_score_threshold", 2.0);
        return create_mean_reversion_strategy(window, z_score_threshold, name);
    } else {
        throw std::runtime_error("未知的策略类型: " + type);
    }
}

std::vector<std::shared_ptr<UnifiedStrategy>> create_multi_strategy_portfolio(const nlohmann::json& config) {
    std::vector<std::shared_ptr<UnifiedStrategy>> strategies;

    if (!config.contains("strategies") || !config["strategies"].is_array()) {
        throw std::runtime_error("多策略配置中缺少strategies数组");
    }

    for (const auto& strategy_config : config["strategies"]) {
        auto strategy = create_strategy_from_config(strategy_config);
        strategies.push_back(strategy);
    }

    return strategies;
}

} // namespace unified_factory

} // namespace qaultra::engine