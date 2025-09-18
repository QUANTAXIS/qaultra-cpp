#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

// Forward declaration to avoid nlohmann dependency in header
namespace nlohmann { class json; }

namespace qaultra::account {

// Type definitions
using Price = double;
using Volume = double;
using Amount = double;
using Timestamp = std::chrono::system_clock::time_point;
using AssetId = std::string;

/// Order direction
enum class Direction {
    BUY = 1,
    SELL = -1
};

/// Order status
enum class OrderStatus {
    PENDING,
    PARTIAL_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

/**
 * @brief Trading order representation
 */
class Order {
public:
    std::string order_id;           ///< Unique order identifier
    std::string account_id;         ///< Account identifier
    AssetId code;                   ///< Asset identifier
    Direction direction;            ///< Order direction (BUY/SELL)
    Price price;                    ///< Order price (0 for market orders)
    Volume volume;                  ///< Original volume
    Volume volume_left;             ///< Remaining volume
    Volume volume_filled;           ///< Filled volume
    OrderStatus status;             ///< Order status
    Timestamp create_time;          ///< Creation timestamp
    Timestamp update_time;          ///< Last update timestamp
    std::string exchange_id;        ///< Exchange identifier
    std::string user_id;            ///< User identifier
    std::string strategy_id;        ///< Strategy identifier

    // Order type specific fields
    enum class OrderType {
        MARKET,
        LIMIT,
        STOP,
        STOP_LIMIT
    };

    enum class TimeCondition {
        GTC,    // Good Till Cancel
        IOC,    // Immediate or Cancel
        FOK,    // Fill or Kill
        DAY     // Day order
    };

    OrderType order_type;           ///< Order type
    TimeCondition time_condition;   ///< Time condition
    std::string last_msg;           ///< Last message/error

    /// Default constructor
    Order() = default;

    /// Constructor for basic order
    Order(const std::string& order_id, const std::string& account_id,
          const AssetId& code, Direction direction, Price price, Volume volume);

    /// Copy constructor
    Order(const Order&) = default;

    /// Move constructor
    Order(Order&&) = default;

    /// Assignment operators
    Order& operator=(const Order&) = default;
    Order& operator=(Order&&) = default;

    /// Destructor
    virtual ~Order() = default;

    /// Check if order is active (can be filled)
    bool is_active() const;

    /// Check if order is completed (filled or cancelled)
    bool is_completed() const;

    /// Check if order is partially filled
    bool is_partially_filled() const;

    /// Get fill percentage (0.0 to 1.0)
    double fill_percentage() const;

    /// Get remaining volume
    Volume get_remaining_volume() const;

    /// Fill order (partially or completely)
    bool fill(Volume fill_volume, Price fill_price);

    /// Cancel order
    void cancel();

    /// Reject order
    void reject(const std::string& reason);

    /// Update order status
    void update_status(OrderStatus new_status);

    /// Get order value (price * volume)
    Amount get_order_value() const;

    /// Get filled value
    Amount get_filled_value() const;

    /// Convert to JSON
    nlohmann::json to_json() const;

    /// Create from JSON
    static Order from_json(const nlohmann::json& j);

    /// String representation
    std::string to_string() const;

    /// Comparison operators
    bool operator==(const Order& other) const;
    bool operator!=(const Order& other) const;

private:
    Amount filled_value_ = 0.0;     ///< Total filled value

    void update_filled_volume();
    void update_timestamp();
};

/// Order management utilities
class OrderManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Order>> orders_;
    std::vector<std::string> active_order_ids_;

public:
    /// Add order to management
    bool add_order(std::unique_ptr<Order> order);

    /// Get order by ID
    Order* get_order(const std::string& order_id);
    const Order* get_order(const std::string& order_id) const;

    /// Remove order
    bool remove_order(const std::string& order_id);

    /// Get all active orders
    std::vector<Order*> get_active_orders();
    std::vector<const Order*> get_active_orders() const;

    /// Get orders by status
    std::vector<Order*> get_orders_by_status(OrderStatus status);

    /// Get orders by asset
    std::vector<Order*> get_orders_by_asset(const AssetId& code);

    /// Cancel all orders
    void cancel_all_orders();

    /// Cancel orders by asset
    void cancel_orders_by_asset(const AssetId& code);

    /// Get order count
    size_t size() const;
    bool empty() const;

    /// Clear all orders
    void clear();

    /// Get statistics
    struct OrderStats {
        size_t total_count = 0;
        size_t active_count = 0;
        size_t filled_count = 0;
        size_t cancelled_count = 0;
        Amount total_value = 0.0;
        Amount filled_value = 0.0;
    };

    OrderStats get_statistics() const;

    /// Convert to JSON
    nlohmann::json to_json() const;
};

/// Order factory functions
namespace order_factory {
    /// Create market buy order
    std::unique_ptr<Order> create_market_buy(
        const std::string& account_id, const AssetId& code, Volume volume);

    /// Create market sell order
    std::unique_ptr<Order> create_market_sell(
        const std::string& account_id, const AssetId& code, Volume volume);

    /// Create limit buy order
    std::unique_ptr<Order> create_limit_buy(
        const std::string& account_id, const AssetId& code, Volume volume, Price price);

    /// Create limit sell order
    std::unique_ptr<Order> create_limit_sell(
        const std::string& account_id, const AssetId& code, Volume volume, Price price);
}

} // namespace qaultra::account