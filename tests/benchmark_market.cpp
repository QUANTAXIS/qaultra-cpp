/**
 * @file benchmark_market.cpp
 * @brief 市场模块性能基准测试
 *
 * 测试市场数据处理、撮合引擎、历史数据查询等模块的性能
 */

#include <benchmark/benchmark.h>
#include "qaultra/market/simmarket.hpp"
#include "qaultra/market/matchengine/orderbook.hpp"
#include "qaultra/market/matchengine/order_queues.hpp"
#include "qaultra/data/unified_datatype.hpp"
#include <random>
#include <thread>
#include <vector>

using namespace qaultra::market;
using namespace qaultra::data;

// ===== 模拟市场性能测试 =====

static void BM_SimMarket_DataGeneration(benchmark::State& state) {
    const int num_ticks = state.range(0);

    for (auto _ : state) {
        qaultra::market market;
        market.set_code("000001.SZ");
        market.set_init_price(10.0);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_ticks; ++i) {
            auto tick_data = market.generate_next_tick();
            benchmark::DoNotOptimize(tick_data);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["TicksPerSecond"] = benchmark::Counter(
            num_ticks * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_SimMarket_DataGeneration)->Range(1000, 100000);

static void BM_SimMarket_MultiSymbolGeneration(benchmark::State& state) {
    const int num_symbols = state.range(0);
    const int ticks_per_symbol = 1000;

    for (auto _ : state) {
        std::vector<std::unique_ptr<SimMarket>> markets;

        // 创建多个市场
        for (int i = 0; i < num_symbols; ++i) {
            auto market = std::make_unique<SimMarket>();
            market->set_code("STOCK_" + std::to_string(i));
            market->set_init_price(10.0 + i * 0.1);
            markets.push_back(std::move(market));
        }

        auto start = std::chrono::high_resolution_clock::now();

        // 同时为所有市场生成数据
        for (int tick = 0; tick < ticks_per_symbol; ++tick) {
            for (auto& market : markets) {
                auto tick_data = market->generate_next_tick();
                benchmark::DoNotOptimize(tick_data);
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["TotalTicksPerSecond"] = benchmark::Counter(
            num_symbols * ticks_per_symbol * 1000000.0 / elapsed.count(),
            benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_SimMarket_MultiSymbolGeneration)->Range(1, 100);

// ===== 撮合引擎性能测试 =====

static void BM_OrderBook_OrderInsertion(benchmark::State& state) {
    using namespace matchengine;

    OrderBook orderbook("000001.SZ");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(9.5, 10.5);
    std::uniform_real_distribution<> volume_dist(100.0, 10000.0);

    int order_count = 0;

    for (auto _ : state) {
        Order order;
        order.order_id = "BENCH_ORDER_" + std::to_string(order_count++);
        order.symbol = "000001.SZ";
        order.side = (order_count % 2 == 0) ? OrderSide::Buy : OrderSide::Sell;
        order.order_type = OrderType::Limit;
        order.price = price_dist(gen);
        order.quantity = volume_dist(gen);
        order.timestamp = std::chrono::steady_clock::now();

        auto result = orderbook.add_order(order);
        benchmark::DoNotOptimize(result);
    }

    state.counters["OrdersProcessed"] = order_count;
}
BENCHMARK(BM_OrderBook_OrderInsertion);

static void BM_OrderBook_MatchingPerformance(benchmark::State& state) {
    using namespace matchengine;

    OrderBook orderbook("000001.SZ");
    const int num_orders = state.range(0);

    // 预先插入一些买单
    for (int i = 0; i < num_orders / 2; ++i) {
        Order buy_order;
        buy_order.order_id = "BUY_" + std::to_string(i);
        buy_order.symbol = "000001.SZ";
        buy_order.side = OrderSide::Buy;
        buy_order.order_type = OrderType::Limit;
        buy_order.price = 10.0 - i * 0.001;  // 递减价格
        buy_order.quantity = 100.0;
        buy_order.timestamp = std::chrono::steady_clock::now();
        orderbook.add_order(buy_order);
    }

    int matches_count = 0;

    for (auto _ : state) {
        // 插入卖单，触发撮合
        Order sell_order;
        sell_order.order_id = "SELL_" + std::to_string(matches_count);
        sell_order.symbol = "000001.SZ";
        sell_order.side = OrderSide::Sell;
        sell_order.order_type = OrderType::Market;  // 市价单，立即撮合
        sell_order.quantity = 100.0;
        sell_order.timestamp = std::chrono::steady_clock::now();

        auto result = orderbook.add_order(sell_order);
        if (result.matched) {
            matches_count++;
        }
        benchmark::DoNotOptimize(result);
    }

    state.counters["MatchesPerSecond"] = benchmark::Counter(
        matches_count, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_OrderBook_MatchingPerformance)->Range(100, 10000);

static void BM_OrderQueues_PriceLevel(benchmark::State& state) {
    using namespace matchengine;

    PriceLevelQueue queue(10.0, OrderSide::Buy);
    const int num_orders = state.range(0);

    for (auto _ : state) {
        // 清空队列
        queue.clear();

        auto start = std::chrono::high_resolution_clock::now();

        // 添加订单到价格档位
        for (int i = 0; i < num_orders; ++i) {
            Order order;
            order.order_id = "QUEUE_ORDER_" + std::to_string(i);
            order.symbol = "000001.SZ";
            order.side = OrderSide::Buy;
            order.price = 10.0;
            order.quantity = 100.0;
            order.timestamp = std::chrono::steady_clock::now();

            queue.add_order(order);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["OrdersPerSecond"] = benchmark::Counter(
            num_orders * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_OrderQueues_PriceLevel)->Range(100, 10000);

// ===== 市场数据结构性能测试 =====

static void BM_MarketData_KLineGeneration(benchmark::State& state) {
    const int num_klines = state.range(0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(9.5, 10.5);
    std::uniform_real_distribution<> volume_dist(1000.0, 100000.0);

    for (auto _ : state) {
        std::vector<KLine> klines;
        klines.reserve(num_klines);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_klines; ++i) {
            KLine kline;
            kline.symbol = "000001.SZ";
            kline.datetime = "2024-01-01 09:30:00";
            kline.open = price_dist(gen);
            kline.high = kline.open + 0.1;
            kline.low = kline.open - 0.1;
            kline.close = price_dist(gen);
            kline.volume = volume_dist(gen);
            kline.amount = kline.close * kline.volume;

            klines.push_back(std::move(kline));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["KLinesPerSecond"] = benchmark::Counter(
            num_klines * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(klines);
    }
}
BENCHMARK(BM_MarketData_KLineGeneration)->Range(1000, 100000);

static void BM_MarketData_TickDataProcessing(benchmark::State& state) {
    const int num_ticks = state.range(0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(9.95, 10.05);
    std::uniform_real_distribution<> volume_dist(100.0, 10000.0);

    for (auto _ : state) {
        std::vector<TickData> ticks;
        ticks.reserve(num_ticks);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_ticks; ++i) {
            TickData tick;
            tick.symbol = "000001.SZ";
            tick.datetime = "2024-01-01 09:30:00." + std::to_string(i % 1000);
            tick.last_price = price_dist(gen);
            tick.volume = volume_dist(gen);
            tick.amount = tick.last_price * tick.volume;

            // 模拟5档行情
            for (int j = 0; j < 5; ++j) {
                tick.bid_prices[j] = tick.last_price - (j + 1) * 0.01;
                tick.ask_prices[j] = tick.last_price + (j + 1) * 0.01;
                tick.bid_volumes[j] = volume_dist(gen);
                tick.ask_volumes[j] = volume_dist(gen);
            }

            ticks.push_back(std::move(tick));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["TicksPerSecond"] = benchmark::Counter(
            num_ticks * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(ticks);
    }
}
BENCHMARK(BM_MarketData_TickDataProcessing)->Range(1000, 100000);

// ===== 并发市场数据处理测试 =====

static void BM_Market_ConcurrentDataProcessing(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int data_per_thread = 1000;

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> total_processed{0};

        auto start = std::chrono::high_resolution_clock::now();

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&total_processed, data_per_thread, t]() {
                SimMarket market;
                market.set_code("STOCK_" + std::to_string(t));
                market.set_init_price(10.0 + t * 0.1);

                for (int i = 0; i < data_per_thread; ++i) {
                    auto tick_data = market.generate_next_tick();
                    total_processed++;

                    // 模拟一些处理时间
                    volatile double dummy = 0;
                    for (int j = 0; j < 100; ++j) {
                        dummy += tick_data.close * j;
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["ThreadCount"] = num_threads;
        state.counters["TotalProcessedPerSecond"] = benchmark::Counter(
            total_processed.load() * 1000000.0 / elapsed.count(),
            benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_Market_ConcurrentDataProcessing)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// ===== 历史数据查询性能测试 =====

static void BM_HistoricalData_RangeQuery(benchmark::State& state) {
    const int data_size = state.range(0);

    // 创建大量历史数据
    std::vector<KLine> historical_data;
    historical_data.reserve(data_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(9.5, 10.5);

    for (int i = 0; i < data_size; ++i) {
        KLine kline;
        kline.symbol = "000001.SZ";
        kline.datetime = "2024-01-01 09:30:00";
        kline.open = price_dist(gen);
        kline.close = price_dist(gen);
        kline.volume = 10000.0;
        historical_data.push_back(kline);
    }

    for (auto _ : state) {
        // 模拟范围查询（查询最近10%的数据）
        int query_start = data_size * 0.9;
        int query_end = data_size;

        std::vector<KLine> result;
        result.reserve(query_end - query_start);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = query_start; i < query_end; ++i) {
            result.push_back(historical_data[i]);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["QuerySize"] = query_end - query_start;
        state.counters["QueryPerSecond"] = benchmark::Counter(
            1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_HistoricalData_RangeQuery)->Range(10000, 1000000);

// ===== 技术指标计算性能测试 =====

static void BM_TechnicalIndicators_SMA(benchmark::State& state) {
    const int data_size = state.range(0);
    const int period = 20;

    // 准备价格数据
    std::vector<double> prices(data_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_changes(0.0, 0.01);

    double price = 10.0;
    for (int i = 0; i < data_size; ++i) {
        price += price_changes(gen);
        prices[i] = price;
    }

    for (auto _ : state) {
        std::vector<double> sma_result;
        sma_result.reserve(data_size - period + 1);

        auto start = std::chrono::high_resolution_clock::now();

        // 计算简单移动平均
        for (int i = period - 1; i < data_size; ++i) {
            double sum = 0.0;
            for (int j = i - period + 1; j <= i; ++j) {
                sum += prices[j];
            }
            sma_result.push_back(sum / period);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["DataPoints"] = data_size;
        state.counters["CalculationsPerSecond"] = benchmark::Counter(
            (data_size - period + 1) * 1000000.0 / elapsed.count(),
            benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(sma_result);
    }
}
BENCHMARK(BM_TechnicalIndicators_SMA)->Range(1000, 100000);

static void BM_TechnicalIndicators_MACD(benchmark::State& state) {
    const int data_size = state.range(0);

    // 准备价格数据
    std::vector<double> prices(data_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_changes(0.0, 0.01);

    double price = 10.0;
    for (int i = 0; i < data_size; ++i) {
        price += price_changes(gen);
        prices[i] = price;
    }

    for (auto _ : state) {
        std::vector<double> ema_12(data_size);
        std::vector<double> ema_26(data_size);
        std::vector<double> macd(data_size);

        auto start = std::chrono::high_resolution_clock::now();

        // 计算EMA12
        ema_12[0] = prices[0];
        double multiplier_12 = 2.0 / (12 + 1);
        for (int i = 1; i < data_size; ++i) {
            ema_12[i] = (prices[i] - ema_12[i-1]) * multiplier_12 + ema_12[i-1];
        }

        // 计算EMA26
        ema_26[0] = prices[0];
        double multiplier_26 = 2.0 / (26 + 1);
        for (int i = 1; i < data_size; ++i) {
            ema_26[i] = (prices[i] - ema_26[i-1]) * multiplier_26 + ema_26[i-1];
        }

        // 计算MACD
        for (int i = 0; i < data_size; ++i) {
            macd[i] = ema_12[i] - ema_26[i];
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["MACDCalculationsPerSecond"] = benchmark::Counter(
            data_size * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(macd);
    }
}
BENCHMARK(BM_TechnicalIndicators_MACD)->Range(1000, 100000);

// ===== 实时市场数据流处理 =====

static void BM_RealTimeMarket_DataStream(benchmark::State& state) {
    const int stream_duration_ms = 1000; // 1秒的数据流
    const int tick_interval_us = 1000;   // 1ms一个tick

    for (auto _ : state) {
        SimMarket market;
        market.set_code("000001.SZ");
        market.set_init_price(10.0);

        std::vector<TickData> tick_stream;
        tick_stream.reserve(stream_duration_ms);

        auto start = std::chrono::high_resolution_clock::now();

        // 模拟实时数据流
        for (int tick = 0; tick < stream_duration_ms; ++tick) {
            auto tick_data = market.generate_next_tick();
            tick_stream.push_back(tick_data);

            // 模拟处理延迟
            std::this_thread::sleep_for(std::chrono::microseconds(tick_interval_us));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        state.counters["StreamDurationMs"] = elapsed.count();
        state.counters["TicksPerSecond"] = benchmark::Counter(
            tick_stream.size() * 1000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(tick_stream);
    }
}
BENCHMARK(BM_RealTimeMarket_DataStream);

// ===== 订单簿深度性能测试 =====

static void BM_OrderBook_DepthCalculation(benchmark::State& state) {
    using namespace matchengine;

    OrderBook orderbook("000001.SZ");
    const int num_levels = state.range(0);

    // 填充订单簿
    for (int i = 0; i < num_levels; ++i) {
        // 买单
        Order buy_order;
        buy_order.order_id = "BUY_" + std::to_string(i);
        buy_order.symbol = "000001.SZ";
        buy_order.side = OrderSide::Buy;
        buy_order.order_type = OrderType::Limit;
        buy_order.price = 10.0 - i * 0.01;
        buy_order.quantity = 100.0 * (i + 1);
        orderbook.add_order(buy_order);

        // 卖单
        Order sell_order;
        sell_order.order_id = "SELL_" + std::to_string(i);
        sell_order.symbol = "000001.SZ";
        sell_order.side = OrderSide::Sell;
        sell_order.order_type = OrderType::Limit;
        sell_order.price = 10.0 + i * 0.01;
        sell_order.quantity = 100.0 * (i + 1);
        orderbook.add_order(sell_order);
    }

    for (auto _ : state) {
        auto market_depth = orderbook.get_market_depth(10); // 获取10档行情
        benchmark::DoNotOptimize(market_depth);
    }

    state.counters["OrderBookLevels"] = num_levels * 2; // 买卖各一半
}
BENCHMARK(BM_OrderBook_DepthCalculation)->Range(10, 1000);