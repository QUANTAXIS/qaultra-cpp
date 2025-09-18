#pragma once

#include "../qaultra.hpp"
#include "position.hpp"
#include "order.hpp"
#include <nlohmann/json.hpp>
#include <memory>

namespace qaultra::account {

/**
 * @brief Main account class for trading operations
 */
class Account {
public:
    std::string account_id;         ///< Account identifier
    std::string user_id;           ///< User identifier
    std::string broker_name;       ///< Broker name
    std::string account_cookie;    ///< Account session cookie
    std::string portfolio_cookie;  ///< Portfolio session cookie
    std::string trade_model;       ///< Trading model (real/sim/backtest)
    bool allow_sellopen;           ///< Allow sell open for stocks
    bool allow_t0;                 ///< Allow T+0 trading

    // Account financial information
    struct AccountInfo {
        Amount static_balance = 0.0;    ///< Static balance
        Amount balance = 0.0;           ///< Current balance
        Amount available = 0.0;         ///< Available funds
        Amount market_value = 0.0;      ///< Market value of positions
        Amount float_profit = 0.0;      ///< Unrealized profit/loss
        Amount position_profit = 0.0;   ///< Realized profit/loss
        Amount frozen = 0.0;            ///< Frozen funds
        Amount margin = 0.0;            ///< Required margin
        Amount commission = 0.0;        ///< Total commission paid
        double risk_ratio = 0.0;        ///< Risk ratio
        Timestamp last_update;          ///< Last update time
    } info;

    // Current datetime for backtesting
    std::string current_datetime;

private:
    std::unique_ptr<PositionManager> position_manager_;
    std::unique_ptr<OrderManager> order_manager_;

public:
    /// Default constructor
    Account() = default;

    /// Constructor with basic information
    Account(const std::string& account_id, const std::string& broker_name,
            const std::string& user_id, Amount initial_balance,
            bool allow_t0 = false, const std::string& trade_model = "real");

    /// Copy constructor (deleted - use move semantics)
    Account(const Account&) = delete;

    /// Move constructor
    Account(Account&&) = default;

    /// Assignment operators
    Account& operator=(const Account&) = delete;
    Account& operator=(Account&&) = default;

    /// Destructor
    ~Account() = default;

    /// Initialize account
    void initialize();

    /// Trading operations
    /// @{

    /// Buy stocks
    std::unique_ptr<Order> buy(const AssetId& code, Volume volume,
                              const std::string& datetime, Price price);

    /// Sell stocks
    std::unique_ptr<Order> sell(const AssetId& code, Volume volume,
                               const std::string& datetime, Price price);

    /// Buy open (futures)
    std::unique_ptr<Order> buy_open(const AssetId& code, Volume volume,
                                   const std::string& datetime, Price price);

    /// Sell open (futures)
    std::unique_ptr<Order> sell_open(const AssetId& code, Volume volume,
                                    const std::string& datetime, Price price);

    /// Buy close (futures)
    std::unique_ptr<Order> buy_close(const AssetId& code, Volume volume,
                                    const std::string& datetime, Price price);

    /// Sell close (futures)
    std::unique_ptr<Order> sell_close(const AssetId& code, Volume volume,
                                     const std::string& datetime, Price price);

    /// Buy close today (futures)
    std::unique_ptr<Order> buy_close_today(const AssetId& code, Volume volume,
                                          const std::string& datetime, Price price);

    /// Sell close today (futures)
    std::unique_ptr<Order> sell_close_today(const AssetId& code, Volume volume,
                                           const std::string& datetime, Price price);
    /// @}

    /// Account information queries
    /// @{

    /// Get current balance
    Amount get_balance() const;

    /// Get available funds
    Amount get_available() const;

    /// Get market value of all positions
    Amount get_market_value() const;

    /// Get total float profit
    Amount get_float_profit() const;

    /// Get total position profit
    Amount get_position_profit() const;

    /// Get total margin required
    Amount get_margin() const;

    /// Get risk ratio
    double get_risk_ratio() const;

    /// Get position for specific asset
    const Position* get_position(const AssetId& code) const;
    Position* get_position(const AssetId& code);

    /// Get all positions
    std::vector<const Position*> get_positions() const;

    /// Get order by ID
    const Order* get_order(const std::string& order_id) const;
    Order* get_order(const std::string& order_id);

    /// Get all orders
    std::vector<const Order*> get_orders() const;

    /// Get active orders
    std::vector<const Order*> get_active_orders() const;

    /// @}

    /// Account operations
    /// @{

    /// Update account with price changes
    void on_price_change(const AssetId& code, Price new_price,
                        const std::string& datetime);

    /// Update all positions with market data
    void update_market_data(const std::unordered_map<AssetId, Price>& prices,
                           const std::string& datetime);

    /// Settle account (end of day)
    void settle();

    /// Change current datetime (for backtesting)
    void change_datetime(const std::string& datetime);

    /// Deposit funds
    void deposit(Amount amount);

    /// Withdraw funds
    bool withdraw(Amount amount);

    /// @}

    /// Serialization
    /// @{

    /// Convert to JSON
    nlohmann::json to_json() const;

    /// Create from JSON
    static std::unique_ptr<Account> from_json(const nlohmann::json& j);

    /// String representation
    std::string to_string() const;

    /// @}

    /// Validation and checks
    /// @{

    /// Check if enough funds available for order
    bool check_available_funds(Amount required_amount) const;

    /// Check if position can be closed
    bool can_close_position(const AssetId& code, Volume volume, bool is_today) const;

    /// Validate order parameters
    bool validate_order(const AssetId& code, Volume volume, Price price,
                       Direction direction) const;

    /// @}

private:
    /// Internal order processing
    std::unique_ptr<Order> send_order(const AssetId& code, Volume volume,
                                     const std::string& datetime,
                                     int direction, Price price,
                                     const std::string& offset = "",
                                     const std::string& order_type = "LIMIT");

    /// Process order execution
    void process_order_execution(Order* order, Volume fill_volume, Price fill_price);

    /// Update account info
    void update_account_info();

    /// Calculate required margin for order
    Amount calculate_order_margin(const AssetId& code, Volume volume,
                                 Direction direction) const;

    /// Freeze funds for order
    void freeze_funds(Amount amount);

    /// Unfreeze funds
    void unfreeze_funds(Amount amount);
};

/// Account factory functions
namespace account_factory {
    /// Create real trading account
    std::unique_ptr<Account> create_real_account(
        const std::string& account_id, const std::string& broker_name,
        const std::string& user_id, Amount initial_balance);

    /// Create simulation account
    std::unique_ptr<Account> create_sim_account(
        const std::string& account_id, const std::string& user_id,
        Amount initial_balance);

    /// Create backtest account
    std::unique_ptr<Account> create_backtest_account(
        const std::string& account_id, const std::string& user_id,
        Amount initial_balance);
}

} // namespace qaultra::account