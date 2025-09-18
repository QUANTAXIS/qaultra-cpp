#pragma once

/**
 * @file qaultra.hpp
 * @brief Main header for QAULTRA-CPP - High-performance quantitative trading system
 * @version 1.0.0
 *
 * QAULTRA-CPP is a C++ port of the QARS quantitative trading system,
 * implementing comprehensive trading infrastructure including account management,
 * market simulation, backtesting engines, and portfolio analytics.
 */

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

// Forward declarations to avoid circular dependencies
namespace qaultra {
    namespace account {
        class Account;
        class Position;
        class Order;
        class PositionManager;
        class OrderManager;
    }
    namespace data {
        class Kline;
        class KlineCollection;
    }
    namespace market {
        class MarketData;
        class OrderBook;
        class MatchEngine;
    }
    namespace engine {
        class Backtest;
        class Strategy;
    }
}

// Core modules - include only when needed
// Users should include specific headers they need

namespace qaultra {

/// Version information
constexpr const char* VERSION = "1.0.0";
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

/// Common type definitions
using Price = double;
using Volume = double;
using Amount = double;
using Timestamp = std::chrono::system_clock::time_point;
using Duration = std::chrono::milliseconds;

/// Asset identifier
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

/// Position side
enum class PositionSide {
    LONG = 1,
    SHORT = -1
};

/// Market type
enum class MarketType {
    STOCK,
    FUTURE,
    OPTION,
    FOREX
};

/// Common utility functions
namespace utils {
    /// Convert string to timestamp
    Timestamp parse_datetime(const std::string& datetime_str);

    /// Convert timestamp to string
    std::string format_datetime(const Timestamp& ts);

    /// Get current timestamp
    Timestamp now();

    /// Generate UUID
    std::string generate_uuid();
}

} // namespace qaultra