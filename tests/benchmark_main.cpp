#include <benchmark/benchmark.h>
#include "qaultra/qaultra.hpp"

using namespace qaultra;

// 账户性能基准测试
static void BM_AccountCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto account = std::make_shared<account::QA_Account>(
            "benchmark_account", "benchmark_portfolio", "benchmark_user",
            1000000.0, false, "backtest"
        );
        benchmark::DoNotOptimize(account);
    }
}
BENCHMARK(BM_AccountCreation);

static void BM_BuyOrders(benchmark::State& state) {
    auto account = std::make_shared<account::QA_Account>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        1000000.0, false, "backtest"
    );

    for (auto _ : state) {
        try {
            auto order = account->buy("TEST_STOCK", 100.0, "2024-01-15 09:30:00", 100.0);
            benchmark::DoNotOptimize(order);
        } catch (...) {
            // 忽略资金不足等错误
        }
    }
}
BENCHMARK(BM_BuyOrders);

static void BM_PriceUpdates(benchmark::State& state) {
    auto account = std::make_shared<account::QA_Account>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        1000000.0, false, "backtest"
    );

    // 先买入一些股票
    account->buy("TEST_STOCK", 1000.0, "2024-01-15 09:30:00", 100.0);

    double price = 100.0;
    for (auto _ : state) {
        price += 0.01; // 价格小幅变动
        account->on_price_change("TEST_STOCK", price, "2024-01-15 10:00:00");
        benchmark::DoNotOptimize(price);
    }
}
BENCHMARK(BM_PriceUpdates);

// SIMD性能基准测试
static void BM_SIMD_VectorAdd(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<double> a(size, 1.0);
    std::vector<double> b(size, 2.0);

    for (auto _ : state) {
        auto result = simd::vectorized_add(a.data(), b.data(), size);
        benchmark::DoNotOptimize(result.data());
    }
}
BENCHMARK(BM_SIMD_VectorAdd)->Range(1000, 1000000);

static void BM_SIMD_VectorMultiply(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<double> a(size, 1.5);
    std::vector<double> b(size, 2.5);

    for (auto _ : state) {
        auto result = simd::vectorized_multiply(a.data(), b.data(), size);
        benchmark::DoNotOptimize(result.data());
    }
}
BENCHMARK(BM_SIMD_VectorMultiply)->Range(1000, 1000000);

static void BM_SIMD_SMA_Calculation(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<double> prices(size);

    // 生成模拟价格数据
    for (size_t i = 0; i < size; ++i) {
        prices[i] = 100.0 + sin(i * 0.1) * 10.0;
    }

    for (auto _ : state) {
        auto result = simd::calculate_sma(prices.data(), size, 20);
        benchmark::DoNotOptimize(result.data());
    }
}
BENCHMARK(BM_SIMD_SMA_Calculation)->Range(1000, 100000);

// 标准库实现对比
static void BM_Standard_VectorAdd(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<double> a(size, 1.0);
    std::vector<double> b(size, 2.0);
    std::vector<double> result(size);

    for (auto _ : state) {
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
        benchmark::DoNotOptimize(result.data());
    }
}
BENCHMARK(BM_Standard_VectorAdd)->Range(1000, 1000000);

// Arrow数据结构性能测试
static void BM_ArrowKlineCollection_Creation(benchmark::State& state) {
    size_t size = state.range(0);

    std::vector<std::string> codes(size, "TEST_STOCK");
    std::vector<int64_t> timestamps(size);
    std::vector<double> opens(size), highs(size), lows(size), closes(size);
    std::vector<double> volumes(size), amounts(size);

    // 填充测试数据
    for (size_t i = 0; i < size; ++i) {
        timestamps[i] = 1704067200000LL + i * 60000LL; // 每分钟
        opens[i] = 100.0 + i * 0.01;
        highs[i] = opens[i] + 1.0;
        lows[i] = opens[i] - 1.0;
        closes[i] = opens[i] + 0.5;
        volumes[i] = 10000.0;
        amounts[i] = closes[i] * volumes[i];
    }

    for (auto _ : state) {
        auto collection = std::make_shared<arrow_data::ArrowKlineCollection>();
        collection->add_batch(codes, timestamps, opens, highs, lows, closes, volumes, amounts);
        benchmark::DoNotOptimize(collection);
    }
}
BENCHMARK(BM_ArrowKlineCollection_Creation)->Range(1000, 50000);

static void BM_ArrowKlineCollection_SMA(benchmark::State& state) {
    size_t size = state.range(0);

    // 创建K线数据
    auto collection = std::make_shared<arrow_data::ArrowKlineCollection>();
    std::vector<std::string> codes(size, "TEST_STOCK");
    std::vector<int64_t> timestamps(size);
    std::vector<double> opens(size), highs(size), lows(size), closes(size);
    std::vector<double> volumes(size), amounts(size);

    for (size_t i = 0; i < size; ++i) {
        timestamps[i] = 1704067200000LL + i * 60000LL;
        closes[i] = 100.0 + sin(i * 0.01) * 10.0; // 模拟价格波动
        opens[i] = closes[i] - 0.1;
        highs[i] = closes[i] + 0.5;
        lows[i] = closes[i] - 0.5;
        volumes[i] = 10000.0;
        amounts[i] = closes[i] * volumes[i];
    }

    collection->add_batch(codes, timestamps, opens, highs, lows, closes, volumes, amounts);

    for (auto _ : state) {
        auto sma_result = collection->sma(20);
        benchmark::DoNotOptimize(sma_result);
    }
}
BENCHMARK(BM_ArrowKlineCollection_SMA)->Range(1000, 50000);

// 撮合引擎性能测试
static void BM_MatchingEngine_OrderSubmission(benchmark::State& state) {
    auto engine = market::factory::create_matching_engine(4);
    engine->start();

    int order_count = 0;
    for (auto _ : state) {
        auto order = std::make_shared<account::Order>();
        order->order_id = "BENCH_ORDER_" + std::to_string(order_count++);
        order->code = "TEST_STOCK";
        order->volume = 100.0;
        order->price = 100.0 + (order_count % 10) * 0.1; // 价格变化
        order->direction = (order_count % 2 == 0) ? Direction::BUY : Direction::SELL;
        order->status = OrderStatus::PENDING;

        bool submitted = engine->submit_order(order);
        benchmark::DoNotOptimize(submitted);
    }

    engine->stop();
}
BENCHMARK(BM_MatchingEngine_OrderSubmission);

// 协议序列化性能测试
static void BM_QIFI_Serialization(benchmark::State& state) {
    protocol::QIFIAccount account;
    account.account_cookie = "test_account";
    account.portfolio_cookie = "test_portfolio";
    account.user_cookie = "test_user";
    account.init_cash = 1000000.0;
    account.cash = 500000.0;
    account.total_value = 1200000.0;

    // 添加一些持仓数据
    for (int i = 0; i < 10; ++i) {
        protocol::QIFIPosition pos;
        pos.code = "STOCK_" + std::to_string(i);
        pos.volume_long = 1000.0;
        pos.price = 100.0 + i;
        pos.market_value = pos.volume_long * pos.price;
        account.positions.push_back(pos);
    }

    for (auto _ : state) {
        auto json_data = account.to_json();
        benchmark::DoNotOptimize(json_data);
    }
}
BENCHMARK(BM_QIFI_Serialization);

static void BM_QIFI_Deserialization(benchmark::State& state) {
    // 创建测试JSON数据
    protocol::QIFIAccount test_account;
    test_account.account_cookie = "test_account";
    test_account.portfolio_cookie = "test_portfolio";
    test_account.user_cookie = "test_user";
    test_account.init_cash = 1000000.0;

    auto json_data = test_account.to_json();

    for (auto _ : state) {
        protocol::QIFIAccount account;
        account.from_json(json_data);
        benchmark::DoNotOptimize(account);
    }
}
BENCHMARK(BM_QIFI_Deserialization);

// 内存管理性能测试
static void BM_ObjectPool_Acquisition(benchmark::State& state) {
    auto pool = std::make_shared<memory::ObjectPool<account::Order>>(1000);

    for (auto _ : state) {
        auto order = pool->acquire();
        benchmark::DoNotOptimize(order);
        pool->release(order);
    }
}
BENCHMARK(BM_ObjectPool_Acquisition);

static void BM_LockFreeQueue_Operations(benchmark::State& state) {
    threading::LockFreeQueue<double> queue(10000);

    double value = 123.456;
    for (auto _ : state) {
        queue.enqueue(value);
        double dequeued;
        bool success = queue.dequeue(dequeued);
        benchmark::DoNotOptimize(success);
        benchmark::DoNotOptimize(dequeued);
    }
}
BENCHMARK(BM_LockFreeQueue_Operations);

// 多线程性能测试
static void BM_ThreadSafe_AccountOperations(benchmark::State& state) {
    auto account = std::make_shared<account::QA_Account>(
        "mt_account", "mt_portfolio", "mt_user",
        10000000.0, false, "backtest"
    );

    const int num_threads = state.range(0);

    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&account, t]() {
                std::string symbol = "STOCK_" + std::to_string(t % 5);
                try {
                    auto order = account->buy(symbol, 100.0, "2024-01-15 09:30:00", 100.0);
                    account->on_price_change(symbol, 101.0, "2024-01-15 10:00:00");
                } catch (...) {
                    // 忽略并发访问错误
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
    }
}
BENCHMARK(BM_ThreadSafe_AccountOperations)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// 性能分析工具基准测试
static void BM_Performance_Analysis(benchmark::State& state) {
    // 生成模拟收益率数据
    size_t size = state.range(0);
    std::vector<double> returns(size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(0.001, 0.02); // 日收益率分布

    for (size_t i = 0; i < size; ++i) {
        returns[i] = dist(gen);
    }

    analysis::PerformanceAnalyzer analyzer;

    for (auto _ : state) {
        auto metrics = analyzer.calculate_performance(returns);
        benchmark::DoNotOptimize(metrics);
    }
}
BENCHMARK(BM_Performance_Analysis)->Range(252, 2520); // 1年到10年的交易日

// 自定义计数器用于显示吞吐量
static void BM_OrderProcessing_Throughput(benchmark::State& state) {
    auto engine = market::factory::create_matching_engine(4);
    engine->start();

    int64_t orders_processed = 0;

    for (auto _ : state) {
        auto order = std::make_shared<account::Order>();
        order->order_id = "THROUGHPUT_" + std::to_string(orders_processed);
        order->code = "TEST";
        order->volume = 100.0;
        order->price = 100.0;
        order->direction = Direction::BUY;
        order->status = OrderStatus::PENDING;

        if (engine->submit_order(order)) {
            ++orders_processed;
        }
    }

    state.counters["OrdersPerSecond"] = benchmark::Counter(orders_processed,
                                                          benchmark::Counter::kIsRate);

    engine->stop();
}
BENCHMARK(BM_OrderProcessing_Throughput);

// 主函数
BENCHMARK_MAIN();