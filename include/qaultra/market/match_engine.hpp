#pragma once

#include "../account/order.hpp"
#include "../simd/simd_math.hpp"
#include "../memory/object_pool.hpp"
#include "../threading/lockfree_queue.hpp"

#include <tbb/concurrent_priority_queue.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/spin_mutex.h>

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>

namespace qaultra::market {

/// Price level for order book
struct PriceLevel {
    double price;
    double total_volume;
    uint32_t order_count;

    // Zero-copy linked list for orders at this price level
    struct OrderNode {
        std::shared_ptr<account::Order> order;
        OrderNode* next = nullptr;
        OrderNode* prev = nullptr;
    };

    OrderNode* head = nullptr;
    OrderNode* tail = nullptr;

    /// Add order to this price level
    void add_order(std::shared_ptr<account::Order> order);

    /// Remove order from this price level
    bool remove_order(const std::string& order_id);

    /// Get total volume at this level
    double get_volume() const { return total_volume; }

    /// Get order count at this level
    uint32_t get_count() const { return order_count; }

    /// Clear all orders
    void clear();
};

/// High-performance order book with zero-copy operations
class OrderBook {
private:
    using PriceLevelMap = tbb::concurrent_hash_map<uint64_t, std::unique_ptr<PriceLevel>>;

    // Buy orders (bids) - sorted by price descending
    PriceLevelMap buy_levels_;
    std::atomic<uint64_t> best_bid_price_{0};
    std::atomic<double> best_bid_volume_{0.0};

    // Sell orders (asks) - sorted by price ascending
    PriceLevelMap sell_levels_;
    std::atomic<uint64_t> best_ask_price_{UINT64_MAX};
    std::atomic<double> best_ask_volume_{0.0};

    // Order lookup for fast access
    tbb::concurrent_hash_map<std::string, std::shared_ptr<account::Order>> order_lookup_;

    // Statistics
    std::atomic<uint64_t> total_orders_{0};
    std::atomic<uint64_t> total_volume_{0};
    std::atomic<uint64_t> last_trade_price_{0};
    std::atomic<double> last_trade_volume_{0.0};

    // Memory pool for order nodes
    std::shared_ptr<memory::ObjectPool<PriceLevel::OrderNode>> node_pool_;

    // Price precision (for converting double to uint64_t)
    static constexpr uint64_t PRICE_MULTIPLIER = 10000; // 4 decimal places

    uint64_t price_to_key(double price) const {
        return static_cast<uint64_t>(price * PRICE_MULTIPLIER);
    }

    double key_to_price(uint64_t key) const {
        return static_cast<double>(key) / PRICE_MULTIPLIER;
    }

public:
    /// Constructor
    OrderBook();

    /// Destructor
    ~OrderBook();

    /// Add order to book
    bool add_order(std::shared_ptr<account::Order> order);

    /// Remove order from book
    bool remove_order(const std::string& order_id);

    /// Modify order (price or volume)
    bool modify_order(const std::string& order_id, double new_price, double new_volume);

    /// Get order by ID
    std::shared_ptr<account::Order> get_order(const std::string& order_id) const;

    /// Market data queries
    /// @{

    /// Get best bid/ask
    std::pair<double, double> get_best_bid() const;
    std::pair<double, double> get_best_ask() const;

    /// Get spread
    double get_spread() const;

    /// Get mid price
    double get_mid_price() const;

    /// Get last trade
    std::pair<double, double> get_last_trade() const;

    /// @}

    /// Order book depth
    /// @{

    struct DepthLevel {
        double price;
        double volume;
        uint32_t count;
    };

    /// Get market depth (top N levels)
    std::vector<DepthLevel> get_depth_bids(size_t levels = 10) const;
    std::vector<DepthLevel> get_depth_asks(size_t levels = 10) const;

    /// Get full depth
    std::pair<std::vector<DepthLevel>, std::vector<DepthLevel>> get_full_depth() const;

    /// @}

    /// Statistics
    /// @{

    size_t get_total_orders() const { return total_orders_.load(); }
    uint64_t get_total_volume() const { return total_volume_.load(); }
    bool is_empty() const { return total_orders_.load() == 0; }

    /// @}

    /// Clear all orders
    void clear();

private:
    void update_best_levels();
    PriceLevel* get_or_create_level(uint64_t price_key, account::Direction direction);
    void remove_empty_level(uint64_t price_key, account::Direction direction);
};

/// Trade result from matching
struct TradeResult {
    std::string trade_id;
    std::shared_ptr<account::Order> aggressive_order;  // Market taker
    std::shared_ptr<account::Order> passive_order;     // Market maker
    double trade_price;
    double trade_volume;
    std::chrono::high_resolution_clock::time_point timestamp;

    nlohmann::json to_json() const;
};

/// High-performance matching engine
class MatchingEngine {
public:
    /// Trade callback function
    using TradeCallback = std::function<void(const TradeResult&)>;

    /// Order update callback
    using OrderCallback = std::function<void(std::shared_ptr<account::Order>)>;

private:
    // Order books per symbol
    tbb::concurrent_hash_map<std::string, std::unique_ptr<OrderBook>> order_books_;

    // Trade callbacks
    std::vector<TradeCallback> trade_callbacks_;
    std::vector<OrderCallback> order_callbacks_;

    // Trade ID generation
    std::atomic<uint64_t> trade_id_counter_{0};

    // Processing statistics
    std::atomic<uint64_t> orders_processed_{0};
    std::atomic<uint64_t> trades_executed_{0};
    std::atomic<uint64_t> orders_rejected_{0};

    // Lock-free queues for high-frequency processing
    threading::LockFreeQueue<std::shared_ptr<account::Order>> incoming_orders_;
    threading::LockFreeQueue<TradeResult> outgoing_trades_;

    // Processing thread control
    std::atomic<bool> processing_enabled_{true};
    std::vector<std::thread> processing_threads_;

    // Memory pools
    std::shared_ptr<memory::ObjectPool<TradeResult>> trade_pool_;

public:
    /// Constructor
    explicit MatchingEngine(size_t processing_threads = 4);

    /// Destructor
    ~MatchingEngine();

    /// Add trade callback
    void add_trade_callback(TradeCallback callback);

    /// Add order update callback
    void add_order_callback(OrderCallback callback);

    /// Order operations
    /// @{

    /// Submit order for matching
    bool submit_order(std::shared_ptr<account::Order> order);

    /// Cancel order
    bool cancel_order(const std::string& symbol, const std::string& order_id);

    /// Modify order
    bool modify_order(const std::string& symbol, const std::string& order_id,
                     double new_price, double new_volume);

    /// @}

    /// Market data access
    /// @{

    /// Get order book for symbol
    const OrderBook* get_order_book(const std::string& symbol) const;

    /// Get best bid/ask
    std::pair<double, double> get_best_bid_ask(const std::string& symbol) const;

    /// Get market depth
    std::pair<std::vector<OrderBook::DepthLevel>, std::vector<OrderBook::DepthLevel>>
    get_market_depth(const std::string& symbol, size_t levels = 10) const;

    /// @}

    /// Statistics
    /// @{

    struct EngineStats {
        uint64_t orders_processed = 0;
        uint64_t trades_executed = 0;
        uint64_t orders_rejected = 0;
        uint64_t active_symbols = 0;
        uint64_t total_orders_in_book = 0;
        double avg_processing_time_ns = 0.0;
    };

    EngineStats get_statistics() const;

    /// @}

    /// Control operations
    /// @{

    /// Start processing
    void start();

    /// Stop processing
    void stop();

    /// Clear all order books
    void clear_all();

    /// @}

private:
    /// Process incoming orders (worker thread function)
    void process_orders();

    /// Match order against book
    std::vector<TradeResult> match_order(std::shared_ptr<account::Order> order,
                                        OrderBook* book);

    /// Execute trade
    TradeResult execute_trade(std::shared_ptr<account::Order> aggressive_order,
                             std::shared_ptr<account::Order> passive_order,
                             double price, double volume);

    /// Notify callbacks
    void notify_trade_callbacks(const TradeResult& trade);
    void notify_order_callbacks(std::shared_ptr<account::Order> order);

    /// Generate trade ID
    std::string generate_trade_id();

    /// Validate order
    bool validate_order(std::shared_ptr<account::Order> order) const;

    /// Get or create order book
    OrderBook* get_or_create_book(const std::string& symbol);
};

/// Market simulator for backtesting
class MarketSimulator {
private:
    std::unique_ptr<MatchingEngine> matching_engine_;

    // Market data playback
    struct MarketTick {
        std::string symbol;
        double price;
        double volume;
        std::chrono::high_resolution_clock::time_point timestamp;
    };

    std::vector<MarketTick> market_data_;
    size_t current_tick_index_ = 0;

    // Simulated latency
    std::chrono::nanoseconds order_latency_{0};
    std::chrono::nanoseconds trade_latency_{0};

public:
    /// Constructor
    MarketSimulator();

    /// Set market data
    void set_market_data(const std::vector<MarketTick>& data);

    /// Load market data from file
    bool load_market_data(const std::string& filename);

    /// Set latency simulation
    void set_order_latency(std::chrono::nanoseconds latency) { order_latency_ = latency; }
    void set_trade_latency(std::chrono::nanoseconds latency) { trade_latency_ = latency; }

    /// Access matching engine
    MatchingEngine* get_matching_engine() { return matching_engine_.get(); }

    /// Simulation control
    void run_simulation();
    void step_simulation();
    void reset_simulation();

    /// Get current simulation time
    std::chrono::high_resolution_clock::time_point get_current_time() const;

private:
    void process_market_tick(const MarketTick& tick);
    void simulate_market_orders(const MarketTick& tick);
};

/// Real-time market data feed interface
class MarketDataFeed {
public:
    virtual ~MarketDataFeed() = default;

    /// Market data event
    struct MarketEvent {
        enum Type { TRADE, QUOTE, DEPTH_UPDATE, STATUS_CHANGE };

        Type type;
        std::string symbol;
        double price = 0.0;
        double volume = 0.0;
        double bid_price = 0.0;
        double ask_price = 0.0;
        double bid_volume = 0.0;
        double ask_volume = 0.0;
        std::chrono::high_resolution_clock::time_point timestamp;

        nlohmann::json metadata;
    };

    /// Event callback
    using EventCallback = std::function<void(const MarketEvent&)>;

    /// Subscribe to symbol
    virtual bool subscribe(const std::string& symbol) = 0;

    /// Unsubscribe from symbol
    virtual bool unsubscribe(const std::string& symbol) = 0;

    /// Add event callback
    virtual void add_callback(EventCallback callback) = 0;

    /// Start feed
    virtual void start() = 0;

    /// Stop feed
    virtual void stop() = 0;
};

/// Factory functions
namespace factory {
    /// Create matching engine with default configuration
    std::unique_ptr<MatchingEngine> create_matching_engine(
        size_t thread_count = std::thread::hardware_concurrency());

    /// Create market simulator
    std::unique_ptr<MarketSimulator> create_market_simulator();

    /// Create order book
    std::unique_ptr<OrderBook> create_order_book();
}

} // namespace qaultra::market