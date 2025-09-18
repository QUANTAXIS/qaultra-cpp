/**
 * @file basic_trading.cpp
 * @brief Basic trading example demonstrating QAULTRA C++ usage
 */

#include "qaultra/data/kline.hpp"
#include "qaultra/account/order.hpp"
#include <iostream>
#include <vector>
#include <chrono>

using namespace qaultra;

int main() {
    std::cout << "QAULTRA C++ Basic Trading Example\n";
    std::cout << "==================================\n\n";

    // Create some sample kline data
    std::cout << "1. Creating sample market data...\n";
    data::KlineCollection market_data;

    auto now = std::chrono::system_clock::now();
    for (int i = 0; i < 5; ++i) {
        auto timestamp = now + std::chrono::minutes(i);
        double base_price = 100.0 + i * 0.5;

        data::Kline kline("000001",
                         base_price,                    // open
                         base_price + 1.0,             // high
                         base_price - 0.5,             // low
                         base_price + 0.3,             // close
                         100000.0 + i * 1000,          // volume
                         (base_price + 0.3) * (100000.0 + i * 1000),  // amount
                         timestamp);

        market_data.add(std::move(kline));
        std::cout << "  Added: " << kline.to_string() << "\n";
    }

    // Display market statistics
    std::cout << "\n2. Market Statistics:\n";
    std::cout << "  Total candles: " << market_data.size() << "\n";
    std::cout << "  Max price: " << market_data.max_price() << "\n";
    std::cout << "  Min price: " << market_data.min_price() << "\n";
    std::cout << "  Average price: " << market_data.avg_price() << "\n";
    std::cout << "  Total volume: " << market_data.total_volume() << "\n";

    // Create and manage orders
    std::cout << "\n3. Creating and managing orders...\n";
    account::OrderManager order_manager;

    // Create a buy order
    auto buy_order = account::order_factory::create_limit_buy(
        "account_001", "000001", 1000.0, 100.5);

    std::cout << "  Created buy order: " << buy_order->to_string() << "\n";

    // Create a sell order
    auto sell_order = account::order_factory::create_limit_sell(
        "account_001", "000001", 500.0, 102.0);

    std::cout << "  Created sell order: " << sell_order->to_string() << "\n";

    // Add orders to manager
    std::string buy_order_id = buy_order->order_id;
    std::string sell_order_id = sell_order->order_id;

    order_manager.add_order(std::move(buy_order));
    order_manager.add_order(std::move(sell_order));

    std::cout << "  Total orders in manager: " << order_manager.size() << "\n";

    // Simulate partial fill
    std::cout << "\n4. Simulating order execution...\n";
    auto* buy_ptr = order_manager.get_order(buy_order_id);
    if (buy_ptr) {
        std::cout << "  Before fill - Status: " << static_cast<int>(buy_ptr->status)
                  << ", Remaining: " << buy_ptr->volume_left << "\n";

        // Partially fill the buy order
        buy_ptr->fill(300.0, 100.6);

        std::cout << "  After partial fill - Status: " << static_cast<int>(buy_ptr->status)
                  << ", Remaining: " << buy_ptr->volume_left
                  << ", Filled: " << buy_ptr->volume_filled
                  << ", Fill %: " << (buy_ptr->fill_percentage() * 100) << "%\n";

        // Complete the fill
        buy_ptr->fill(700.0, 100.7);

        std::cout << "  After complete fill - Status: " << static_cast<int>(buy_ptr->status)
                  << ", Filled: " << buy_ptr->volume_filled
                  << ", Fill %: " << (buy_ptr->fill_percentage() * 100) << "%\n";
    }

    // Display order statistics
    std::cout << "\n5. Order Statistics:\n";
    auto stats = order_manager.get_statistics();
    std::cout << "  Total orders: " << stats.total_count << "\n";
    std::cout << "  Active orders: " << stats.active_count << "\n";
    std::cout << "  Filled orders: " << stats.filled_count << "\n";
    std::cout << "  Cancelled orders: " << stats.cancelled_count << "\n";
    std::cout << "  Total value: " << stats.total_value << "\n";
    std::cout << "  Filled value: " << stats.filled_value << "\n";

    // Technical analysis example
    std::cout << "\n6. Technical Analysis:\n";
    auto closes = market_data.get_closes();
    std::cout << "  Close prices: ";
    for (size_t i = 0; i < closes.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << closes[i];
    }
    std::cout << "\n";

    // Simple moving average (last 3 periods)
    if (closes.size() >= 3) {
        double ma3 = 0.0;
        for (size_t i = closes.size() - 3; i < closes.size(); ++i) {
            ma3 += closes[i];
        }
        ma3 /= 3.0;
        std::cout << "  Simple MA(3): " << ma3 << "\n";

        // Price trend
        double latest_price = closes.back();
        if (latest_price > ma3) {
            std::cout << "  Trend: BULLISH (price above MA)\n";
        } else {
            std::cout << "  Trend: BEARISH (price below MA)\n";
        }
    }

    std::cout << "\nExample completed successfully!\n";
    return 0;
}