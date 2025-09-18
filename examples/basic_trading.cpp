#include "qaultra/qaultra.hpp"
#include <iostream>
#include <iomanip>

using namespace qaultra;

int main() {
    std::cout << "QAULTRA C++ - Basic Trading Example\n";
    std::cout << "====================================\n\n";

    try {
        // Create a backtest trading account
        auto account = std::make_shared<account::QA_Account>(
            "test_account",           // account_cookie
            "test_portfolio",         // portfolio_cookie
            "user123",               // user_cookie
            1000000.0,               // initial_cash ($1M)
            false,                   // auto_reload
            "backtest"               // environment
        );

        std::cout << "Initial Account Status:\n";
        std::cout << "Cash: $" << std::fixed << std::setprecision(2) << account->get_cash() << "\n";
        std::cout << "Total Value: $" << account->get_total_value() << "\n\n";

        // Buy 1000 shares of AAPL at $150
        std::cout << "Executing Buy Order: 1000 AAPL @ $150\n";
        auto buy_order = account->buy("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0);

        std::cout << "Order Status: " << static_cast<int>(buy_order->status) << "\n";
        std::cout << "Cash After Buy: $" << account->get_cash() << "\n";
        std::cout << "Market Value: $" << account->get_market_value() << "\n\n";

        // Update market price (simulate price movement)
        std::cout << "Market Update: AAPL price moves to $155\n";
        account->on_price_change("AAPL", 155.0, "2024-01-15 16:00:00");

        std::cout << "Float P&L: $" << account->get_float_profit() << "\n";
        std::cout << "Total Value: $" << account->get_total_value() << "\n\n";

        // Get position details
        auto position = account->get_position("AAPL");
        if (position) {
            std::cout << "AAPL Position:\n";
            std::cout << "  Volume: " << position->volume_long << "\n";
            std::cout << "  Current Price: $" << position->price << "\n";
            std::cout << "  Market Value: $" << position->market_value << "\n";
            std::cout << "  Float Profit: $" << position->float_profit << "\n\n";
        }

        // Sell half the position
        std::cout << "Executing Sell Order: 500 AAPL @ $155\n";
        auto sell_order = account->sell("AAPL", 500.0, "2024-01-15 16:30:00", 155.0);

        std::cout << "Cash After Sell: $" << account->get_cash() << "\n";
        std::cout << "Market Value: $" << account->get_market_value() << "\n";
        std::cout << "Total Value: $" << account->get_total_value() << "\n\n";

        // Show all trades
        auto trades = account->get_trades();
        std::cout << "Trade History (" << trades.size() << " trades):\n";
        for (const auto& trade : trades) {
            std::cout << "  " << trade->trade_id << ": "
                     << (trade->direction == Direction::BUY ? "BUY" : "SELL")
                     << " " << trade->volume << " " << trade->code
                     << " @ $" << trade->price << "\n";
        }
        std::cout << "\n";

        // Export to QIFI format
        auto qifi = account->to_qifi();
        std::cout << "QIFI Export Summary:\n";
        std::cout << "Account: " << qifi.account_cookie << "\n";
        std::cout << "Total Value: $" << qifi.total_value << "\n";
        std::cout << "Positions: " << qifi.positions.size() << "\n";
        std::cout << "Orders: " << qifi.orders.size() << "\n";
        std::cout << "Trades: " << qifi.trades.size() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}