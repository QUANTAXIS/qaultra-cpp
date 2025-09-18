#include <gtest/gtest.h>
#include "qaultra/data/kline.hpp"
#include <chrono>

using namespace qaultra::data;

class KlineTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto now = std::chrono::system_clock::now();
        kline = Kline("000001", 10.0, 11.0, 9.5, 10.8, 1000000.0, 10800000.0, now);
    }

    Kline kline;
};

TEST_F(KlineTest, BasicProperties) {
    EXPECT_EQ(kline.code, "000001");
    EXPECT_EQ(kline.open, 10.0);
    EXPECT_EQ(kline.high, 11.0);
    EXPECT_EQ(kline.low, 9.5);
    EXPECT_EQ(kline.close, 10.8);
    EXPECT_EQ(kline.volume, 1000000.0);
    EXPECT_EQ(kline.amount, 10800000.0);
}

TEST_F(KlineTest, CalculatedProperties) {
    EXPECT_NEAR(kline.typical_price(), (11.0 + 9.5 + 10.8) / 3.0, 1e-9);
    EXPECT_NEAR(kline.weighted_close(), (11.0 + 9.5 + 10.8 + 10.8) / 4.0, 1e-9);
    EXPECT_TRUE(kline.is_bullish());
    EXPECT_FALSE(kline.is_bearish());
    EXPECT_NEAR(kline.body_size(), 0.8, 1e-9);
    EXPECT_NEAR(kline.range(), 1.5, 1e-9);
}

TEST_F(KlineTest, JsonSerialization) {
    auto json_obj = kline.to_json();
    auto restored_kline = Kline::from_json(json_obj);

    EXPECT_EQ(restored_kline.code, kline.code);
    EXPECT_EQ(restored_kline.open, kline.open);
    EXPECT_EQ(restored_kline.high, kline.high);
    EXPECT_EQ(restored_kline.low, kline.low);
    EXPECT_EQ(restored_kline.close, kline.close);
    EXPECT_EQ(restored_kline.volume, kline.volume);
    EXPECT_EQ(restored_kline.amount, kline.amount);
}

TEST(KlineCollectionTest, BasicOperations) {
    KlineCollection collection;
    auto now = std::chrono::system_clock::now();

    // Add some klines
    collection.add(Kline("000001", 10.0, 11.0, 9.5, 10.8, 1000000.0, 10800000.0, now));
    collection.add(Kline("000001", 10.8, 12.0, 10.5, 11.5, 1200000.0, 13800000.0,
                         now + std::chrono::minutes(1)));

    EXPECT_EQ(collection.size(), 2);
    EXPECT_FALSE(collection.empty());

    // Test statistical functions
    EXPECT_NEAR(collection.max_price(), 12.0, 1e-9);
    EXPECT_NEAR(collection.min_price(), 9.5, 1e-9);
    EXPECT_NEAR(collection.avg_price(), (10.8 + 11.5) / 2.0, 1e-9);
    EXPECT_NEAR(collection.total_volume(), 2200000.0, 1e-9);
}

TEST(KlineCollectionTest, TechnicalAnalysis) {
    KlineCollection collection;
    auto now = std::chrono::system_clock::now();

    collection.add(Kline("000001", 10.0, 11.0, 9.5, 10.8, 1000000.0, 10800000.0, now));
    collection.add(Kline("000001", 10.8, 12.0, 10.5, 11.5, 1200000.0, 13800000.0,
                         now + std::chrono::minutes(1)));

    auto closes = collection.get_closes();
    EXPECT_EQ(closes.size(), 2);
    EXPECT_EQ(closes[0], 10.8);
    EXPECT_EQ(closes[1], 11.5);

    auto highs = collection.get_highs();
    EXPECT_EQ(highs.size(), 2);
    EXPECT_EQ(highs[0], 11.0);
    EXPECT_EQ(highs[1], 12.0);

    auto lows = collection.get_lows();
    EXPECT_EQ(lows.size(), 2);
    EXPECT_EQ(lows[0], 9.5);
    EXPECT_EQ(lows[1], 10.5);

    auto volumes = collection.get_volumes();
    EXPECT_EQ(volumes.size(), 2);
    EXPECT_EQ(volumes[0], 1000000.0);
    EXPECT_EQ(volumes[1], 1200000.0);
}