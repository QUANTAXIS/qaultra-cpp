#include "qaultra/account/account_full.hpp"
#include "qaultra/simd/simd_math.hpp"
#include "qaultra/util/datetime_utils.hpp"
#include "qaultra/util/uuid_generator.hpp"

#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <sstream>

namespace qaultra::account {

// QA_Account Implementation

QA_Account::QA_Account(
    const std::string& account_cookie,
    const std::string& portfolio_cookie,
    const std::string& user_cookie,
    double init_cash,
    bool auto_reload,
    const std::string& environment
) : account_cookie_(account_cookie)
  , portfolio_cookie_(portfolio_cookie)
  , user_cookie_(user_cookie)
  , init_cash_(init_cash)
  , auto_reload_(auto_reload)
  , environment_(environment)
  , cash_(init_cash)
  , frozen_cash_(0.0)
  , trade_id_counter_(0)
  , order_id_counter_(0)
  , settlement_engine_(std::make_unique<SettlementEngine>())
  , order_pool_(std::make_shared<memory::ObjectPool<Order>>(10000))
  , trade_pool_(std::make_shared<memory::ObjectPool<Trade>>(50000))
  , position_pool_(std::make_shared<memory::ObjectPool<Position>>(1000))
{
    // Initialize SIMD-aligned memory for calculations
    float_profit_.store(0.0);
    total_value_.store(init_cash);

    // Create initial cash position
    auto cash_position = create_position("CASH", 0.0, 0.0, init_cash);
    tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::accessor accessor;
    positions_.insert(accessor, std::make_pair("CASH", cash_position));
}

QA_Account::~QA_Account() = default;

std::shared_ptr<Order> QA_Account::buy(
    const std::string& code,
    double volume,
    const std::string& datetime,
    double price,
    const std::string& order_id
) {
    std::lock_guard<tbb::spin_mutex> lock(account_mutex_);

    // Validate order
    if (!validate_order(code, volume, price, Direction::BUY)) {
        throw std::invalid_argument("Order validation failed");
    }

    double required_cash = volume * price * (1.0 + get_commission_rate(code));
    if (cash_ < required_cash) {
        throw std::runtime_error("Insufficient cash for buy order");
    }

    // Create order
    auto order = create_order(
        order_id.empty() ? generate_order_id() : order_id,
        code, volume, price, Direction::BUY, datetime
    );

    // Execute immediately in backtest mode
    if (environment_ == "backtest" || environment_ == "real") {
        execute_order_immediate(order, datetime);
    } else {
        // In paper trading, add to pending orders
        pending_orders_[order->order_id] = order;
        frozen_cash_ += required_cash;
        cash_ -= required_cash;
    }

    orders_[order->order_id] = order;
    return order;
}

std::shared_ptr<Order> QA_Account::sell(
    const std::string& code,
    double volume,
    const std::string& datetime,
    double price,
    const std::string& order_id
) {
    std::lock_guard<tbb::spin_mutex> lock(account_mutex_);

    // Check position availability
    auto position = get_position(code);
    if (!position || position->volume_long < volume) {
        throw std::runtime_error("Insufficient position for sell order");
    }

    auto order = create_order(
        order_id.empty() ? generate_order_id() : order_id,
        code, volume, price, Direction::SELL, datetime
    );

    if (environment_ == "backtest" || environment_ == "real") {
        execute_order_immediate(order, datetime);
    } else {
        pending_orders_[order->order_id] = order;
        // Freeze position
        position->frozen_volume += volume;
    }

    orders_[order->order_id] = order;
    return order;
}

std::shared_ptr<Order> QA_Account::buy_open(
    const std::string& code,
    double volume,
    const std::string& datetime,
    double price,
    const std::string& order_id
) {
    return futures_trade(code, volume, datetime, price, Direction::BUY,
                        PositionEffect::OPEN, order_id);
}

std::shared_ptr<Order> QA_Account::sell_open(
    const std::string& code,
    double volume,
    const std::string& datetime,
    double price,
    const std::string& order_id
) {
    return futures_trade(code, volume, datetime, price, Direction::SELL,
                        PositionEffect::OPEN, order_id);
}

std::shared_ptr<Order> QA_Account::buy_close(
    const std::string& code,
    double volume,
    const std::string& datetime,
    double price,
    const std::string& order_id
) {
    return futures_trade(code, volume, datetime, price, Direction::BUY,
                        PositionEffect::CLOSE, order_id);
}

std::shared_ptr<Order> QA_Account::sell_close(
    const std::string& code,
    double volume,
    const std::string& datetime,
    double price,
    const std::string& order_id
) {
    return futures_trade(code, volume, datetime, price, Direction::SELL,
                        PositionEffect::CLOSE, order_id);
}

void QA_Account::on_price_change(
    const std::string& code,
    double price,
    const std::string& datetime
) {
    tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::accessor accessor;
    if (positions_.find(accessor, code)) {
        auto position = accessor->second;

        // Update position market value using SIMD
        double old_market_value = position->market_value;
        position->price = price;
        position->market_value = simd::vectorized_multiply_scalar(
            &position->volume_long, &price, 1
        )[0];

        // Update last datetime
        position->last_datetime = datetime;

        // Recalculate float profit
        calculate_float_profit();
    }
}

double QA_Account::get_balance() const {
    return cash_ + frozen_cash_;
}

double QA_Account::get_cash() const {
    return cash_;
}

double QA_Account::get_frozen_cash() const {
    return frozen_cash_;
}

double QA_Account::get_market_value() const {
    double total_market_value = 0.0;

    // Use TBB parallel reduce for large position counts
    total_market_value = tbb::parallel_reduce(
        tbb::blocked_range<typename tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::const_iterator>(
            positions_.begin(), positions_.end()
        ),
        0.0,
        [](const auto& range, double init) {
            return std::accumulate(
                range.begin(), range.end(), init,
                [](double sum, const auto& pair) {
                    const auto& position = pair.second;
                    return sum + position->market_value;
                }
            );
        },
        std::plus<double>()
    );

    return total_market_value;
}

double QA_Account::get_float_profit() const {
    return float_profit_.load();
}

double QA_Account::get_total_value() const {
    return total_value_.load();
}

std::shared_ptr<Position> QA_Account::get_position(const std::string& code) const {
    tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::const_accessor accessor;
    if (positions_.find(accessor, code)) {
        return accessor->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Position>> QA_Account::get_positions() const {
    std::vector<std::shared_ptr<Position>> result;
    result.reserve(positions_.size());

    for (const auto& pair : positions_) {
        if (pair.second->volume_long > 0 || pair.second->volume_short > 0) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<std::shared_ptr<Order>> QA_Account::get_orders() const {
    std::vector<std::shared_ptr<Order>> result;
    result.reserve(orders_.size());

    for (const auto& pair : orders_) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<std::shared_ptr<Trade>> QA_Account::get_trades() const {
    std::vector<std::shared_ptr<Trade>> result;
    result.reserve(trades_.size());

    for (const auto& trade : trades_) {
        result.push_back(trade);
    }

    return result;
}

protocol::QIFIAccount QA_Account::to_qifi() const {
    protocol::QIFIAccount qifi;

    qifi.account_cookie = account_cookie_;
    qifi.portfolio_cookie = portfolio_cookie_;
    qifi.user_cookie = user_cookie_;
    qifi.init_cash = init_cash_;
    qifi.cash = cash_;
    qifi.frozen_cash = frozen_cash_;
    qifi.balance = get_balance();
    qifi.market_value = get_market_value();
    qifi.float_profit = get_float_profit();
    qifi.total_value = get_total_value();

    // Convert positions
    for (const auto& pair : positions_) {
        const auto& position = pair.second;
        if (position->volume_long > 0 || position->volume_short > 0) {
            protocol::QIFIPosition qifi_pos;
            qifi_pos.code = position->code;
            qifi_pos.volume_long = position->volume_long;
            qifi_pos.volume_short = position->volume_short;
            qifi_pos.price = position->price;
            qifi_pos.cost_long = position->cost_long;
            qifi_pos.cost_short = position->cost_short;
            qifi_pos.market_value = position->market_value;
            qifi_pos.float_profit = position->float_profit;
            qifi_pos.last_datetime = position->last_datetime;

            qifi.positions.push_back(qifi_pos);
        }
    }

    // Convert orders
    for (const auto& pair : orders_) {
        const auto& order = pair.second;
        protocol::QIFIOrder qifi_order;
        qifi_order.order_id = order->order_id;
        qifi_order.code = order->code;
        qifi_order.volume = order->volume;
        qifi_order.price = order->price;
        qifi_order.direction = static_cast<int>(order->direction);
        qifi_order.status = static_cast<int>(order->status);
        qifi_order.datetime = order->datetime;
        qifi_order.trade_volume = order->trade_volume;
        qifi_order.trade_price = order->trade_price;

        qifi.orders.push_back(qifi_order);
    }

    return qifi;
}

void QA_Account::from_qifi(const protocol::QIFIAccount& qifi) {
    // Clear existing data
    positions_.clear();
    orders_.clear();
    trades_.clear();

    // Restore account data
    account_cookie_ = qifi.account_cookie;
    portfolio_cookie_ = qifi.portfolio_cookie;
    user_cookie_ = qifi.user_cookie;
    init_cash_ = qifi.init_cash;
    cash_ = qifi.cash;
    frozen_cash_ = qifi.frozen_cash;

    // Restore positions
    for (const auto& qifi_pos : qifi.positions) {
        auto position = create_position(
            qifi_pos.code,
            qifi_pos.volume_long,
            qifi_pos.volume_short,
            qifi_pos.price
        );
        position->cost_long = qifi_pos.cost_long;
        position->cost_short = qifi_pos.cost_short;
        position->market_value = qifi_pos.market_value;
        position->float_profit = qifi_pos.float_profit;
        position->last_datetime = qifi_pos.last_datetime;

        tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::accessor accessor;
        positions_.insert(accessor, std::make_pair(qifi_pos.code, position));
    }

    // Restore orders
    for (const auto& qifi_order : qifi.orders) {
        auto order = create_order(
            qifi_order.order_id,
            qifi_order.code,
            qifi_order.volume,
            qifi_order.price,
            static_cast<Direction>(qifi_order.direction),
            qifi_order.datetime
        );
        order->status = static_cast<OrderStatus>(qifi_order.status);
        order->trade_volume = qifi_order.trade_volume;
        order->trade_price = qifi_order.trade_price;

        orders_[qifi_order.order_id] = order;
    }

    // Recalculate derived values
    calculate_float_profit();
}

// Private methods

std::shared_ptr<Order> QA_Account::create_order(
    const std::string& order_id,
    const std::string& code,
    double volume,
    double price,
    Direction direction,
    const std::string& datetime
) {
    auto order = order_pool_->acquire();
    order->order_id = order_id;
    order->account_id = account_cookie_;
    order->code = code;
    order->volume = volume;
    order->price = price;
    order->direction = direction;
    order->status = OrderStatus::PENDING;
    order->datetime = datetime;
    order->trade_volume = 0.0;
    order->trade_price = 0.0;
    order->commission = 0.0;

    return order;
}

std::shared_ptr<Position> QA_Account::create_position(
    const std::string& code,
    double volume_long,
    double volume_short,
    double price
) {
    auto position = position_pool_->acquire();
    position->code = code;
    position->volume_long = volume_long;
    position->volume_short = volume_short;
    position->frozen_volume = 0.0;
    position->price = price;
    position->cost_long = volume_long * price;
    position->cost_short = volume_short * price;
    position->market_value = (volume_long - volume_short) * price;
    position->float_profit = 0.0;
    position->last_datetime = utils::format_datetime(utils::now());

    return position;
}

std::shared_ptr<Trade> QA_Account::create_trade(
    const std::string& trade_id,
    const std::string& order_id,
    const std::string& code,
    double volume,
    double price,
    Direction direction,
    const std::string& datetime
) {
    auto trade = trade_pool_->acquire();
    trade->trade_id = trade_id;
    trade->order_id = order_id;
    trade->account_id = account_cookie_;
    trade->code = code;
    trade->volume = volume;
    trade->price = price;
    trade->direction = direction;
    trade->datetime = datetime;
    trade->commission = calculate_commission(code, volume, price);

    return trade;
}

void QA_Account::execute_order_immediate(
    std::shared_ptr<Order> order,
    const std::string& datetime
) {
    // Create trade
    auto trade = create_trade(
        generate_trade_id(),
        order->order_id,
        order->code,
        order->volume,
        order->price,
        order->direction,
        datetime
    );

    // Update order status
    order->status = OrderStatus::FILLED;
    order->trade_volume = order->volume;
    order->trade_price = order->price;
    order->commission = trade->commission;

    // Update position
    update_position(trade);

    // Store trade
    trades_.push_back(trade);

    // Settlement
    settlement_engine_->process_trade(trade);

    // Recalculate metrics
    calculate_float_profit();
}

void QA_Account::update_position(std::shared_ptr<Trade> trade) {
    tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::accessor accessor;

    if (!positions_.find(accessor, trade->code)) {
        // Create new position
        auto position = create_position(trade->code, 0.0, 0.0, trade->price);
        positions_.insert(accessor, std::make_pair(trade->code, position));
    }

    auto position = accessor->second;

    if (trade->direction == Direction::BUY) {
        position->volume_long += trade->volume;
        position->cost_long += trade->volume * trade->price + trade->commission;
        cash_ -= trade->volume * trade->price + trade->commission;
    } else {
        if (position->volume_long >= trade->volume) {
            // Close long position
            position->volume_long -= trade->volume;
            position->cost_long -= (position->cost_long / (position->volume_long + trade->volume)) * trade->volume;
        } else {
            // Open short position
            double short_volume = trade->volume - position->volume_long;
            position->volume_long = 0.0;
            position->cost_long = 0.0;
            position->volume_short += short_volume;
            position->cost_short += short_volume * trade->price + trade->commission;
        }
        cash_ += trade->volume * trade->price - trade->commission;
    }

    // Update market value
    position->market_value = (position->volume_long - position->volume_short) * trade->price;
    position->last_datetime = trade->datetime;
}

std::shared_ptr<Order> QA_Account::futures_trade(
    const std::string& code,
    double volume,
    const std::string& datetime,
    double price,
    Direction direction,
    PositionEffect effect,
    const std::string& order_id
) {
    std::lock_guard<tbb::spin_mutex> lock(account_mutex_);

    auto order = create_order(
        order_id.empty() ? generate_order_id() : order_id,
        code, volume, price, direction, datetime
    );

    // Set position effect
    order->position_effect = effect;

    if (environment_ == "backtest" || environment_ == "real") {
        execute_futures_order(order, datetime);
    } else {
        pending_orders_[order->order_id] = order;
        // Handle margin requirements for futures
        double margin = calculate_futures_margin(code, volume, price);
        frozen_cash_ += margin;
        cash_ -= margin;
    }

    orders_[order->order_id] = order;
    return order;
}

void QA_Account::execute_futures_order(
    std::shared_ptr<Order> order,
    const std::string& datetime
) {
    auto trade = create_trade(
        generate_trade_id(),
        order->order_id,
        order->code,
        order->volume,
        order->price,
        order->direction,
        datetime
    );

    // Update order
    order->status = OrderStatus::FILLED;
    order->trade_volume = order->volume;
    order->trade_price = order->price;
    order->commission = trade->commission;

    // Update futures position
    update_futures_position(trade, order->position_effect);

    trades_.push_back(trade);
    settlement_engine_->process_trade(trade);
    calculate_float_profit();
}

void QA_Account::update_futures_position(
    std::shared_ptr<Trade> trade,
    PositionEffect effect
) {
    tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::accessor accessor;

    if (!positions_.find(accessor, trade->code)) {
        auto position = create_position(trade->code, 0.0, 0.0, trade->price);
        positions_.insert(accessor, std::make_pair(trade->code, position));
    }

    auto position = accessor->second;

    if (effect == PositionEffect::OPEN) {
        if (trade->direction == Direction::BUY) {
            position->volume_long += trade->volume;
            position->cost_long += trade->volume * trade->price;
        } else {
            position->volume_short += trade->volume;
            position->cost_short += trade->volume * trade->price;
        }
    } else { // CLOSE
        if (trade->direction == Direction::BUY) {
            // Close short position
            double close_volume = std::min(position->volume_short, trade->volume);
            position->volume_short -= close_volume;
            position->cost_short -= (position->cost_short / (position->volume_short + close_volume)) * close_volume;
        } else {
            // Close long position
            double close_volume = std::min(position->volume_long, trade->volume);
            position->volume_long -= close_volume;
            position->cost_long -= (position->cost_long / (position->volume_long + close_volume)) * close_volume;
        }
    }

    // Update market value
    position->market_value = (position->volume_long - position->volume_short) * trade->price;
    position->last_datetime = trade->datetime;

    // Handle cash flow for futures (margin-based)
    double margin_change = calculate_futures_margin(trade->code, trade->volume, trade->price);
    if (effect == PositionEffect::OPEN) {
        cash_ -= margin_change + trade->commission;
    } else {
        cash_ += margin_change - trade->commission;
    }
}

void QA_Account::calculate_float_profit() {
    double total_float_profit = 0.0;

    // Use parallel reduce for performance
    total_float_profit = tbb::parallel_reduce(
        tbb::blocked_range<typename tbb::concurrent_hash_map<std::string, std::shared_ptr<Position>>::const_iterator>(
            positions_.begin(), positions_.end()
        ),
        0.0,
        [](const auto& range, double init) {
            return std::accumulate(
                range.begin(), range.end(), init,
                [](double sum, const auto& pair) {
                    const auto& position = pair.second;
                    double position_profit = position->market_value -
                        (position->cost_long - position->cost_short);
                    const_cast<Position*>(position.get())->float_profit = position_profit;
                    return sum + position_profit;
                }
            );
        },
        std::plus<double>()
    );

    float_profit_.store(total_float_profit);
    total_value_.store(get_balance() + get_market_value());
}

bool QA_Account::validate_order(
    const std::string& code,
    double volume,
    double price,
    Direction direction
) const {
    if (code.empty() || volume <= 0 || price <= 0) {
        return false;
    }

    // Additional validation logic
    return true;
}

double QA_Account::calculate_commission(
    const std::string& code,
    double volume,
    double price
) const {
    // Simple commission calculation - can be made more sophisticated
    double commission_rate = get_commission_rate(code);
    return volume * price * commission_rate;
}

double QA_Account::calculate_futures_margin(
    const std::string& code,
    double volume,
    double price
) const {
    // Simple margin calculation - typically 10-20% of notional value
    double margin_rate = get_margin_rate(code);
    return volume * price * margin_rate;
}

double QA_Account::get_commission_rate(const std::string& code) const {
    // Default commission rates - can be configured per symbol
    if (code.find("Future") != std::string::npos) {
        return 0.0003; // 0.03% for futures
    }
    return 0.0025; // 0.25% for stocks
}

double QA_Account::get_margin_rate(const std::string& code) const {
    // Default margin rates for futures
    return 0.15; // 15% margin requirement
}

std::string QA_Account::generate_order_id() {
    return utils::generate_uuid() + "_" + std::to_string(++order_id_counter_);
}

std::string QA_Account::generate_trade_id() {
    return utils::generate_uuid() + "_" + std::to_string(++trade_id_counter_);
}

// SettlementEngine Implementation

void SettlementEngine::process_trade(std::shared_ptr<Trade> trade) {
    std::lock_guard<std::mutex> lock(settlement_mutex_);

    pending_settlements_.push_back({
        trade,
        utils::now() + std::chrono::hours(24) // T+1 settlement
    });
}

void SettlementEngine::process_settlements() {
    std::lock_guard<std::mutex> lock(settlement_mutex_);

    auto now = utils::now();
    auto it = std::remove_if(
        pending_settlements_.begin(),
        pending_settlements_.end(),
        [now](const Settlement& settlement) {
            return settlement.settlement_date <= now;
        }
    );

    if (it != pending_settlements_.end()) {
        // Process settled trades
        for (auto settle_it = it; settle_it != pending_settlements_.end(); ++settle_it) {
            settled_trades_.push_back(settle_it->trade);
        }
        pending_settlements_.erase(it, pending_settlements_.end());
    }
}

std::vector<std::shared_ptr<Trade>> SettlementEngine::get_settled_trades() const {
    std::lock_guard<std::mutex> lock(settlement_mutex_);
    return settled_trades_;
}

} // namespace qaultra::account