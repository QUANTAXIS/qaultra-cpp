#include <gtest/gtest.h>
#include "qaultra/account/order.hpp"

using namespace qaultra::account;

class OrderTest : public ::testing::Test {
protected:
    void SetUp() override {
        order = std::make_unique<Order>();
        order->order_id = "order_001";
        order->account_cookie = "account_001";
        order->instrument_id = "000001";
        order->direction = "BUY";
        order->price_order = 10.0;
        order->volume_orign = 1000.0;
        order->volume_left = 1000.0;
        order->status = "NEW";
    }

    std::unique_ptr<Order> order;
};

TEST_F(OrderTest, BasicProperties) {
    EXPECT_EQ(order->order_id, "order_001");
    EXPECT_EQ(order->account_cookie, "account_001");
    EXPECT_EQ(order->instrument_id, "000001");
    EXPECT_EQ(order->direction, "BUY");
    EXPECT_EQ(order->price_order, 10.0);
    EXPECT_EQ(order->volume_orign, 1000.0);
    EXPECT_EQ(order->volume_left, 1000.0);
    EXPECT_EQ(order->volume_fill, 0.0);
    EXPECT_EQ(order->status, "NEW");
}

TEST_F(OrderTest, OrderStatus) {
    EXPECT_TRUE(order->is_active());
    EXPECT_FALSE(order->is_completed());
    EXPECT_FALSE(order->is_partially_filled());
    EXPECT_NEAR(order->fill_percentage(), 0.0, 1e-9);
}

TEST_F(OrderTest, PartialFill) {
    // Fill 300 shares at 10.1
    EXPECT_TRUE(order->fill(300.0, 10.1));

    EXPECT_EQ(order->volume_filled, 300.0);
    EXPECT_EQ(order->volume_left, 700.0);
    EXPECT_EQ(order->status, OrderStatus::PARTIAL_FILLED);
    EXPECT_TRUE(order->is_partially_filled());
    EXPECT_NEAR(order->fill_percentage(), 0.3, 1e-9);
}

TEST_F(OrderTest, CompleteFill) {
    // Fill all shares
    EXPECT_TRUE(order->fill(1000.0, 10.1));

    EXPECT_EQ(order->volume_filled, 1000.0);
    EXPECT_EQ(order->volume_left, 0.0);
    EXPECT_EQ(order->status, OrderStatus::FILLED);
    EXPECT_FALSE(order->is_active());
    EXPECT_TRUE(order->is_completed());
    EXPECT_NEAR(order->fill_percentage(), 1.0, 1e-9);
}

TEST_F(OrderTest, OverFill) {
    // Try to fill more than available
    EXPECT_FALSE(order->fill(1500.0, 10.1));
    EXPECT_EQ(order->volume_filled, 0.0);
    EXPECT_EQ(order->status, OrderStatus::NEW);
}

TEST_F(OrderTest, Cancel) {
    order->cancel();
    EXPECT_EQ(order->status, OrderStatus::CANCELLED);
    EXPECT_FALSE(order->is_active());
    EXPECT_TRUE(order->is_completed());
}

TEST_F(OrderTest, Reject) {
    order->reject("Insufficient funds");
    EXPECT_EQ(order->status, OrderStatus::REJECTED);
    EXPECT_EQ(order->last_msg, "Insufficient funds");
    EXPECT_FALSE(order->is_active());
    EXPECT_TRUE(order->is_completed());
}

TEST_F(OrderTest, OrderValue) {
    EXPECT_NEAR(order->get_order_value(), 10000.0, 1e-9);
    EXPECT_NEAR(order->get_filled_value(), 0.0, 1e-9);

    order->fill(300.0, 10.1);
    EXPECT_NEAR(order->get_filled_value(), 3030.0, 1e-9);
}

TEST(OrderManagerTest, BasicOperations) {
    OrderManager manager;

    auto order1 = std::make_unique<Order>("order_001", "account_001", "000001",
                                         Direction::BUY, 10.0, 1000.0);
    auto order2 = std::make_unique<Order>("order_002", "account_001", "000002",
                                         Direction::SELL, 20.0, 500.0);

    // Add orders
    EXPECT_TRUE(manager.add_order(std::move(order1)));
    EXPECT_TRUE(manager.add_order(std::move(order2)));
    EXPECT_EQ(manager.size(), 2);

    // Get order
    auto* retrieved_order = manager.get_order("order_001");
    ASSERT_NE(retrieved_order, nullptr);
    EXPECT_EQ(retrieved_order->order_id, "order_001");

    // Get active orders
    auto active_orders = manager.get_active_orders();
    EXPECT_EQ(active_orders.size(), 2);

    // Cancel all orders
    manager.cancel_all_orders();
    for (auto* order : manager.get_active_orders()) {
        EXPECT_EQ(order->status, OrderStatus::CANCELLED);
    }
}

TEST(OrderFactoryTest, CreateOrders) {
    auto buy_order = order_factory::create_market_buy("account_001", "000001", 1000.0);
    ASSERT_NE(buy_order, nullptr);
    EXPECT_EQ(buy_order->direction, Direction::BUY);
    EXPECT_EQ(buy_order->volume, 1000.0);
    EXPECT_EQ(buy_order->price, 0.0); // Market order

    auto limit_sell = order_factory::create_limit_sell("account_001", "000002", 500.0, 25.0);
    ASSERT_NE(limit_sell, nullptr);
    EXPECT_EQ(limit_sell->direction, Direction::SELL);
    EXPECT_EQ(limit_sell->volume, 500.0);
    EXPECT_EQ(limit_sell->price, 25.0);
}