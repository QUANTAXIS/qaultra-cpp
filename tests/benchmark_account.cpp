/**
 * @file benchmark_account.cpp
 * @brief UnifiedAccount 性能基准测试
 *
 * 测试账户操作的性能，包括订单处理、持仓更新、QIFI转换等
 */

#include <benchmark/benchmark.h>
#include "qaultra/account/unified_account.hpp"
#include "qaultra/account/market_preset.hpp"
#include <random>
#include <thread>

using namespace qaultra::account;

// ===== UnifiedAccount 基础性能测试 =====

static void BM_UnifiedAccount_Creation(benchmark::State& state) {
    for (auto _ : state) {
        auto account = std::make_unique<UnifiedAccount>(
            "BENCH_ACCOUNT", "BENCH_PORTFOLIO", "BENCH_USER", 1000000.0, false
        );
        benchmark::DoNotOptimize(account);
    }
}
BENCHMARK(BM_UnifiedAccount_Creation);

static void BM_UnifiedAccount_StockBuyOrders(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "BENCH_ACCOUNT", "BENCH_PORTFOLIO", "BENCH_USER", 10000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    int order_count = 0;
    for (auto _ : state) {
        std::string order_id = account->buy("000001.SZ", 100.0, 10.0 + (order_count % 10) * 0.01);
        benchmark::DoNotOptimize(order_id);
        ++order_count;
    }
}
BENCHMARK(BM_UnifiedAccount_StockBuyOrders);

static void BM_UnifiedAccount_FutureBuyOpen(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "BENCH_ACCOUNT", "BENCH_PORTFOLIO", "BENCH_USER", 10000000.0, false
    );
    account->set_market_preset(MarketPreset::get_future_preset());

    int order_count = 0;
    for (auto _ : state) {
        std::string order_id = account->buy_open("IF2401", 1.0, 4000.0 + (order_count % 100));
        benchmark::DoNotOptimize(order_id);
        ++order_count;
    }
}
BENCHMARK(BM_UnifiedAccount_FutureBuyOpen);

// ===== 交易执行性能测试 =====

static void BM_UnifiedAccount_TradeExecution(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "BENCH_ACCOUNT", "BENCH_PORTFOLIO", "BENCH_USER", 10000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    // 预先创建一些订单
    std::vector<std::string> order_ids;
    for (int i = 0; i < 1000; ++i) {
        order_ids.push_back(account->buy("000001.SZ", 100.0, 10.0));
    }

    int trade_count = 0;
    for (auto _ : state) {
        std::string order_id = order_ids[trade_count % order_ids.size()];
        account->add_trade(order_id, 10.0, 100.0);
        benchmark::DoNotOptimize(order_id);
        ++trade_count;
    }
}
BENCHMARK(BM_UnifiedAccount_TradeExecution);

static void BM_UnifiedAccount_PositionUpdates(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "BENCH_ACCOUNT", "BENCH_PORTFOLIO", "BENCH_USER", 10000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    // 建立一些初始持仓
    std::vector<std::string> symbols = {"000001.SZ", "000002.SZ", "600000.SH", "600036.SH"};
    for (const auto& symbol : symbols) {
        std::string order_id = account->buy(symbol, 1000.0, 10.0);
        account->add_trade(order_id, 10.0, 1000.0);
    }

    double price = 10.0;
    for (auto _ : state) {
        price += 0.01;
        const std::string& symbol = symbols[state.iterations() % symbols.size()];
        account->update_position_price(symbol, price);
        benchmark::DoNotOptimize(price);
    }
}
BENCHMARK(BM_UnifiedAccount_PositionUpdates);

// ===== 大批量操作性能测试 =====

static void BM_UnifiedAccount_BatchOrderSubmission(benchmark::State& state) {
    const int batch_size = state.range(0);

    for (auto _ : state) {
        auto account = std::make_unique<UnifiedAccount>(
            "BATCH_ACCOUNT", "BATCH_PORTFOLIO", "BATCH_USER", 100000000.0, false
        );
        account->set_market_preset(MarketPreset::get_stock_preset());

        std::vector<std::string> order_ids;
        order_ids.reserve(batch_size);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < batch_size; ++i) {
            std::string symbol = "00000" + std::to_string(i % 10) + ".SZ";
            order_ids.push_back(account->buy(symbol, 100.0, 10.0 + i * 0.001));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["OrdersPerSecond"] = benchmark::Counter(
            batch_size * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(order_ids);
    }
}
BENCHMARK(BM_UnifiedAccount_BatchOrderSubmission)->Range(100, 10000);

static void BM_UnifiedAccount_BatchTradeProcessing(benchmark::State& state) {
    const int batch_size = state.range(0);

    auto account = std::make_unique<UnifiedAccount>(
        "BATCH_ACCOUNT", "BATCH_PORTFOLIO", "BATCH_USER", 100000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    // 预先创建订单
    std::vector<std::string> order_ids;
    for (int i = 0; i < batch_size; ++i) {
        order_ids.push_back(account->buy("000001.SZ", 100.0, 10.0));
    }

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < batch_size; ++i) {
            account->add_trade(order_ids[i], 10.0, 100.0);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["TradesPerSecond"] = benchmark::Counter(
            batch_size * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_UnifiedAccount_BatchTradeProcessing)->Range(100, 5000);

// ===== QIFI 协议性能测试 =====

static void BM_UnifiedAccount_QIFIConversion(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "QIFI_ACCOUNT", "QIFI_PORTFOLIO", "QIFI_USER", 1000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    // 建立复杂的账户状态
    std::vector<std::string> symbols = {"000001.SZ", "000002.SZ", "600000.SH", "600036.SH"};
    for (const auto& symbol : symbols) {
        std::string order_id = account->buy(symbol, 1000.0, 10.0 + symbols.size() * 0.1);
        account->add_trade(order_id, 10.0 + symbols.size() * 0.1, 1000.0);
    }

    for (auto _ : state) {
        auto qifi_data = account->to_qifi();
        benchmark::DoNotOptimize(qifi_data);
    }
}
BENCHMARK(BM_UnifiedAccount_QIFIConversion);

static void BM_UnifiedAccount_JSONSerialization(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "JSON_ACCOUNT", "JSON_PORTFOLIO", "JSON_USER", 1000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    // 建立复杂状态
    for (int i = 0; i < 10; ++i) {
        std::string symbol = "00000" + std::to_string(i) + ".SZ";
        std::string order_id = account->buy(symbol, 100.0 * (i + 1), 10.0 + i * 0.5);
        account->add_trade(order_id, 10.0 + i * 0.5, 100.0 * (i + 1));
    }

    for (auto _ : state) {
        auto json_data = account->to_json();
        benchmark::DoNotOptimize(json_data);
    }
}
BENCHMARK(BM_UnifiedAccount_JSONSerialization);

// ===== 内存使用性能测试 =====

static void BM_UnifiedAccount_MemoryFootprint(benchmark::State& state) {
    const int num_positions = state.range(0);

    for (auto _ : state) {
        auto account = std::make_unique<UnifiedAccount>(
            "MEM_ACCOUNT", "MEM_PORTFOLIO", "MEM_USER", 100000000.0, false
        );
        account->set_market_preset(MarketPreset::get_stock_preset());

        // 创建大量持仓
        for (int i = 0; i < num_positions; ++i) {
            std::string symbol = "STOCK_" + std::to_string(i);
            std::string order_id = account->buy(symbol, 100.0, 10.0);
            account->add_trade(order_id, 10.0, 100.0);
        }

        // 测量账户占用的内存
        benchmark::DoNotOptimize(account);

        state.counters["PositionsCount"] = num_positions;
        state.counters["EstimatedMemoryKB"] = benchmark::Counter(
            sizeof(*account) + num_positions * 500, // 粗略估计每个持仓500字节
            benchmark::Counter::kDefaults,
            benchmark::Counter::OneK::kIs1024
        );
    }
}
BENCHMARK(BM_UnifiedAccount_MemoryFootprint)->Range(10, 10000);

// ===== 多线程性能测试 =====

static void BM_UnifiedAccount_ThreadSafety(benchmark::State& state) {
    const int num_threads = state.range(0);

    auto account = std::make_unique<UnifiedAccount>(
        "MT_ACCOUNT", "MT_PORTFOLIO", "MT_USER", 100000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> operations_completed{0};

        auto start = std::chrono::high_resolution_clock::now();

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&account, &operations_completed, t]() {
                for (int i = 0; i < 100; ++i) {
                    std::string symbol = "THREAD_" + std::to_string(t) + "_" + std::to_string(i % 10);
                    std::string order_id = account->buy(symbol, 100.0, 10.0 + i * 0.001);
                    if (!order_id.empty()) {
                        account->add_trade(order_id, 10.0 + i * 0.001, 100.0);
                        operations_completed++;
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
        state.counters["OperationsPerSecond"] = benchmark::Counter(
            operations_completed.load() * 1000000.0 / elapsed.count(),
            benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_UnifiedAccount_ThreadSafety)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// ===== 现实交易场景模拟 =====

static void BM_UnifiedAccount_RealisticTradingDay(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "REALISTIC_ACCOUNT", "REALISTIC_PORTFOLIO", "REALISTIC_USER", 10000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    // 模拟一个交易日的操作
    std::vector<std::string> symbols = {
        "000001.SZ", "000002.SZ", "000858.SZ", "002415.SZ", "002594.SZ",
        "600000.SH", "600036.SH", "600519.SH", "600837.SH", "601318.SH"
    };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(8.0, 200.0);
    std::uniform_real_distribution<> volume_dist(100.0, 10000.0);
    std::uniform_int_distribution<> symbol_dist(0, symbols.size() - 1);

    for (auto _ : state) {
        // 模拟240分钟交易日，每分钟1-3次操作
        for (int minute = 0; minute < 240; ++minute) {
            int ops_this_minute = 1 + minute % 3;

            for (int op = 0; op < ops_this_minute; ++op) {
                const std::string& symbol = symbols[symbol_dist(gen)];
                double price = price_dist(gen);
                double volume = volume_dist(gen);

                // 70% 买入，30% 卖出
                if (gen() % 10 < 7) {
                    std::string order_id = account->buy(symbol, volume, price);
                    if (!order_id.empty() && gen() % 2 == 0) {
                        // 50% 概率立即成交
                        account->add_trade(order_id, price, volume);
                    }
                } else {
                    if (account->has_position(symbol)) {
                        std::string order_id = account->sell(symbol, volume, price);
                        if (!order_id.empty() && gen() % 2 == 0) {
                            account->add_trade(order_id, price, volume);
                        }
                    }
                }
            }
        }

        benchmark::DoNotOptimize(account->get_total_value());
    }
}
BENCHMARK(BM_UnifiedAccount_RealisticTradingDay);

// ===== 资产计算性能测试 =====

static void BM_UnifiedAccount_AssetCalculation(benchmark::State& state) {
    auto account = std::make_unique<UnifiedAccount>(
        "CALC_ACCOUNT", "CALC_PORTFOLIO", "CALC_USER", 10000000.0, false
    );
    account->set_market_preset(MarketPreset::get_stock_preset());

    // 建立大量持仓
    std::vector<std::string> symbols;
    for (int i = 0; i < 100; ++i) {
        std::string symbol = "STOCK_" + std::to_string(i);
        symbols.push_back(symbol);
        std::string order_id = account->buy(symbol, 100.0 * (i + 1), 10.0 + i * 0.1);
        account->add_trade(order_id, 10.0 + i * 0.1, 100.0 * (i + 1));
    }

    for (auto _ : state) {
        // 重复计算各种资产指标
        double total_value = account->get_total_value();
        double market_value = account->get_market_value();
        double available_cash = account->get_available_cash();
        double frozen_cash = account->get_frozen_cash();

        benchmark::DoNotOptimize(total_value);
        benchmark::DoNotOptimize(market_value);
        benchmark::DoNotOptimize(available_cash);
        benchmark::DoNotOptimize(frozen_cash);
    }
}
BENCHMARK(BM_UnifiedAccount_AssetCalculation);