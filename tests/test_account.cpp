#include <gtest/gtest.h>
#include "qaultra/account/account_full.hpp"
#include "qaultra/protocol/qifi.hpp"

using namespace qaultra;

class AccountTest : public ::testing::Test {
protected:
    void SetUp() override {
        account = std::make_shared<account::QA_Account>(
            "test_account",
            "test_portfolio",
            "test_user",
            1000000.0,
            false,
            "backtest"
        );
    }

    std::shared_ptr<account::QA_Account> account;
};

TEST_F(AccountTest, InitialState) {
    EXPECT_EQ(account->get_cash(), 1000000.0);
    EXPECT_EQ(account->get_total_value(), 1000000.0);
    EXPECT_EQ(account->get_float_profit(), 0.0);
    EXPECT_EQ(account->get_market_value(), 0.0);
    EXPECT_TRUE(account->get_positions().empty());
    EXPECT_TRUE(account->get_orders().empty());
    EXPECT_TRUE(account->get_trades().empty());
}

TEST_F(AccountTest, BuyOrderExecution) {
    auto order = account->buy("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0);

    EXPECT_EQ(order->code, "AAPL");
    EXPECT_EQ(order->volume, 1000.0);
    EXPECT_EQ(order->price, 150.0);
    EXPECT_EQ(order->direction, Direction::BUY);
    EXPECT_EQ(order->status, OrderStatus::FILLED);

    // Check account state after buy
    EXPECT_LT(account->get_cash(), 1000000.0); // Cash reduced
    EXPECT_GT(account->get_market_value(), 0.0); // Market value increased

    // Check position
    auto position = account->get_position("AAPL");
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(position->volume_long, 1000.0);
    EXPECT_EQ(position->price, 150.0);
}

TEST_F(AccountTest, SellOrderExecution) {
    // First buy
    account->buy("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0);

    // Then sell
    auto sell_order = account->sell("AAPL", 500.0, "2024-01-15 10:30:00", 155.0);

    EXPECT_EQ(sell_order->status, OrderStatus::FILLED);

    // Check remaining position
    auto position = account->get_position("AAPL");
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(position->volume_long, 500.0);
}

TEST_F(AccountTest, PriceUpdate) {
    account->buy("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0);

    double initial_value = account->get_total_value();

    // Price increases by $5
    account->on_price_change("AAPL", 155.0, "2024-01-15 16:00:00");

    EXPECT_GT(account->get_float_profit(), 0.0);
    EXPECT_GT(account->get_total_value(), initial_value);

    // Check position update
    auto position = account->get_position("AAPL");
    EXPECT_EQ(position->price, 155.0);
    EXPECT_EQ(position->market_value, 1000.0 * 155.0);
}

TEST_F(AccountTest, FuturesTrading) {
    // Open long position
    auto buy_open = account->buy_open("ES2024", 10.0, "2024-01-15 09:30:00", 4500.0);
    EXPECT_EQ(buy_open->status, OrderStatus::FILLED);

    auto position = account->get_position("ES2024");
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(position->volume_long, 10.0);

    // Close position
    auto sell_close = account->sell_close("ES2024", 10.0, "2024-01-15 16:00:00", 4520.0);
    EXPECT_EQ(sell_close->status, OrderStatus::FILLED);

    position = account->get_position("ES2024");
    EXPECT_EQ(position->volume_long, 0.0);
}

TEST_F(AccountTest, QIFIExportImport) {
    // Create some activity
    account->buy("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0);
    account->on_price_change("AAPL", 155.0, "2024-01-15 16:00:00");

    // Export to QIFI
    auto qifi = account->to_qifi();

    EXPECT_EQ(qifi.account_cookie, "test_account");
    EXPECT_EQ(qifi.portfolio_cookie, "test_portfolio");
    EXPECT_EQ(qifi.user_cookie, "test_user");
    EXPECT_EQ(qifi.positions.size(), 1);
    EXPECT_EQ(qifi.orders.size(), 1);
    EXPECT_EQ(qifi.trades.size(), 1);

    // Create new account and import
    auto new_account = std::make_shared<account::QA_Account>(
        "imported_account", "imported_portfolio", "imported_user",
        0.0, false, "backtest"
    );

    new_account->from_qifi(qifi);

    EXPECT_EQ(new_account->get_cash(), account->get_cash());
    EXPECT_EQ(new_account->get_total_value(), account->get_total_value());
    EXPECT_EQ(new_account->get_positions().size(), 1);
}

TEST_F(AccountTest, CommissionCalculation) {
    double initial_cash = account->get_cash();
    auto order = account->buy("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0);

    // Cash should be reduced by more than just stock value due to commission
    double stock_value = 1000.0 * 150.0;
    double cash_spent = initial_cash - account->get_cash();

    EXPECT_GT(cash_spent, stock_value); // Commission included
    EXPECT_GT(order->commission, 0.0);
}

TEST_F(AccountTest, InsufficientFundsError) {
    // Try to buy more than available cash
    EXPECT_THROW(
        account->buy("AAPL", 10000.0, "2024-01-15 09:30:00", 150.0),
        std::runtime_error
    );
}

TEST_F(AccountTest, InsufficientPositionError) {
    // Try to sell without position
    EXPECT_THROW(
        account->sell("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0),
        std::runtime_error
    );
}

TEST_F(AccountTest, MultiplePositions) {
    account->buy("AAPL", 1000.0, "2024-01-15 09:30:00", 150.0);
    account->buy("MSFT", 500.0, "2024-01-15 09:30:00", 300.0);
    account->buy("GOOGL", 200.0, "2024-01-15 09:30:00", 2500.0);

    auto positions = account->get_positions();
    EXPECT_EQ(positions.size(), 3);

    // Update prices
    account->on_price_change("AAPL", 155.0, "2024-01-15 16:00:00");
    account->on_price_change("MSFT", 310.0, "2024-01-15 16:00:00");
    account->on_price_change("GOOGL", 2450.0, "2024-01-15 16:00:00");

    double expected_pnl = (155.0 - 150.0) * 1000.0 +  // AAPL
                          (310.0 - 300.0) * 500.0 +    // MSFT
                          (2450.0 - 2500.0) * 200.0;   // GOOGL

    EXPECT_NEAR(account->get_float_profit(), expected_pnl, 1.0); // Allow small tolerance
}

// Performance tests
TEST_F(AccountTest, PerformanceQuick) {
    auto start = std::chrono::high_resolution_clock::now();

    // Execute 1000 orders
    for (int i = 0; i < 1000; ++i) {
        std::string symbol = "SYM" + std::to_string(i % 10);
        double price = 100.0 + (i % 100) * 0.1;
        account->buy(symbol, 100.0, "2024-01-15 09:30:00", price);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "1000 orders executed in " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 1000); // Should complete in under 1 second
}

// Thread safety test
TEST_F(AccountTest, ThreadSafety) {
    const int num_threads = 4;
    const int orders_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, orders_per_thread]() {
            for (int i = 0; i < orders_per_thread; ++i) {
                std::string symbol = "SYM" + std::to_string(t);
                double price = 100.0 + i * 0.1;
                account->buy(symbol, 10.0, "2024-01-15 09:30:00", price);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all orders were processed
    auto orders = account->get_orders();
    EXPECT_EQ(orders.size(), num_threads * orders_per_thread);
}