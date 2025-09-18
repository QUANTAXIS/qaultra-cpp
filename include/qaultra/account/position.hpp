#pragma once

#include "../qaultra.hpp"
#include <nlohmann/json.hpp>

namespace qaultra::account {

/**
 * @brief Position holding information for an asset
 */
class Position {
public:
    AssetId code;                   ///< Asset identifier
    std::string account_id;         ///< Account identifier
    std::string user_id;           ///< User identifier
    std::string exchange_id;       ///< Exchange identifier

    // Long position
    Volume volume_long_today = 0.0;     ///< Long volume opened today
    Volume volume_long_his = 0.0;       ///< Long volume from previous days
    Price open_cost_long = 0.0;         ///< Total cost of long positions
    Price position_cost_long = 0.0;     ///< Position cost for long

    // Short position (for futures)
    Volume volume_short_today = 0.0;    ///< Short volume opened today
    Volume volume_short_his = 0.0;      ///< Short volume from previous days
    Price open_cost_short = 0.0;        ///< Total cost of short positions
    Price position_cost_short = 0.0;    ///< Position cost for short

    // Current market information
    Price latest_price = 0.0;           ///< Latest market price
    std::string latest_datetime;        ///< Latest update time

    // Margin and commission
    Price margin_long = 0.0;            ///< Margin for long positions
    Price margin_short = 0.0;           ///< Margin for short positions
    Price commission = 0.0;             ///< Total commission paid

    // Market preset information
    struct MarketPreset {
        std::string name;
        int unit_table = 1;             ///< Contract multiplier
        Price price_tick = 0.01;        ///< Minimum price tick
        Volume volume_tick = 1.0;       ///< Minimum volume tick
        Price buy_fee_ratio = 0.0;      ///< Buy fee ratio
        Price sell_fee_ratio = 0.0;     ///< Sell fee ratio
        Price min_fee = 0.0;            ///< Minimum fee
    } preset;

    /// Default constructor
    Position() = default;

    /// Constructor with asset code
    explicit Position(const AssetId& code);

    /// Constructor with full initialization
    Position(const AssetId& code, const std::string& account_id,
             const std::string& user_id, const std::string& exchange_id);

    /// Copy constructor
    Position(const Position&) = default;

    /// Move constructor
    Position(Position&&) = default;

    /// Assignment operators
    Position& operator=(const Position&) = default;
    Position& operator=(Position&&) = default;

    /// Destructor
    ~Position() = default;

    /// Get total long volume
    Volume volume_long() const;

    /// Get total short volume
    Volume volume_short() const;

    /// Get net volume (long - short)
    Volume volume_net() const;

    /// Check if position is long
    bool is_long() const;

    /// Check if position is short
    bool is_short() const;

    /// Check if position is flat (no holdings)
    bool is_flat() const;

    /// Get total margin required
    Price margin() const;

    /// Get average long price
    Price avg_price_long() const;

    /// Get average short price
    Price avg_price_short() const;

    /// Get market value for long positions
    Price market_value_long() const;

    /// Get market value for short positions
    Price market_value_short() const;

    /// Get total market value
    Price market_value() const;

    /// Get float profit for long positions
    Price float_profit_long() const;

    /// Get float profit for short positions
    Price float_profit_short() const;

    /// Get total float profit
    Price float_profit() const;

    /// Get position profit for long positions
    Price position_profit_long() const;

    /// Get position profit for short positions
    Price position_profit_short() const;

    /// Get total position profit
    Price position_profit() const;

    /// Update position when price changes
    void on_price_change(Price new_price, const std::string& datetime);

    /// Update position when trade occurs
    /// @param price Trade price
    /// @param volume Trade volume (positive for buy, negative for sell)
    /// @param towards Trade direction (1=buy_open, -1=sell_open, 2=buy_close, -2=sell_close)
    void update_position(Price price, Volume volume, int towards);

    /// Close position (partial or full)
    void close_position(Volume volume, Price price, bool is_today = true);

    /// Settle position (end of day)
    void settle();

    /// Calculate commission for a trade
    Price calculate_commission(Volume volume, Price price, Direction direction) const;

    /// Convert to JSON
    nlohmann::json to_json() const;

    /// Create from JSON
    static Position from_json(const nlohmann::json& j);

    /// String representation
    std::string to_string() const;

    /// Comparison operators
    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;

private:
    void update_margin();
    void validate_trade(Volume volume, int towards) const;
};

/// Position manager for handling multiple positions
class PositionManager {
private:
    std::unordered_map<AssetId, std::unique_ptr<Position>> positions_;
    std::string account_id_;
    std::string user_id_;

public:
    /// Constructor
    PositionManager(const std::string& account_id, const std::string& user_id);

    /// Get or create position for asset
    Position* get_position(const AssetId& code);
    const Position* get_position(const AssetId& code) const;

    /// Get all positions
    std::vector<Position*> get_all_positions();
    std::vector<const Position*> get_all_positions() const;

    /// Get positions with holdings
    std::vector<Position*> get_active_positions();

    /// Update all positions with new prices
    void update_prices(const std::unordered_map<AssetId, Price>& prices,
                      const std::string& datetime);

    /// Settle all positions
    void settle_all();

    /// Get total market value
    Price total_market_value() const;

    /// Get total float profit
    Price total_float_profit() const;

    /// Get total position profit
    Price total_position_profit() const;

    /// Get total margin required
    Price total_margin() const;

    /// Get position count
    size_t size() const;
    bool empty() const;

    /// Clear all positions
    void clear();

    /// Remove position
    bool remove_position(const AssetId& code);

    /// Position statistics
    struct PositionStats {
        size_t total_positions = 0;
        size_t active_positions = 0;
        size_t long_positions = 0;
        size_t short_positions = 0;
        Price total_market_value = 0.0;
        Price total_float_profit = 0.0;
        Price total_margin = 0.0;
    };

    PositionStats get_statistics() const;

    /// Convert to JSON
    nlohmann::json to_json() const;

    /// Create from JSON
    static std::unique_ptr<PositionManager> from_json(const nlohmann::json& j);
};

} // namespace qaultra::account