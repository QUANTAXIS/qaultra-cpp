#include "../../include/qaultra/engine/backtest_engine.hpp"
#include "../../include/qaultra/util/uuid_generator.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace qaultra::engine {

// BacktestStats 实现
nlohmann::json BacktestStats::to_json() const {
    nlohmann::json j;
    j["total_return"] = total_return;
    j["annual_return"] = annual_return;
    j["max_drawdown"] = max_drawdown;
    j["sharpe_ratio"] = sharpe_ratio;
    j["sortino_ratio"] = sortino_ratio;
    j["calmar_ratio"] = calmar_ratio;
    j["volatility"] = volatility;
    j["downside_deviation"] = downside_deviation;
    j["win_rate"] = win_rate;
    j["profit_loss_ratio"] = profit_loss_ratio;
    j["total_trades"] = total_trades;
    j["win_trades"] = win_trades;
    j["loss_trades"] = loss_trades;
    j["start_date"] = start_date;
    j["end_date"] = end_date;
    j["trading_days"] = trading_days;
    return j;
}

// Strategy 基类实现
void Strategy::buy(const std::string& instrument, double volume, double price) {
    if (!engine_) return;

    protocol::tifi::Order order;
    order.order_id = qaultra::util::UUIDGenerator::generate_order_id();
    order.instrument_id = instrument;
    order.direction = protocol::tifi::Direction::BUY;
    order.offset = protocol::tifi::Offset::OPEN;
    order.volume = volume;
    order.price = price;
    order.price_type = (price > 0.0) ? protocol::tifi::PriceType::LIMIT : protocol::tifi::PriceType::MARKET;
    order.status = protocol::tifi::OrderStatus::PENDING;

    engine_->place_order(order);
}

void Strategy::sell(const std::string& instrument, double volume, double price) {
    if (!engine_) return;

    protocol::tifi::Order order;
    order.order_id = qaultra::util::UUIDGenerator::generate_order_id();
    order.instrument_id = instrument;
    order.direction = protocol::tifi::Direction::SELL;
    order.offset = protocol::tifi::Offset::CLOSE;
    order.volume = volume;
    order.price = price;
    order.price_type = (price > 0.0) ? protocol::tifi::PriceType::LIMIT : protocol::tifi::PriceType::MARKET;
    order.status = protocol::tifi::OrderStatus::PENDING;

    engine_->place_order(order);
}

void Strategy::buy_to_cover(const std::string& instrument, double volume, double price) {
    // 简化实现：与buy相同
    buy(instrument, volume, price);
}

void Strategy::sell_short(const std::string& instrument, double volume, double price) {
    // 简化实现：与sell相同
    sell(instrument, volume, price);
}

double Strategy::get_position(const std::string& instrument) const {
    if (!engine_) return 0.0;
    return engine_->get_position(instrument);
}

double Strategy::get_cash() const {
    if (!engine_) return 0.0;
    return engine_->get_cash();
}

double Strategy::get_total_value() const {
    if (!engine_) return 0.0;
    return engine_->get_total_value();
}

void Strategy::set_parameter(const std::string& key, const nlohmann::json& value) {
    parameters_[key] = value;
}

nlohmann::json Strategy::get_parameter(const std::string& key) const {
    auto it = parameters_.find(key);
    return (it != parameters_.end()) ? it->second : nlohmann::json();
}

// EventEngine 实现
void EventEngine::register_handler(EventType type, EventHandler handler) {
    handlers_[type].push_back(handler);
}

void EventEngine::put_event(std::shared_ptr<Event> event) {
    event_queue_.push(event);
}

void EventEngine::process_events() {
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
    while (!event_queue_.empty()) {
        event_queue_.pop();
    }
}

// DataManager 简化实现
bool DataManager::load_data(const std::string& instrument, const std::string& start_date,
                           const std::string& end_date, const std::string& frequency) {
    // 简化实现：生成模拟数据
    std::vector<protocol::mifi::Kline> klines;

    // 生成100个交易日的模拟数据
    double base_price = 100.0;
    for (int i = 0; i < 100; ++i) {
        protocol::mifi::Kline kline;
        kline.instrument_id = instrument;
        kline.exchange_id = "TEST";
        kline.market_type = protocol::mifi::MarketType::STOCK;

        // 简单的随机游走
        double change = (rand() % 200 - 100) / 10000.0; // -1% 到 +1%
        base_price *= (1.0 + change);

        kline.open = base_price;
        kline.high = base_price * (1.0 + rand() % 50 / 10000.0);
        kline.low = base_price * (1.0 - rand() % 50 / 10000.0);
        kline.close = kline.low + (kline.high - kline.low) * (rand() % 100) / 100.0;
        kline.volume = 1000000 + rand() % 5000000;
        kline.amount = kline.volume * (kline.open + kline.close) / 2.0;

        // 生成时间戳
        std::stringstream ss;
        ss << "2024-01-" << std::setfill('0') << std::setw(2) << ((i % 30) + 1) << "T09:30:00";
        kline.datetime = ss.str();

        klines.push_back(kline);
    }

    data_[instrument] = klines;
    data_index_[instrument] = 0;
    return true;
}

std::optional<protocol::mifi::Kline> DataManager::get_next_data(const std::string& instrument) {
    auto data_it = data_.find(instrument);
    if (data_it == data_.end()) {
        return std::nullopt;
    }

    auto index_it = data_index_.find(instrument);
    if (index_it == data_index_.end() || index_it->second >= data_it->second.size()) {
        return std::nullopt;
    }

    auto kline = data_it->second[index_it->second];
    index_it->second++;
    current_time_ = kline.datetime;
    return kline;
}

void DataManager::reset() {
    for (auto& [instrument, index] : data_index_) {
        index = 0;
    }
    current_time_.clear();
}

bool DataManager::has_more_data() const {
    for (const auto& [instrument, data] : data_) {
        auto index_it = data_index_.find(instrument);
        if (index_it != data_index_.end() && index_it->second < data.size()) {
            return true;
        }
    }
    return false;
}

std::string DataManager::get_current_time() const {
    return current_time_;
}

// Portfolio 简化实现
bool Portfolio::process_order(const protocol::tifi::Order& order) {
    // 简化实现：直接成交
    return true;
}

void Portfolio::process_trade(const protocol::tifi::Trade& trade) {
    const std::string& instrument = trade.instrument_id;
    double volume = trade.volume;
    double price = trade.price;

    if (trade.direction == protocol::tifi::Direction::BUY) {
        // 买入
        double cost = volume * price;
        if (cash_ >= cost) {
            cash_ -= cost;

            // 更新持仓均价
            double current_pos = positions_[instrument];
            double current_avg = avg_prices_[instrument];
            double new_pos = current_pos + volume;

            if (new_pos > 0) {
                avg_prices_[instrument] = (current_pos * current_avg + volume * price) / new_pos;
            }

            positions_[instrument] = new_pos;
        }
    } else {
        // 卖出
        double current_pos = positions_[instrument];
        if (current_pos >= volume) {
            cash_ += volume * price;
            positions_[instrument] = current_pos - volume;

            if (positions_[instrument] <= 0) {
                positions_[instrument] = 0;
                avg_prices_[instrument] = 0;
            }
        }
    }
}

void Portfolio::update_positions(const std::unordered_map<std::string, protocol::mifi::Kline>& market_data) {
    total_value_ = cash_;

    for (const auto& [instrument, position] : positions_) {
        if (position > 0) {
            auto market_it = market_data.find(instrument);
            if (market_it != market_data.end()) {
                total_value_ += position * market_it->second.close;
            } else {
                // 使用平均价作为当前价
                total_value_ += position * avg_prices_[instrument];
            }
        }
    }

    // 记录历史
    value_history_.push_back(total_value_);

    // 简化：使用当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    time_history_.push_back(ss.str());
}

double Portfolio::get_position(const std::string& instrument) const {
    auto it = positions_.find(instrument);
    return (it != positions_.end()) ? it->second : 0.0;
}

protocol::tifi::Account Portfolio::get_account_info() const {
    protocol::tifi::Account account;
    account.account_id = "BACKTEST_ACCOUNT";
    account.total_asset = total_value_;
    account.available_cash = cash_;
    account.realized_pnl = total_value_ - initial_capital_;
    return account;
}

std::vector<double> Portfolio::get_returns() const {
    if (value_history_.size() < 2) return {};

    std::vector<double> returns;
    for (size_t i = 1; i < value_history_.size(); ++i) {
        double ret = (value_history_[i] - value_history_[i-1]) / value_history_[i-1];
        returns.push_back(ret);
    }
    return returns;
}

// BacktestEngine 构造函数
BacktestEngine::BacktestEngine(const BacktestConfig& config)
    : config_(config) {
    event_engine_ = std::make_shared<EventEngine>();
    data_manager_ = std::make_shared<DataManager>();
    portfolio_ = std::make_shared<Portfolio>(config.initial_capital);
}

void BacktestEngine::add_strategy(std::shared_ptr<Strategy> strategy) {
    strategy->engine_ = this;
    strategies_.push_back(strategy);
}

bool BacktestEngine::add_data(const std::string& instrument, const std::string& start_date,
                             const std::string& end_date, const std::string& frequency) {
    return data_manager_->load_data(instrument, start_date, end_date, frequency);
}

bool BacktestEngine::run() {
    initialize();

    // 策略初始化
    for (auto& strategy : strategies_) {
        strategy->initialize();
    }

    // 主循环：处理市场数据
    while (data_manager_->has_more_data()) {
        process_market_data();
        event_engine_->process_events();
    }

    // 策略结束
    for (auto& strategy : strategies_) {
        strategy->finalize();
    }

    return true;
}

BacktestStats BacktestEngine::get_stats() const {
    return calculate_stats();
}

bool BacktestEngine::export_results(const std::string& output_dir) const {
    // 简化实现：仅返回成功
    return true;
}

void BacktestEngine::place_order(const protocol::tifi::Order& order) {
    // 简化实现：直接成交
    protocol::tifi::Trade trade;
    trade.trade_id = qaultra::util::UUIDGenerator::generate_trade_id();
    trade.order_id = order.order_id;
    trade.instrument_id = order.instrument_id;
    trade.direction = order.direction;
    trade.offset = order.offset;
    trade.volume = order.volume;
    trade.price = order.price;

    portfolio_->process_trade(trade);
    trade_history_.push_back(trade);
}

double BacktestEngine::get_position(const std::string& instrument) const {
    return portfolio_->get_position(instrument);
}

double BacktestEngine::get_cash() const {
    return portfolio_->get_cash();
}

double BacktestEngine::get_total_value() const {
    return portfolio_->get_total_value();
}

void BacktestEngine::initialize() {
    // 注册事件处理器
    using namespace std::placeholders;
    event_engine_->register_handler(EventType::MARKET_DATA,
        std::bind(&BacktestEngine::on_market_data_event, this, _1));
    event_engine_->register_handler(EventType::ORDER,
        std::bind(&BacktestEngine::on_order_event, this, _1));
    event_engine_->register_handler(EventType::TRADE,
        std::bind(&BacktestEngine::on_trade_event, this, _1));
}

void BacktestEngine::process_market_data() {
    std::unordered_map<std::string, protocol::mifi::Kline> current_data;

    // 获取所有品种的当前数据
    for (auto& strategy : strategies_) {
        // 简化：假设每个策略只关注一个品种
        auto kline_opt = data_manager_->get_next_data("TEST_INSTRUMENT");
        if (kline_opt) {
            current_data["TEST_INSTRUMENT"] = *kline_opt;

            // 发送市场数据事件
            auto event = std::make_shared<MarketDataEvent>(kline_opt->datetime, *kline_opt);
            event_engine_->put_event(event);
        }
    }

    // 更新组合
    portfolio_->update_positions(current_data);
}

void BacktestEngine::on_market_data_event(std::shared_ptr<Event> event) {
    auto market_event = std::dynamic_pointer_cast<MarketDataEvent>(event);
    if (!market_event) return;

    // 将市场数据发送给所有策略
    for (auto& strategy : strategies_) {
        strategy->on_market_data(market_event->kline);
    }
}

void BacktestEngine::on_order_event(std::shared_ptr<Event> event) {
    // 简化实现
}

void BacktestEngine::on_trade_event(std::shared_ptr<Event> event) {
    // 简化实现
}

BacktestStats BacktestEngine::calculate_stats() const {
    BacktestStats stats;

    auto returns = portfolio_->get_returns();
    if (returns.empty()) return stats;

    // 基本统计
    stats.start_date = config_.start_date;
    stats.end_date = config_.end_date;
    stats.trading_days = returns.size();
    stats.total_trades = trade_history_.size();

    // 总收益率
    double initial = config_.initial_capital;
    double final = portfolio_->get_total_value();
    stats.total_return = (final - initial) / initial;

    // 年化收益率
    stats.annual_return = utils::calculate_annual_return(stats.total_return, stats.trading_days);

    // 最大回撤
    stats.max_drawdown = utils::calculate_max_drawdown(portfolio_->value_history_);

    // 波动率
    stats.volatility = utils::calculate_volatility(returns);

    // 夏普比率
    stats.sharpe_ratio = utils::calculate_sharpe_ratio(returns);

    return stats;
}

// SimpleMovingAverageStrategy 实现
void SimpleMovingAverageStrategy::initialize() {
    std::cout << "初始化简单移动平均策略: " << name_ << std::endl;
}

void SimpleMovingAverageStrategy::on_market_data(const protocol::mifi::Kline& kline) {
    const std::string& instrument = kline.instrument_id;

    // 更新价格历史
    price_history_[instrument].push_back(kline.close);

    // 保持窗口大小
    if (price_history_[instrument].size() > static_cast<size_t>(long_window_)) {
        price_history_[instrument].erase(price_history_[instrument].begin());
    }

    // 计算移动平均线
    if (price_history_[instrument].size() >= static_cast<size_t>(long_window_)) {
        double short_ma = calculate_sma(price_history_[instrument], short_window_);
        double long_ma = calculate_sma(price_history_[instrument], long_window_);

        bool current_position = in_position_[instrument];

        // 金叉买入
        if (!current_position && short_ma > long_ma) {
            buy(instrument, 1000, kline.close);
            in_position_[instrument] = true;
        }
        // 死叉卖出
        else if (current_position && short_ma < long_ma) {
            sell(instrument, 1000, kline.close);
            in_position_[instrument] = false;
        }
    }
}

double SimpleMovingAverageStrategy::calculate_sma(const std::vector<double>& prices, int window) const {
    if (prices.size() < static_cast<size_t>(window)) {
        return 0.0;
    }

    double sum = 0.0;
    for (int i = prices.size() - window; i < static_cast<int>(prices.size()); ++i) {
        sum += prices[i];
    }
    return sum / window;
}

// 工具函数实现
namespace utils {

std::chrono::system_clock::time_point parse_date(const std::string& date_str) {
    // 简化实现
    return std::chrono::system_clock::now();
}

std::string format_date(const std::chrono::system_clock::time_point& time_point) {
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d");
    return ss.str();
}

int calculate_trading_days(const std::string& start_date, const std::string& end_date) {
    // 简化实现：假设252个交易日/年
    return 252;
}

double calculate_annual_return(double total_return, int trading_days) {
    if (trading_days <= 0) return 0.0;
    double years = trading_days / 252.0;
    return std::pow(1.0 + total_return, 1.0 / years) - 1.0;
}

double calculate_max_drawdown(const std::vector<double>& values) {
    if (values.size() < 2) return 0.0;

    double max_dd = 0.0;
    double peak = values[0];

    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i] > peak) {
            peak = values[i];
        } else {
            double dd = (peak - values[i]) / peak;
            max_dd = std::max(max_dd, dd);
        }
    }

    return max_dd;
}

double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean_return) * (ret - mean_return);
    }
    variance /= returns.size();
    double volatility = std::sqrt(variance);

    if (volatility == 0.0) return 0.0;

    return (mean_return - risk_free_rate) / volatility * std::sqrt(252.0);
}

double calculate_sortino_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double downside_variance = 0.0;
    int downside_count = 0;
    for (double ret : returns) {
        if (ret < risk_free_rate) {
            downside_variance += (ret - risk_free_rate) * (ret - risk_free_rate);
            downside_count++;
        }
    }

    if (downside_count == 0) return 0.0;

    downside_variance /= downside_count;
    double downside_deviation = std::sqrt(downside_variance);

    if (downside_deviation == 0.0) return 0.0;

    return (mean_return - risk_free_rate) / downside_deviation * std::sqrt(252.0);
}

double calculate_volatility(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean_return) * (ret - mean_return);
    }
    variance /= returns.size();

    return std::sqrt(variance) * std::sqrt(252.0);
}

} // namespace utils

} // namespace qaultra::engine