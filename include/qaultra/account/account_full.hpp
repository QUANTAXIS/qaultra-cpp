#pragma once

#include "position.hpp"
#include "order.hpp"
#include "../protocol/qifi.hpp"
#include "../simd/simd_math.hpp"
#include "../threading/thread_pool.hpp"
#include "../memory/object_pool.hpp"

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <tbb/parallel_for.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <shared_mutex>

namespace qaultra::account {

/// Market preset configuration matching Rust implementation
struct MarketPreset {
    std::string name;
    int unit_table = 1;
    double price_tick = 0.01;
    double volume_tick = 1.0;
    double buy_fee_ratio = 0.0;
    double sell_fee_ratio = 0.0;
    double min_fee = 0.0;
    double tax_ratio = 0.001;      // Stock tax
    double margin_ratio = 0.1;     // Futures margin
    bool is_stock = true;
    bool allow_t0 = false;
    bool allow_sellopen = false;

    MarketPreset() = default;
    MarketPreset(const std::string& market_name);

    static MarketPreset get_stock_preset();
    static MarketPreset get_future_preset();
    static MarketPreset get_forex_preset();

    nlohmann::json to_json() const;
    static MarketPreset from_json(const nlohmann::json& j);
};

/// Frozen funds information
struct Frozen {
    Amount money = 0.0;
    std::string order_id;
    std::string datetime;
    AssetId code;

    nlohmann::json to_json() const;
    static Frozen from_json(const nlohmann::json& j);
};

/// Account slice for historical tracking
struct AccountSlice {
    std::string datetime;
    Amount cash;
    protocol::qifi::Account accounts;
    std::unordered_map<AssetId, Position> positions;

    nlohmann::json to_json() const;
    static AccountSlice from_json(const nlohmann::json& j);
};

/// Market-on-Market slice
struct MOMSlice {
    std::string datetime;
    std::string user_id;
    Amount pre_balance = 0.0;
    Amount close_profit = 0.0;
    Amount commission = 0.0;
    Amount position_profit = 0.0;
    Amount float_profit = 0.0;
    Amount balance = 0.0;
    Amount margin = 0.0;
    Amount available = 0.0;
    double risk_ratio = 0.0;

    nlohmann::json to_json() const;
    static MOMSlice from_json(const nlohmann::json& j);
};

/// Account information for communication
struct AccountInfo {
    std::string datetime;
    Amount balance;
    std::string account_cookie;

    nlohmann::json to_json() const;
    static AccountInfo from_json(const nlohmann::json& j);
};

/// High-performance quantitative trading account (exact port of Rust QA_Account)
class QA_Account {
private:
    // Thread-safe containers for high-frequency trading
    mutable std::shared_mutex account_mutex_;
    tbb::concurrent_hash_map<AssetId, std::unique_ptr<Position>> hold_;
    tbb::concurrent_hash_map<std::string, std::unique_ptr<Order>> daily_orders_;
    tbb::concurrent_hash_map<std::string, protocol::qifi::Trade> daily_trades_;
    tbb::concurrent_hash_map<AssetId, Frozen> frozen_;

    // High-performance thread pool for calculations
    std::shared_ptr<threading::ThreadPool> thread_pool_;

    // Memory pools for frequent allocations
    std::shared_ptr<memory::ObjectPool<Order>> order_pool_;
    std::shared_ptr<memory::ObjectPool<Position>> position_pool_;

    // Atomic counters for thread safety
    std::atomic<Amount> money_{0.0};
    std::atomic<int> event_id_{0};

    // SIMD-optimized calculation buffers
    mutable simd::f64_vector calculation_buffer_;

public:
    // Core account properties (matching Rust implementation)
    Amount init_cash;
    std::unordered_map<AssetId, Position> init_hold;
    bool allow_t0;
    bool allow_sellopen;
    bool allow_margin;
    bool auto_reload;
    MarketPreset market_preset;
    std::string time;
    std::unordered_map<std::string, std::string> events;
    protocol::qifi::Account accounts;
    std::string account_cookie;
    std::string portfolio_cookie;
    std::string user_cookie;
    std::string environment;
    double commission_ratio;
    double tax_ratio;

    // Algorithm order manager (forward declaration)
    std::unique_ptr<class AlgoOrderManager> algo_manager;

public:
    /// Default constructor
    QA_Account() = default;

    /// Main constructor (matching Rust implementation)
    QA_Account(const std::string& account_cookie,
               const std::string& portfolio_cookie,
               const std::string& user_cookie,
               Amount init_cash,
               bool auto_reload = false,
               const std::string& environment = "real");

    /// Constructor from QIFI format
    static std::unique_ptr<QA_Account> from_qifi(const protocol::qifi::QIFI& qifi);

    /// Copy constructor (deleted - use move semantics)
    QA_Account(const QA_Account&) = delete;

    /// Move constructor
    QA_Account(QA_Account&&) = default;

    /// Assignment operators
    QA_Account& operator=(const QA_Account&) = delete;
    QA_Account& operator=(QA_Account&&) = default;

    /// Destructor
    ~QA_Account();

    /// Configuration methods
    /// @{
    void set_sellopen(bool sellopen) { allow_sellopen = sellopen; }
    void set_t0(bool t0) { allow_t0 = t0; }
    void set_portfolio_cookie(const std::string& portfolio) { portfolio_cookie = portfolio; }
    void set_commission_ratio(double ratio) { commission_ratio = ratio; }
    void set_tax_ratio(double ratio) { tax_ratio = ratio; }
    /// @}

    /// Position management
    /// @{

    /// Initialize position for asset
    void init_position(const AssetId& code);

    /// Get position (thread-safe)
    const Position* get_position(const AssetId& code) const;
    Position* get_position(const AssetId& code);

    /// Get all positions (thread-safe copy)
    std::unordered_map<AssetId, Position> get_all_positions() const;

    /// Get position volume for asset
    Volume get_volume_long(const AssetId& code) const;
    Volume get_volume_short(const AssetId& code) const;
    Volume get_volume_net(const AssetId& code) const;

    /// @}

    /// Account information queries (high-performance implementations)
    /// @{

    /// Get current cash (lock-free)
    Amount get_cash() const { return money_.load(std::memory_order_acquire); }

    /// Get frozen margin (SIMD-optimized)
    Amount get_frozen_margin() const;

    /// Get risk ratio
    double get_risk_ratio() const;

    /// Get position profit (parallel calculation)
    Amount get_position_profit() const;

    /// Get float profit (parallel calculation with SIMD)
    Amount get_float_profit() const;

    /// Get balance
    Amount get_balance() const;

    /// Get margin (parallel calculation)
    Amount get_margin() const;

    /// Get available funds
    Amount get_available() const { return get_cash(); }

    /// @}

    /// Trading operations (exact ports from Rust)
    /// @{

    /// Stock trading
    std::unique_ptr<Order> buy(const AssetId& code, Volume volume,
                              const std::string& datetime, Price price);
    std::unique_ptr<Order> sell(const AssetId& code, Volume volume,
                               const std::string& datetime, Price price);

    /// Futures trading
    std::unique_ptr<Order> buy_open(const AssetId& code, Volume volume,
                                   const std::string& datetime, Price price);
    std::unique_ptr<Order> sell_open(const AssetId& code, Volume volume,
                                    const std::string& datetime, Price price);
    std::unique_ptr<Order> buy_close(const AssetId& code, Volume volume,
                                    const std::string& datetime, Price price);
    std::unique_ptr<Order> sell_close(const AssetId& code, Volume volume,
                                     const std::string& datetime, Price price);
    std::unique_ptr<Order> buy_close_today(const AssetId& code, Volume volume,
                                          const std::string& datetime, Price price);
    std::unique_ptr<Order> sell_close_today(const AssetId& code, Volume volume,
                                           const std::string& datetime, Price price);

    /// Smart trading (auto-detect open/close)
    std::unique_ptr<Order> smart_buy(const AssetId& code, Volume volume,
                                    const std::string& datetime, Price price);
    std::unique_ptr<Order> smart_sell(const AssetId& code, Volume volume,
                                     const std::string& datetime, Price price);

    /// @}

    /// Market data updates
    /// @{

    /// Update price for single asset
    void on_price_change(const AssetId& code, Price new_price, const std::string& datetime);

    /// Batch update prices (parallel processing)
    void update_market_data(const std::unordered_map<AssetId, Price>& prices,
                           const std::string& datetime);

    /// @}

    /// Order management
    /// @{

    /// Get order by ID
    const Order* get_order(const std::string& order_id) const;
    Order* get_order(const std::string& order_id);

    /// Get all orders
    std::vector<const Order*> get_orders() const;

    /// Get active orders
    std::vector<const Order*> get_active_orders() const;

    /// Cancel order
    bool cancel_order(const std::string& order_id);

    /// Cancel all orders
    void cancel_all_orders();

    /// @}

    /// Account operations
    /// @{

    /// Settle account (end of day)
    void settle();

    /// Change current datetime
    void change_datetime(const std::string& datetime);

    /// Deposit funds
    void deposit(Amount amount);

    /// Withdraw funds
    bool withdraw(Amount amount);

    /// Reload account state
    void reload();

    /// @}

    /// Analytics and reporting
    /// @{

    /// Get MOM slice
    MOMSlice get_mom_slice() const;

    /// Get account slice
    AccountSlice get_account_slice() const;

    /// Get account info
    AccountInfo get_account_info() const;

    /// Get position values (parallel calculation)
    std::unordered_map<AssetId, Amount> get_account_pos_value() const;

    /// Get position weights
    std::unordered_map<AssetId, double> get_account_pos_weight() const;

    /// @}

    /// Risk management
    /// @{

    /// Check if order is allowed
    bool check_order_allowed(const AssetId& code, Volume volume, Price price,
                            Direction direction) const;

    /// Check available funds
    bool check_available_funds(Amount required_amount) const;

    /// Get maximum order size
    Volume get_max_order_size(const AssetId& code, Price price, Direction direction) const;

    /// Calculate required margin
    Amount calculate_required_margin(const AssetId& code, Volume volume) const;

    /// @}

    /// Serialization (thread-safe)
    /// @{

    /// Convert to QIFI format
    protocol::qifi::QIFI to_qifi() const;

    /// Convert to JSON
    nlohmann::json to_json() const;

    /// Create from JSON
    static std::unique_ptr<QA_Account> from_json(const nlohmann::json& j);

    /// @}

    /// Performance monitoring
    /// @{

    /// Get processing statistics
    struct ProcessingStats {
        std::atomic<uint64_t> orders_processed{0};
        std::atomic<uint64_t> trades_processed{0};
        std::atomic<uint64_t> price_updates{0};
        std::atomic<uint64_t> calculations_performed{0};
        std::chrono::steady_clock::time_point start_time;
    };

    const ProcessingStats& get_processing_stats() const { return stats_; }

    /// @}

private:
    /// Internal statistics
    mutable ProcessingStats stats_;

    /// Internal order processing
    std::unique_ptr<Order> send_order(const AssetId& code, Volume volume,
                                     const std::string& datetime,
                                     int direction, Price price,
                                     const std::string& offset = "",
                                     const std::string& order_type = "LIMIT");

    /// Process order execution (thread-safe)
    void process_order_execution(Order* order, Volume fill_volume, Price fill_price);

    /// Update account info (thread-safe)
    void update_account_info();

    /// Freeze/unfreeze funds
    void freeze_funds(const AssetId& code, Amount amount, const std::string& order_id);
    void unfreeze_funds(const AssetId& code, const std::string& order_id);

    /// Commission calculation
    Amount calculate_commission(const AssetId& code, Volume volume, Price price,
                               Direction direction) const;

    /// Tax calculation (for stocks)
    Amount calculate_tax(const AssetId& code, Volume volume, Price price,
                        Direction direction) const;

    /// Market type detection
    MarketPreset get_market_preset(const AssetId& code) const;

    /// Parallel calculation helpers
    template<typename Func>
    void parallel_position_calculation(Func func) const;

    /// Memory management
    void cleanup_expired_objects();
    void reserve_capacity(size_t expected_positions, size_t expected_orders);
};

/// Account factory functions
namespace account_factory {
    /// Create real trading account
    std::unique_ptr<QA_Account> create_real_account(
        const std::string& account_id, const std::string& broker_name,
        const std::string& user_id, Amount initial_balance);

    /// Create simulation account
    std::unique_ptr<QA_Account> create_sim_account(
        const std::string& account_id, const std::string& user_id,
        Amount initial_balance);

    /// Create backtest account
    std::unique_ptr<QA_Account> create_backtest_account(
        const std::string& account_id, const std::string& user_id,
        Amount initial_balance);

    /// Create account from QIFI
    std::unique_ptr<QA_Account> from_qifi_message(const protocol::qifi::QIFI& qifi);
}

/// High-performance account manager for multiple accounts
class AccountManager {
private:
    tbb::concurrent_hash_map<std::string, std::unique_ptr<QA_Account>> accounts_;
    std::shared_ptr<threading::ThreadPool> thread_pool_;
    mutable std::shared_mutex manager_mutex_;

public:
    /// Constructor
    explicit AccountManager(size_t thread_pool_size = std::thread::hardware_concurrency());

    /// Add account
    bool add_account(std::unique_ptr<QA_Account> account);

    /// Get account
    QA_Account* get_account(const std::string& account_id);
    const QA_Account* get_account(const std::string& account_id) const;

    /// Remove account
    bool remove_account(const std::string& account_id);

    /// Get all accounts
    std::vector<std::string> get_account_ids() const;

    /// Batch price update for all accounts
    void update_all_prices(const std::unordered_map<AssetId, Price>& prices,
                          const std::string& datetime);

    /// Settle all accounts
    void settle_all_accounts();

    /// Get combined statistics
    struct CombinedStats {
        Amount total_balance = 0.0;
        Amount total_market_value = 0.0;
        Amount total_float_profit = 0.0;
        size_t account_count = 0;
        size_t position_count = 0;
        size_t order_count = 0;
    };

    CombinedStats get_combined_stats() const;

    /// Save all accounts to directory
    bool save_all_accounts(const std::string& directory_path) const;

    /// Load accounts from directory
    bool load_accounts_from_directory(const std::string& directory_path);
};

} // namespace qaultra::account