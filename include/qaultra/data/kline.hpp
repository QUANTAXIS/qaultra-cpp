#pragma once

#include "../qaultra.hpp"
#include <nlohmann/json.hpp>

namespace qaultra::data {

/**
 * @brief K-line data structure representing OHLCV market data
 */
class Kline {
public:
    AssetId code;           ///< Asset identifier
    Price open;             ///< Opening price
    Price high;             ///< Highest price
    Price low;              ///< Lowest price
    Price close;            ///< Closing price
    Volume volume;          ///< Trading volume
    Amount amount;          ///< Trading amount
    Timestamp datetime;     ///< Timestamp

    /// Default constructor
    Kline() = default;

    /// Constructor with parameters
    Kline(const AssetId& code, Price open, Price high, Price low,
          Price close, Volume volume, Amount amount, const Timestamp& datetime)
        : code(code), open(open), high(high), low(low),
          close(close), volume(volume), amount(amount), datetime(datetime) {}

    /// Copy constructor
    Kline(const Kline&) = default;

    /// Move constructor
    Kline(Kline&&) = default;

    /// Assignment operators
    Kline& operator=(const Kline&) = default;
    Kline& operator=(Kline&&) = default;

    /// Destructor
    ~Kline() = default;

    /// Get the typical price (HLC/3)
    Price typical_price() const;

    /// Get the weighted close price (HLCC/4)
    Price weighted_close() const;

    /// Check if this is a bullish candle
    bool is_bullish() const;

    /// Check if this is a bearish candle
    bool is_bearish() const;

    /// Get the body size (absolute difference between open and close)
    Price body_size() const;

    /// Get the upper shadow size
    Price upper_shadow() const;

    /// Get the lower shadow size
    Price lower_shadow() const;

    /// Get the price range (high - low)
    Price range() const;

    /// Convert to JSON
    nlohmann::json to_json() const;

    /// Create from JSON
    static Kline from_json(const nlohmann::json& j);

    /// Comparison operators
    bool operator==(const Kline& other) const;
    bool operator!=(const Kline& other) const;
    bool operator<(const Kline& other) const;  // Compare by datetime
    bool operator>(const Kline& other) const;

    /// String representation
    std::string to_string() const;
};

/// Collection of Klines with utility functions
class KlineCollection {
private:
    std::vector<Kline> data_;

public:
    /// Default constructor
    KlineCollection() = default;

    /// Constructor with initial data
    explicit KlineCollection(std::vector<Kline> data);

    /// Add a single kline
    void add(const Kline& kline);
    void add(Kline&& kline);

    /// Add multiple klines
    void add_batch(const std::vector<Kline>& klines);
    void add_batch(std::vector<Kline>&& klines);

    /// Get kline count
    size_t size() const;
    bool empty() const;

    /// Access operators
    const Kline& operator[](size_t index) const;
    Kline& operator[](size_t index);

    /// Iterator support
    auto begin() -> decltype(data_.begin());
    auto end() -> decltype(data_.end());
    auto begin() const -> decltype(data_.begin());
    auto end() const -> decltype(data_.end());

    /// Get latest kline
    const Kline& latest() const;
    Kline& latest();

    /// Get klines in time range
    KlineCollection get_range(const Timestamp& start, const Timestamp& end) const;

    /// Get last N klines
    KlineCollection get_last(size_t count) const;

    /// Statistical functions
    Price max_price() const;
    Price min_price() const;
    Price avg_price() const;
    Volume total_volume() const;

    /// Technical analysis helpers
    std::vector<Price> get_closes() const;
    std::vector<Price> get_highs() const;
    std::vector<Price> get_lows() const;
    std::vector<Volume> get_volumes() const;

    /// Sort by datetime
    void sort();

    /// Clear all data
    void clear();

    /// Convert to JSON
    nlohmann::json to_json() const;

    /// Create from JSON
    static KlineCollection from_json(const nlohmann::json& j);
};

} // namespace qaultra::data