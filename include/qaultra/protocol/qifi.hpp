#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <shared_mutex>

namespace qaultra::protocol::qifi {

/// QIFI (Quantitative Investment Format Interface) implementation
/// Exact port from Rust QARS QIFI protocol

/// Trade information
struct Trade {
    std::string user_id;
    std::string trade_id;
    std::string order_id;
    std::string account_id;
    std::string exchange_id;
    std::string instrument_id;
    double price = 0.0;
    double volume = 0.0;
    std::string trade_time;
    std::string direction;        // "BUY" or "SELL"
    std::string offset;          // "OPEN", "CLOSE", "CLOSETODAY", "CLOSEYESTERDAY"
    double commission = 0.0;
    double tax = 0.0;

    nlohmann::json to_json() const;
    static Trade from_json(const nlohmann::json& j);
};

/// Order information
struct Order {
    std::string user_id;
    std::string order_id;
    std::string account_id;
    std::string exchange_id;
    std::string instrument_id;
    double price = 0.0;
    double volume = 0.0;
    double volume_left = 0.0;
    std::string direction;        // "BUY" or "SELL"
    std::string offset;          // "OPEN", "CLOSE", "CLOSETODAY", "CLOSEYESTERDAY"
    std::string order_time;
    std::string status;          // "PENDING", "PARTIAL_FILLED", "FILLED", "CANCELLED", "REJECTED"
    std::string price_type;      // "LIMIT", "MARKET", "STOP"
    std::string time_condition;  // "GTC", "IOC", "FOK", "DAY"
    std::string volume_condition; // "ANY", "MIN", "ALL"
    std::string last_msg;

    nlohmann::json to_json() const;
    static Order from_json(const nlohmann::json& j);
};

/// Position information
struct Position {
    std::string user_id;
    std::string exchange_id;
    std::string instrument_id;
    double volume_long_today = 0.0;
    double volume_long_his = 0.0;
    double volume_long = 0.0;
    double volume_short_today = 0.0;
    double volume_short_his = 0.0;
    double volume_short = 0.0;
    double open_cost_long = 0.0;
    double open_cost_short = 0.0;
    double position_cost_long = 0.0;
    double position_cost_short = 0.0;
    double float_profit_long = 0.0;
    double float_profit_short = 0.0;
    double float_profit = 0.0;
    double position_profit_long = 0.0;
    double position_profit_short = 0.0;
    double position_profit = 0.0;
    double margin_long = 0.0;
    double margin_short = 0.0;
    double margin = 0.0;
    double last_price = 0.0;
    std::string last_updatetime;

    nlohmann::json to_json() const;
    static Position from_json(const nlohmann::json& j);
};

/// Frozen funds information
struct Frozen {
    std::string order_id;
    std::string instrument_id;
    double money = 0.0;
    std::string datetime;

    nlohmann::json to_json() const;
    static Frozen from_json(const nlohmann::json& j);
};

/// Account information
struct Account {
    std::string user_id;
    std::string currency = "CNY";
    double pre_balance = 0.0;        // Previous day balance
    double deposit = 0.0;            // Deposits
    double withdraw = 0.0;           // Withdrawals
    double WithdrawQuota = 0.0;      // Withdrawal quota
    double close_profit = 0.0;       // Realized profit
    double commission = 0.0;         // Commission
    double premium = 0.0;            // Premium
    double static_balance = 0.0;     // Static balance
    double position_profit = 0.0;    // Position profit
    double float_profit = 0.0;       // Float profit
    double balance = 0.0;            // Current balance
    double margin = 0.0;             // Required margin
    double frozen_margin = 0.0;      // Frozen margin
    double frozen_commission = 0.0;  // Frozen commission
    double frozen_premium = 0.0;     // Frozen premium
    double available = 0.0;          // Available funds
    double risk_ratio = 0.0;         // Risk ratio

    nlohmann::json to_json() const;
    static Account from_json(const nlohmann::json& j);
};

/// Bank information
struct Bank {
    std::string bank_id;
    std::string bank_name;
    std::string bank_account;
    double bank_balance = 0.0;
    std::string currency = "CNY";

    nlohmann::json to_json() const;
    static Bank from_json(const nlohmann::json& j);
};

/// Transfer information
struct Transfer {
    std::string transfer_id;
    std::string account_id;
    std::string bank_id;
    double amount = 0.0;
    std::string direction;  // "IN", "OUT"
    std::string status;     // "PENDING", "SUCCESS", "FAILED"
    std::string datetime;

    nlohmann::json to_json() const;
    static Transfer from_json(const nlohmann::json& j);
};

/// QIFI main structure
struct QIFI {
    std::string account_cookie;      // Account identifier
    std::string portfolio;           // Portfolio identifier
    std::string investor_name;       // Investor name
    std::string password;            // Password (encrypted)
    std::string broker_name;         // Broker name
    std::string capital_account;     // Capital account
    std::string bank_account;        // Bank account
    std::string bank_password;       // Bank password (encrypted)
    double money = 0.0;              // Current cash
    std::string source;              // Data source
    std::string sourceid;            // Source identifier
    std::string updatetime;          // Last update time
    std::string trading_day;         // Trading day

    // Core data
    Account accounts;
    std::unordered_map<std::string, Position> positions;
    std::unordered_map<std::string, Order> orders;
    std::unordered_map<std::string, Trade> trades;
    std::unordered_map<std::string, Frozen> frozen;

    // Banking
    std::vector<Bank> banks;
    std::vector<Transfer> transfers;

    // Events and logs
    std::vector<std::string> events;
    std::vector<std::string> errors;

    /// Default constructor
    QIFI() = default;

    /// Constructor with basic information
    QIFI(const std::string& account_cookie,
         const std::string& portfolio,
         const std::string& investor_name,
         const std::string& broker_name);

    /// Add position
    void add_position(const std::string& instrument_id, const Position& position);

    /// Add order
    void add_order(const std::string& order_id, const Order& order);

    /// Add trade
    void add_trade(const std::string& trade_id, const Trade& trade);

    /// Add frozen funds
    void add_frozen(const std::string& order_id, const Frozen& frozen);

    /// Update account info
    void update_account(const Account& account);

    /// Calculate derived values
    void calculate_derived_values();

    /// Validate QIFI data
    bool validate() const;

    /// Get summary information
    struct Summary {
        double total_balance = 0.0;
        double total_market_value = 0.0;
        double total_float_profit = 0.0;
        double total_margin = 0.0;
        size_t position_count = 0;
        size_t order_count = 0;
        size_t trade_count = 0;
    };
    Summary get_summary() const;

    /// Serialization
    nlohmann::json to_json() const;
    static QIFI from_json(const nlohmann::json& j);

    /// Save to file
    bool save_to_file(const std::string& filename) const;

    /// Load from file
    static QIFI load_from_file(const std::string& filename);

    /// Convert to string
    std::string to_string() const;

    /// Create from string
    static QIFI from_string(const std::string& qifi_str);
};

/// QIFI utilities
namespace utils {
    /// Create empty QIFI
    QIFI create_empty_qifi(const std::string& account_id,
                          const std::string& broker_name = "default",
                          double initial_balance = 0.0);

    /// Merge multiple QIFI objects
    QIFI merge_qifi(const std::vector<QIFI>& qifi_list);

    /// Convert QIFI to CSV
    std::string qifi_to_csv(const QIFI& qifi);

    /// Validate QIFI format
    bool validate_qifi_format(const nlohmann::json& j);

    /// Compare two QIFI objects
    struct QIFIDifference {
        std::vector<std::string> account_changes;
        std::vector<std::string> position_changes;
        std::vector<std::string> order_changes;
        std::vector<std::string> trade_changes;
    };
    QIFIDifference compare_qifi(const QIFI& qifi1, const QIFI& qifi2);

    /// Get QIFI statistics
    struct QIFIStats {
        size_t total_positions = 0;
        size_t active_positions = 0;
        size_t total_orders = 0;
        size_t active_orders = 0;
        size_t total_trades = 0;
        double total_volume = 0.0;
        double total_commission = 0.0;
        std::string first_trade_time;
        std::string last_trade_time;
    };
    QIFIStats get_qifi_stats(const QIFI& qifi);

    /// Filter QIFI data
    QIFI filter_by_instrument(const QIFI& qifi, const std::string& instrument_id);
    QIFI filter_by_time_range(const QIFI& qifi, const std::string& start_time,
                             const std::string& end_time);

    /// Calculate performance metrics
    struct PerformanceMetrics {
        double total_return = 0.0;
        double sharpe_ratio = 0.0;
        double max_drawdown = 0.0;
        double win_rate = 0.0;
        double profit_factor = 0.0;
        double avg_trade_profit = 0.0;
        size_t total_trades = 0;
        size_t winning_trades = 0;
        size_t losing_trades = 0;
    };
    PerformanceMetrics calculate_performance(const QIFI& qifi);
}

/// QIFI event handler interface
class QIFIEventHandler {
public:
    virtual ~QIFIEventHandler() = default;

    virtual void on_position_updated(const std::string& instrument_id, const Position& position) {}
    virtual void on_order_updated(const std::string& order_id, const Order& order) {}
    virtual void on_trade_created(const std::string& trade_id, const Trade& trade) {}
    virtual void on_account_updated(const Account& account) {}
    virtual void on_error(const std::string& error_msg) {}
};

/// QIFI manager for real-time updates
class QIFIManager {
private:
    QIFI qifi_;
    std::vector<std::shared_ptr<QIFIEventHandler>> handlers_;
    mutable std::shared_mutex qifi_mutex_;

public:
    /// Constructor
    explicit QIFIManager(const QIFI& initial_qifi = QIFI{});

    /// Add event handler
    void add_handler(std::shared_ptr<QIFIEventHandler> handler);

    /// Remove event handler
    void remove_handler(std::shared_ptr<QIFIEventHandler> handler);

    /// Update operations (thread-safe)
    void update_position(const std::string& instrument_id, const Position& position);
    void update_order(const std::string& order_id, const Order& order);
    void add_trade(const std::string& trade_id, const Trade& trade);
    void update_account(const Account& account);

    /// Get current QIFI (thread-safe copy)
    QIFI get_qifi() const;

    /// Save current state
    bool save_to_file(const std::string& filename) const;

    /// Load state
    bool load_from_file(const std::string& filename);

private:
    void notify_handlers_position(const std::string& instrument_id, const Position& position);
    void notify_handlers_order(const std::string& order_id, const Order& order);
    void notify_handlers_trade(const std::string& trade_id, const Trade& trade);
    void notify_handlers_account(const Account& account);
    void notify_handlers_error(const std::string& error_msg);
};

} // namespace qaultra::protocol::qifi