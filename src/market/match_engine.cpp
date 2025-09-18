#include "qaultra/market/match_engine.hpp"
#include "qaultra/util/uuid_generator.hpp"
#include "qaultra/util/datetime_utils.hpp"

#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace qaultra::market {

// PriceLevel Implementation

void PriceLevel::add_order(std::shared_ptr<account::Order> order) {
    auto node = new OrderNode{order, nullptr, nullptr};

    if (!head) {
        head = tail = node;
    } else {
        tail->next = node;
        node->prev = tail;
        tail = node;
    }

    total_volume += order->volume;
    order_count++;
}

bool PriceLevel::remove_order(const std::string& order_id) {
    OrderNode* current = head;

    while (current) {
        if (current->order->order_id == order_id) {
            // Update volume and count
            total_volume -= current->order->volume;
            order_count--;

            // Remove from linked list
            if (current->prev) {
                current->prev->next = current->next;
            } else {
                head = current->next;
            }

            if (current->next) {
                current->next->prev = current->prev;
            } else {
                tail = current->prev;
            }

            delete current;
            return true;
        }
        current = current->next;
    }

    return false;
}

void PriceLevel::clear() {
    OrderNode* current = head;
    while (current) {
        OrderNode* next = current->next;
        delete current;
        current = next;
    }

    head = tail = nullptr;
    total_volume = 0.0;
    order_count = 0;
}

// OrderBook Implementation

OrderBook::OrderBook()
    : node_pool_(std::make_shared<memory::ObjectPool<PriceLevel::OrderNode>>(100000))
{
}

OrderBook::~OrderBook() {
    clear();
}

bool OrderBook::add_order(std::shared_ptr<account::Order> order) {
    if (!order || order->volume <= 0 || order->price <= 0) {
        return false;
    }

    uint64_t price_key = price_to_key(order->price);
    PriceLevel* level = get_or_create_level(price_key, order->direction);

    if (!level) {
        return false;
    }

    // Add order to lookup
    tbb::concurrent_hash_map<std::string, std::shared_ptr<account::Order>>::accessor accessor;
    order_lookup_.insert(accessor, std::make_pair(order->order_id, order));

    // Add to price level
    level->add_order(order);

    // Update statistics
    total_orders_++;
    total_volume_ += static_cast<uint64_t>(order->volume);

    // Update best levels
    update_best_levels();

    return true;
}

bool OrderBook::remove_order(const std::string& order_id) {
    tbb::concurrent_hash_map<std::string, std::shared_ptr<account::Order>>::accessor accessor;
    if (!order_lookup_.find(accessor, order_id)) {
        return false;
    }

    auto order = accessor->second;
    uint64_t price_key = price_to_key(order->price);

    // Remove from price level
    bool removed = false;
    if (order->direction == account::Direction::BUY) {
        PriceLevelMap::accessor level_accessor;
        if (buy_levels_.find(level_accessor, price_key)) {
            removed = level_accessor->second->remove_order(order_id);
            if (level_accessor->second->get_count() == 0) {
                buy_levels_.erase(level_accessor);
            }
        }
    } else {
        PriceLevelMap::accessor level_accessor;
        if (sell_levels_.find(level_accessor, price_key)) {
            removed = level_accessor->second->remove_order(order_id);
            if (level_accessor->second->get_count() == 0) {
                sell_levels_.erase(level_accessor);
            }
        }
    }

    if (removed) {
        // Remove from lookup
        order_lookup_.erase(accessor);

        // Update statistics
        total_orders_--;
        total_volume_ -= static_cast<uint64_t>(order->volume);

        // Update best levels
        update_best_levels();
    }

    return removed;
}

bool OrderBook::modify_order(const std::string& order_id, double new_price, double new_volume) {
    // Remove old order
    tbb::concurrent_hash_map<std::string, std::shared_ptr<account::Order>>::accessor accessor;
    if (!order_lookup_.find(accessor, order_id)) {
        return false;
    }

    auto order = accessor->second;

    // Remove from current price level
    if (!remove_order(order_id)) {
        return false;
    }

    // Update order
    order->price = new_price;
    order->volume = new_volume;

    // Re-add with new price/volume
    return add_order(order);
}

std::shared_ptr<account::Order> OrderBook::get_order(const std::string& order_id) const {
    tbb::concurrent_hash_map<std::string, std::shared_ptr<account::Order>>::const_accessor accessor;
    if (order_lookup_.find(accessor, order_id)) {
        return accessor->second;
    }
    return nullptr;
}

std::pair<double, double> OrderBook::get_best_bid() const {
    return {key_to_price(best_bid_price_.load()), best_bid_volume_.load()};
}

std::pair<double, double> OrderBook::get_best_ask() const {
    uint64_t ask_price = best_ask_price_.load();
    if (ask_price == UINT64_MAX) {
        return {0.0, 0.0};
    }
    return {key_to_price(ask_price), best_ask_volume_.load()};
}

double OrderBook::get_spread() const {
    auto bid = get_best_bid();
    auto ask = get_best_ask();

    if (bid.first > 0 && ask.first > 0) {
        return ask.first - bid.first;
    }
    return 0.0;
}

double OrderBook::get_mid_price() const {
    auto bid = get_best_bid();
    auto ask = get_best_ask();

    if (bid.first > 0 && ask.first > 0) {
        return (bid.first + ask.first) / 2.0;
    } else if (bid.first > 0) {
        return bid.first;
    } else if (ask.first > 0) {
        return ask.first;
    }
    return 0.0;
}

std::pair<double, double> OrderBook::get_last_trade() const {
    return {key_to_price(last_trade_price_.load()), last_trade_volume_.load()};
}

std::vector<OrderBook::DepthLevel> OrderBook::get_depth_bids(size_t levels) const {
    std::vector<DepthLevel> result;
    result.reserve(levels);

    // Collect all bid levels and sort by price descending
    std::vector<std::pair<uint64_t, std::unique_ptr<PriceLevel>*>> bid_levels;
    for (auto it = buy_levels_.begin(); it != buy_levels_.end(); ++it) {
        bid_levels.emplace_back(it->first, &it->second);
    }

    // Sort by price descending (highest first)
    tbb::parallel_sort(bid_levels.begin(), bid_levels.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    size_t count = std::min(levels, bid_levels.size());
    for (size_t i = 0; i < count; ++i) {
        const auto& level = *bid_levels[i].second;
        result.push_back({
            key_to_price(bid_levels[i].first),
            level->get_volume(),
            level->get_count()
        });
    }

    return result;
}

std::vector<OrderBook::DepthLevel> OrderBook::get_depth_asks(size_t levels) const {
    std::vector<DepthLevel> result;
    result.reserve(levels);

    // Collect all ask levels and sort by price ascending
    std::vector<std::pair<uint64_t, std::unique_ptr<PriceLevel>*>> ask_levels;
    for (auto it = sell_levels_.begin(); it != sell_levels_.end(); ++it) {
        ask_levels.emplace_back(it->first, &it->second);
    }

    // Sort by price ascending (lowest first)
    tbb::parallel_sort(ask_levels.begin(), ask_levels.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    size_t count = std::min(levels, ask_levels.size());
    for (size_t i = 0; i < count; ++i) {
        const auto& level = *ask_levels[i].second;
        result.push_back({
            key_to_price(ask_levels[i].first),
            level->get_volume(),
            level->get_count()
        });
    }

    return result;
}

std::pair<std::vector<OrderBook::DepthLevel>, std::vector<OrderBook::DepthLevel>>
OrderBook::get_full_depth() const {
    return {get_depth_bids(SIZE_MAX), get_depth_asks(SIZE_MAX)};
}

void OrderBook::clear() {
    buy_levels_.clear();
    sell_levels_.clear();
    order_lookup_.clear();

    best_bid_price_.store(0);
    best_bid_volume_.store(0.0);
    best_ask_price_.store(UINT64_MAX);
    best_ask_volume_.store(0.0);

    total_orders_.store(0);
    total_volume_.store(0);
    last_trade_price_.store(0);
    last_trade_volume_.store(0.0);
}

void OrderBook::update_best_levels() {
    // Update best bid
    uint64_t best_bid = 0;
    double best_bid_vol = 0.0;
    for (const auto& pair : buy_levels_) {
        if (pair.first > best_bid && pair.second->get_volume() > 0) {
            best_bid = pair.first;
            best_bid_vol = pair.second->get_volume();
        }
    }
    best_bid_price_.store(best_bid);
    best_bid_volume_.store(best_bid_vol);

    // Update best ask
    uint64_t best_ask = UINT64_MAX;
    double best_ask_vol = 0.0;
    for (const auto& pair : sell_levels_) {
        if (pair.first < best_ask && pair.second->get_volume() > 0) {
            best_ask = pair.first;
            best_ask_vol = pair.second->get_volume();
        }
    }
    best_ask_price_.store(best_ask);
    best_ask_volume_.store(best_ask_vol);
}

PriceLevel* OrderBook::get_or_create_level(uint64_t price_key, account::Direction direction) {
    if (direction == account::Direction::BUY) {
        PriceLevelMap::accessor accessor;
        if (buy_levels_.insert(accessor, price_key)) {
            accessor->second = std::make_unique<PriceLevel>();
            accessor->second->price = key_to_price(price_key);
        }
        return accessor->second.get();
    } else {
        PriceLevelMap::accessor accessor;
        if (sell_levels_.insert(accessor, price_key)) {
            accessor->second = std::make_unique<PriceLevel>();
            accessor->second->price = key_to_price(price_key);
        }
        return accessor->second.get();
    }
}

void OrderBook::remove_empty_level(uint64_t price_key, account::Direction direction) {
    if (direction == account::Direction::BUY) {
        PriceLevelMap::accessor accessor;
        if (buy_levels_.find(accessor, price_key) && accessor->second->get_count() == 0) {
            buy_levels_.erase(accessor);
        }
    } else {
        PriceLevelMap::accessor accessor;
        if (sell_levels_.find(accessor, price_key) && accessor->second->get_count() == 0) {
            sell_levels_.erase(accessor);
        }
    }
}

// TradeResult Implementation

nlohmann::json TradeResult::to_json() const {
    nlohmann::json j;
    j["trade_id"] = trade_id;
    j["aggressive_order_id"] = aggressive_order->order_id;
    j["passive_order_id"] = passive_order->order_id;
    j["trade_price"] = trade_price;
    j["trade_volume"] = trade_volume;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::nanoseconds>(
        timestamp.time_since_epoch()).count();
    return j;
}

// MatchingEngine Implementation

MatchingEngine::MatchingEngine(size_t processing_threads)
    : trade_pool_(std::make_shared<memory::ObjectPool<TradeResult>>(100000))
    , incoming_orders_(1000000)  // 1M order capacity
    , outgoing_trades_(1000000)  // 1M trade capacity
{
    // Start processing threads
    processing_threads_.reserve(processing_threads);
    for (size_t i = 0; i < processing_threads; ++i) {
        processing_threads_.emplace_back(&MatchingEngine::process_orders, this);
    }
}

MatchingEngine::~MatchingEngine() {
    stop();
}

void MatchingEngine::add_trade_callback(TradeCallback callback) {
    trade_callbacks_.push_back(callback);
}

void MatchingEngine::add_order_callback(OrderCallback callback) {
    order_callbacks_.push_back(callback);
}

bool MatchingEngine::submit_order(std::shared_ptr<account::Order> order) {
    if (!validate_order(order)) {
        orders_rejected_++;
        return false;
    }

    // Add to processing queue
    return incoming_orders_.enqueue(order);
}

bool MatchingEngine::cancel_order(const std::string& symbol, const std::string& order_id) {
    auto book = get_or_create_book(symbol);
    if (!book) {
        return false;
    }

    auto order = book->get_order(order_id);
    if (!order) {
        return false;
    }

    bool removed = book->remove_order(order_id);
    if (removed) {
        order->status = account::OrderStatus::CANCELLED;
        notify_order_callbacks(order);
    }

    return removed;
}

bool MatchingEngine::modify_order(
    const std::string& symbol,
    const std::string& order_id,
    double new_price,
    double new_volume
) {
    auto book = get_or_create_book(symbol);
    if (!book) {
        return false;
    }

    return book->modify_order(order_id, new_price, new_volume);
}

const OrderBook* MatchingEngine::get_order_book(const std::string& symbol) const {
    tbb::concurrent_hash_map<std::string, std::unique_ptr<OrderBook>>::const_accessor accessor;
    if (order_books_.find(accessor, symbol)) {
        return accessor->second.get();
    }
    return nullptr;
}

std::pair<double, double> MatchingEngine::get_best_bid_ask(const std::string& symbol) const {
    auto book = get_order_book(symbol);
    if (!book) {
        return {0.0, 0.0};
    }

    auto bid = book->get_best_bid();
    auto ask = book->get_best_ask();
    return {bid.first, ask.first};
}

std::pair<std::vector<OrderBook::DepthLevel>, std::vector<OrderBook::DepthLevel>>
MatchingEngine::get_market_depth(const std::string& symbol, size_t levels) const {
    auto book = get_order_book(symbol);
    if (!book) {
        return {{}, {}};
    }

    return {book->get_depth_bids(levels), book->get_depth_asks(levels)};
}

MatchingEngine::EngineStats MatchingEngine::get_statistics() const {
    EngineStats stats;
    stats.orders_processed = orders_processed_.load();
    stats.trades_executed = trades_executed_.load();
    stats.orders_rejected = orders_rejected_.load();
    stats.active_symbols = order_books_.size();

    // Calculate total orders in books
    uint64_t total_orders = 0;
    for (const auto& pair : order_books_) {
        total_orders += pair.second->get_total_orders();
    }
    stats.total_orders_in_book = total_orders;

    return stats;
}

void MatchingEngine::start() {
    processing_enabled_.store(true);
}

void MatchingEngine::stop() {
    processing_enabled_.store(false);

    // Wait for processing threads to finish
    for (auto& thread : processing_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void MatchingEngine::clear_all() {
    order_books_.clear();

    // Clear queues
    std::shared_ptr<account::Order> order;
    while (incoming_orders_.dequeue(order)) {
        // Drain queue
    }

    TradeResult trade;
    while (outgoing_trades_.dequeue(trade)) {
        // Drain queue
    }

    // Reset counters
    orders_processed_.store(0);
    trades_executed_.store(0);
    orders_rejected_.store(0);
}

void MatchingEngine::process_orders() {
    while (processing_enabled_.load()) {
        std::shared_ptr<account::Order> order;

        if (incoming_orders_.dequeue(order)) {
            orders_processed_++;

            // Get or create order book
            auto book = get_or_create_book(order->code);
            if (!book) {
                orders_rejected_++;
                continue;
            }

            // Try to match the order
            auto trades = match_order(order, book);

            // Process resulting trades
            for (const auto& trade : trades) {
                trades_executed_++;
                notify_trade_callbacks(trade);
                outgoing_trades_.enqueue(trade);
            }

            // If order not fully filled, add to book
            if (order->volume > order->trade_volume) {
                order->volume -= order->trade_volume; // Reduce remaining volume
                book->add_order(order);
                order->status = account::OrderStatus::PARTIAL_FILLED;
            } else if (order->trade_volume > 0) {
                order->status = account::OrderStatus::FILLED;
            }

            notify_order_callbacks(order);
        } else {
            // No orders to process, sleep briefly
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
}

std::vector<TradeResult> MatchingEngine::match_order(
    std::shared_ptr<account::Order> order,
    OrderBook* book
) {
    std::vector<TradeResult> trades;

    double remaining_volume = order->volume;

    if (order->direction == account::Direction::BUY) {
        // Match against asks (sell orders)
        auto asks = book->get_depth_asks(100); // Get top 100 levels

        for (const auto& level : asks) {
            if (remaining_volume <= 0) break;
            if (level.price > order->price) break; // Price too high

            double trade_volume = std::min(remaining_volume, level.volume);

            // Find and match against specific orders at this level
            // This is simplified - in practice you'd maintain order priority
            auto trade = execute_trade(order, nullptr, level.price, trade_volume);
            trades.push_back(trade);

            remaining_volume -= trade_volume;
            order->trade_volume += trade_volume;
            order->trade_price = level.price; // Last trade price
        }
    } else {
        // Match against bids (buy orders)
        auto bids = book->get_depth_bids(100);

        for (const auto& level : bids) {
            if (remaining_volume <= 0) break;
            if (level.price < order->price) break; // Price too low

            double trade_volume = std::min(remaining_volume, level.volume);

            auto trade = execute_trade(order, nullptr, level.price, trade_volume);
            trades.push_back(trade);

            remaining_volume -= trade_volume;
            order->trade_volume += trade_volume;
            order->trade_price = level.price;
        }
    }

    return trades;
}

TradeResult MatchingEngine::execute_trade(
    std::shared_ptr<account::Order> aggressive_order,
    std::shared_ptr<account::Order> passive_order,
    double price,
    double volume
) {
    TradeResult trade;
    trade.trade_id = generate_trade_id();
    trade.aggressive_order = aggressive_order;
    trade.passive_order = passive_order; // May be null in simplified matching
    trade.trade_price = price;
    trade.trade_volume = volume;
    trade.timestamp = std::chrono::high_resolution_clock::now();

    return trade;
}

void MatchingEngine::notify_trade_callbacks(const TradeResult& trade) {
    for (const auto& callback : trade_callbacks_) {
        callback(trade);
    }
}

void MatchingEngine::notify_order_callbacks(std::shared_ptr<account::Order> order) {
    for (const auto& callback : order_callbacks_) {
        callback(order);
    }
}

std::string MatchingEngine::generate_trade_id() {
    return "T" + std::to_string(trade_id_counter_++);
}

bool MatchingEngine::validate_order(std::shared_ptr<account::Order> order) const {
    return order &&
           !order->code.empty() &&
           order->volume > 0 &&
           order->price > 0;
}

OrderBook* MatchingEngine::get_or_create_book(const std::string& symbol) {
    tbb::concurrent_hash_map<std::string, std::unique_ptr<OrderBook>>::accessor accessor;

    if (order_books_.insert(accessor, symbol)) {
        accessor->second = std::make_unique<OrderBook>();
    }

    return accessor->second.get();
}

// MarketSimulator Implementation

MarketSimulator::MarketSimulator()
    : matching_engine_(std::make_unique<MatchingEngine>())
{
}

void MarketSimulator::set_market_data(const std::vector<MarketTick>& data) {
    market_data_ = data;
    current_tick_index_ = 0;
}

bool MarketSimulator::load_market_data(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    market_data_.clear();
    std::string line;

    // Skip header if present
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string symbol, timestamp_str, price_str, volume_str;

        if (std::getline(ss, symbol, ',') &&
            std::getline(ss, timestamp_str, ',') &&
            std::getline(ss, price_str, ',') &&
            std::getline(ss, volume_str, ',')) {

            MarketTick tick;
            tick.symbol = symbol;
            tick.price = std::stod(price_str);
            tick.volume = std::stod(volume_str);
            tick.timestamp = std::chrono::high_resolution_clock::now(); // Simplified

            market_data_.push_back(tick);
        }
    }

    current_tick_index_ = 0;
    return true;
}

void MarketSimulator::run_simulation() {
    while (current_tick_index_ < market_data_.size()) {
        step_simulation();
    }
}

void MarketSimulator::step_simulation() {
    if (current_tick_index_ >= market_data_.size()) {
        return;
    }

    const auto& tick = market_data_[current_tick_index_];
    process_market_tick(tick);
    current_tick_index_++;
}

void MarketSimulator::reset_simulation() {
    current_tick_index_ = 0;
    matching_engine_->clear_all();
}

std::chrono::high_resolution_clock::time_point MarketSimulator::get_current_time() const {
    if (current_tick_index_ < market_data_.size()) {
        return market_data_[current_tick_index_].timestamp;
    }
    return std::chrono::high_resolution_clock::now();
}

void MarketSimulator::process_market_tick(const MarketTick& tick) {
    // Generate market orders based on tick data
    simulate_market_orders(tick);

    // Add latency if configured
    if (order_latency_.count() > 0) {
        std::this_thread::sleep_for(order_latency_);
    }
}

void MarketSimulator::simulate_market_orders(const MarketTick& tick) {
    // Create synthetic market orders around the tick price
    double spread = tick.price * 0.001; // 0.1% spread

    // Create bid order
    auto bid_order = std::make_shared<account::Order>();
    bid_order->order_id = "SIM_BID_" + std::to_string(current_tick_index_);
    bid_order->code = tick.symbol;
    bid_order->volume = tick.volume * 0.5;
    bid_order->price = tick.price - spread/2;
    bid_order->direction = account::Direction::BUY;
    bid_order->status = account::OrderStatus::PENDING;

    // Create ask order
    auto ask_order = std::make_shared<account::Order>();
    ask_order->order_id = "SIM_ASK_" + std::to_string(current_tick_index_);
    ask_order->code = tick.symbol;
    ask_order->volume = tick.volume * 0.5;
    ask_order->price = tick.price + spread/2;
    ask_order->direction = account::Direction::SELL;
    ask_order->status = account::OrderStatus::PENDING;

    // Submit to matching engine
    matching_engine_->submit_order(bid_order);
    matching_engine_->submit_order(ask_order);
}

// Factory functions

namespace factory {

std::unique_ptr<MatchingEngine> create_matching_engine(size_t thread_count) {
    return std::make_unique<MatchingEngine>(thread_count);
}

std::unique_ptr<MarketSimulator> create_market_simulator() {
    return std::make_unique<MarketSimulator>();
}

std::unique_ptr<OrderBook> create_order_book() {
    return std::make_unique<OrderBook>();
}

} // namespace factory

} // namespace qaultra::market