/**
 * @file benchmark_main.cpp
 * @brief 主要基准测试入口文件
 *
 * 包含核心的统一系统性能基准测试
 */

#include <benchmark/benchmark.h>
#include "qaultra/account/unified_account.hpp"
#include "qaultra/account/market_preset.hpp"
#include "qaultra/data/unified_datatype.hpp"
#include <random>
#include <vector>

using namespace qaultra;

// ===== 统一账户性能基准测试 =====

static void BM_UnifiedAccountCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto account = std::make_unique<account::UnifiedAccount>(
            "benchmark_account", "benchmark_portfolio", "benchmark_user",
            1000000.0, false
        );
        benchmark::DoNotOptimize(account);
    }
}
BENCHMARK(BM_UnifiedAccountCreation);

static void BM_UnifiedBuyOrders(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        10000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_stock_preset());

    int order_count = 0;
    for (auto _ : state) {
        std::string order_id = account->buy("000001.SZ", 100.0, 10.0 + (order_count % 10) * 0.01);
        benchmark::DoNotOptimize(order_id);
        ++order_count;
    }
}
BENCHMARK(BM_UnifiedBuyOrders);

static void BM_UnifiedSellOrders(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        10000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_stock_preset());

    // 先买入一些股票建立持仓
    for (int i = 0; i < 100; ++i) {
        std::string buy_order_id = account->buy("000001.SZ", 100.0, 10.0);
        account->add_trade(buy_order_id, 10.0, 100.0);
    }

    int order_count = 0;
    for (auto _ : state) {
        std::string order_id = account->sell("000001.SZ", 50.0, 10.0 + (order_count % 10) * 0.01);
        benchmark::DoNotOptimize(order_id);
        ++order_count;
    }
}
BENCHMARK(BM_UnifiedSellOrders);

static void BM_UnifiedTradeExecution(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        10000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_stock_preset());

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
BENCHMARK(BM_UnifiedTradeExecution);

// ===== 期货交易性能测试 =====

static void BM_FutureBuyOpen(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        10000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_future_preset());

    int order_count = 0;
    for (auto _ : state) {
        std::string order_id = account->buy_open("IF2401", 1.0, 4000.0 + (order_count % 100));
        benchmark::DoNotOptimize(order_id);
        ++order_count;
    }
}
BENCHMARK(BM_FutureBuyOpen);

static void BM_FutureSellClose(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        10000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_future_preset());

    // 先开仓建立持仓
    for (int i = 0; i < 100; ++i) {
        std::string buy_order_id = account->buy_open("IF2401", 1.0, 4000.0);
        account->add_trade(buy_order_id, 4000.0, 1.0);
    }

    int order_count = 0;
    for (auto _ : state) {
        std::string order_id = account->sell_close("IF2401", 1.0, 4000.0 + (order_count % 100));
        benchmark::DoNotOptimize(order_id);
        ++order_count;
    }
}
BENCHMARK(BM_FutureSellClose);

// ===== 账户状态计算性能测试 =====

static void BM_AccountValueCalculation(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        10000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_stock_preset());

    // 建立复杂的持仓状态
    std::vector<std::string> symbols = {"000001.SZ", "000002.SZ", "600000.SH", "600036.SH"};
    for (const auto& symbol : symbols) {
        std::string order_id = account->buy(symbol, 1000.0, 10.0 + symbols.size() * 0.1);
        account->add_trade(order_id, 10.0 + symbols.size() * 0.1, 1000.0);
    }

    for (auto _ : state) {
        double total_value = account->get_total_value();
        double market_value = account->get_market_value();
        double available_cash = account->get_available_cash();

        benchmark::DoNotOptimize(total_value);
        benchmark::DoNotOptimize(market_value);
        benchmark::DoNotOptimize(available_cash);
    }
}
BENCHMARK(BM_AccountValueCalculation);

// ===== QIFI协议转换性能测试 =====

static void BM_QIFIConversion(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        1000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_stock_preset());

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
BENCHMARK(BM_QIFIConversion);

static void BM_JSONSerialization(benchmark::State& state) {
    auto account = std::make_unique<account::UnifiedAccount>(
        "benchmark_account", "benchmark_portfolio", "benchmark_user",
        1000000.0, false
    );
    account->set_market_preset(account::MarketPreset::get_stock_preset());

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
BENCHMARK(BM_JSONSerialization);

// ===== 市场预设性能测试 =====

static void BM_MarketPresetApplication(benchmark::State& state) {
    for (auto _ : state) {
        auto account = std::make_unique<account::UnifiedAccount>(
            "benchmark_account", "benchmark_portfolio", "benchmark_user",
            1000000.0, false
        );

        // 应用不同的市场预设
        account->set_market_preset(account::MarketPreset::get_stock_preset());
        account->set_market_preset(account::MarketPreset::get_future_preset());
        account->set_market_preset(account::MarketPreset::get_forex_preset());

        benchmark::DoNotOptimize(account);
    }
}
BENCHMARK(BM_MarketPresetApplication);

// ===== 大批量操作性能测试 =====

static void BM_BatchOrderSubmission(benchmark::State& state) {
    const int batch_size = state.range(0);

    for (auto _ : state) {
        auto account = std::make_unique<account::UnifiedAccount>(
            "batch_account", "batch_portfolio", "batch_user",
            100000000.0, false
        );
        account->set_market_preset(account::MarketPreset::get_stock_preset());

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
BENCHMARK(BM_BatchOrderSubmission)->Range(100, 10000);

// ===== 简化的数学运算性能测试 =====

static void BM_VectorOperation(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<double> a(size, 1.5);
    std::vector<double> b(size, 2.3);
    std::vector<double> result(size);

    for (auto _ : state) {
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
        benchmark::DoNotOptimize(result.data());
    }

    state.counters["ElementsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_VectorOperation)->Range(1000, 1000000);

static void BM_SMACalculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const int period = 20;

    // 生成模拟价格数据
    std::vector<double> prices(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_dist(100.0, 10.0);

    for (size_t i = 0; i < size; ++i) {
        prices[i] = price_dist(gen);
    }

    for (auto _ : state) {
        std::vector<double> sma_result;
        sma_result.reserve(size - period + 1);

        // 标准SMA计算
        for (size_t i = period - 1; i < size; ++i) {
            double sum = 0.0;
            for (int j = 0; j < period; ++j) {
                sum += prices[i - j];
            }
            sma_result.push_back(sum / period);
        }
        benchmark::DoNotOptimize(sma_result.data());
    }

    state.counters["CalculationsPerSecond"] = benchmark::Counter(
        size - period + 1, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SMACalculation)->Range(1000, 100000);

// ===== 内存使用性能测试 =====

static void BM_MemoryFootprint(benchmark::State& state) {
    const int num_positions = state.range(0);

    for (auto _ : state) {
        auto account = std::make_unique<account::UnifiedAccount>(
            "mem_account", "mem_portfolio", "mem_user", 100000000.0, false
        );
        account->set_market_preset(account::MarketPreset::get_stock_preset());

        // 创建大量持仓
        for (int i = 0; i < num_positions; ++i) {
            std::string symbol = "STOCK_" + std::to_string(i);
            std::string order_id = account->buy(symbol, 100.0, 10.0);
            account->add_trade(order_id, 10.0, 100.0);
        }

        benchmark::DoNotOptimize(account);

        state.counters["PositionsCount"] = num_positions;
        state.counters["EstimatedMemoryKB"] = benchmark::Counter(
            sizeof(*account) + num_positions * 500, // 粗略估计每个持仓500字节
            benchmark::Counter::kDefaults,
            benchmark::Counter::OneK::kIs1024
        );
    }
}
BENCHMARK(BM_MemoryFootprint)->Range(10, 10000);

// 主函数
BENCHMARK_MAIN();