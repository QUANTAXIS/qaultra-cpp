/**
 * @file benchmark_simd.cpp
 * @brief SIMD优化算法性能基准测试
 *
 * 测试SIMD优化的数学计算、技术指标和金融算法的性能提升
 */

#include <benchmark/benchmark.h>
#include "qaultra/util/simd_ops.hpp"
#include <vector>
#include <random>
#include <cmath>
#include <immintrin.h>

using namespace qaultra::util;

// ===== 基础SIMD算术运算测试 =====

static void BM_SIMD_VectorAdd_Double(benchmark::State& state) {
    const size_t size = state.range(0);
    std::vector<double> a(size, 1.5);
    std::vector<double> b(size, 2.3);
    std::vector<double> result(size);

    for (auto _ : state) {
        simd_vector_add(a.data(), b.data(), result.data(), size);
        benchmark::DoNotOptimize(result.data());
    }

    state.counters["ElementsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_VectorAdd_Double)->Range(1000, 1000000);

static void BM_Standard_VectorAdd_Double(benchmark::State& state) {
    const size_t size = state.range(0);
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
BENCHMARK(BM_Standard_VectorAdd_Double)->Range(1000, 1000000);

static void BM_SIMD_VectorMultiply_Double(benchmark::State& state) {
    const size_t size = state.range(0);
    std::vector<double> a(size, 1.5);
    std::vector<double> b(size, 2.3);
    std::vector<double> result(size);

    for (auto _ : state) {
        simd_vector_multiply(a.data(), b.data(), result.data(), size);
        benchmark::DoNotOptimize(result.data());
    }

    state.counters["ElementsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_VectorMultiply_Double)->Range(1000, 1000000);

static void BM_Standard_VectorMultiply_Double(benchmark::State& state) {
    const size_t size = state.range(0);
    std::vector<double> a(size, 1.5);
    std::vector<double> b(size, 2.3);
    std::vector<double> result(size);

    for (auto _ : state) {
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
        benchmark::DoNotOptimize(result.data());
    }

    state.counters["ElementsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_Standard_VectorMultiply_Double)->Range(1000, 1000000);

// ===== 金融计算SIMD优化测试 =====

static void BM_SIMD_SMA_Calculation(benchmark::State& state) {
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
        auto sma_result = simd_calculate_sma(prices.data(), size, period);
        benchmark::DoNotOptimize(sma_result.data());
    }

    state.counters["CalculationsPerSecond"] = benchmark::Counter(
        size - period + 1, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_SMA_Calculation)->Range(1000, 100000);

static void BM_Standard_SMA_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const int period = 20;

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
BENCHMARK(BM_Standard_SMA_Calculation)->Range(1000, 100000);

static void BM_SIMD_EMA_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const int period = 20;

    std::vector<double> prices(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_dist(100.0, 10.0);

    for (size_t i = 0; i < size; ++i) {
        prices[i] = price_dist(gen);
    }

    for (auto _ : state) {
        auto ema_result = simd_calculate_ema(prices.data(), size, period);
        benchmark::DoNotOptimize(ema_result.data());
    }

    state.counters["CalculationsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_EMA_Calculation)->Range(1000, 100000);

static void BM_Standard_EMA_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const int period = 20;

    std::vector<double> prices(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_dist(100.0, 10.0);

    for (size_t i = 0; i < size; ++i) {
        prices[i] = price_dist(gen);
    }

    for (auto _ : state) {
        std::vector<double> ema_result(size);
        double alpha = 2.0 / (period + 1);

        ema_result[0] = prices[0];
        for (size_t i = 1; i < size; ++i) {
            ema_result[i] = alpha * prices[i] + (1 - alpha) * ema_result[i-1];
        }
        benchmark::DoNotOptimize(ema_result.data());
    }

    state.counters["CalculationsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_Standard_EMA_Calculation)->Range(1000, 100000);

// ===== 波动率计算SIMD优化测试 =====

static void BM_SIMD_Volatility_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const int window = 20;

    std::vector<double> returns(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> return_dist(0.001, 0.02);

    for (size_t i = 0; i < size; ++i) {
        returns[i] = return_dist(gen);
    }

    for (auto _ : state) {
        auto volatility = simd_calculate_rolling_volatility(returns.data(), size, window);
        benchmark::DoNotOptimize(volatility.data());
    }

    state.counters["VolatilityCalculationsPerSecond"] = benchmark::Counter(
        size - window + 1, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_Volatility_Calculation)->Range(1000, 50000);

static void BM_Standard_Volatility_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const int window = 20;

    std::vector<double> returns(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> return_dist(0.001, 0.02);

    for (size_t i = 0; i < size; ++i) {
        returns[i] = return_dist(gen);
    }

    for (auto _ : state) {
        std::vector<double> volatility;
        volatility.reserve(size - window + 1);

        for (size_t i = window - 1; i < size; ++i) {
            // 计算均值
            double mean = 0.0;
            for (int j = 0; j < window; ++j) {
                mean += returns[i - j];
            }
            mean /= window;

            // 计算方差
            double variance = 0.0;
            for (int j = 0; j < window; ++j) {
                double diff = returns[i - j] - mean;
                variance += diff * diff;
            }
            variance /= (window - 1);

            volatility.push_back(std::sqrt(variance));
        }
        benchmark::DoNotOptimize(volatility.data());
    }

    state.counters["VolatilityCalculationsPerSecond"] = benchmark::Counter(
        size - window + 1, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_Standard_Volatility_Calculation)->Range(1000, 50000);

// ===== 相关性计算SIMD优化测试 =====

static void BM_SIMD_Correlation_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);

    std::vector<double> x(size);
    std::vector<double> y(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(0.0, 1.0);

    for (size_t i = 0; i < size; ++i) {
        x[i] = dist(gen);
        y[i] = 0.5 * x[i] + 0.5 * dist(gen); // 部分相关
    }

    for (auto _ : state) {
        double correlation = simd_calculate_correlation(x.data(), y.data(), size);
        benchmark::DoNotOptimize(correlation);
    }

    state.counters["ElementsProcessed"] = size;
}
BENCHMARK(BM_SIMD_Correlation_Calculation)->Range(1000, 100000);

static void BM_Standard_Correlation_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);

    std::vector<double> x(size);
    std::vector<double> y(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(0.0, 1.0);

    for (size_t i = 0; i < size; ++i) {
        x[i] = dist(gen);
        y[i] = 0.5 * x[i] + 0.5 * dist(gen);
    }

    for (auto _ : state) {
        // 计算均值
        double mean_x = 0.0, mean_y = 0.0;
        for (size_t i = 0; i < size; ++i) {
            mean_x += x[i];
            mean_y += y[i];
        }
        mean_x /= size;
        mean_y /= size;

        // 计算协方差和方差
        double covariance = 0.0, variance_x = 0.0, variance_y = 0.0;
        for (size_t i = 0; i < size; ++i) {
            double dx = x[i] - mean_x;
            double dy = y[i] - mean_y;
            covariance += dx * dy;
            variance_x += dx * dx;
            variance_y += dy * dy;
        }

        double correlation = covariance / std::sqrt(variance_x * variance_y);
        benchmark::DoNotOptimize(correlation);
    }

    state.counters["ElementsProcessed"] = size;
}
BENCHMARK(BM_Standard_Correlation_Calculation)->Range(1000, 100000);

// ===== SIMD优化的风险计算测试 =====

static void BM_SIMD_VaR_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const double confidence_level = 0.95;

    std::vector<double> returns(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> return_dist(0.001, 0.02);

    for (size_t i = 0; i < size; ++i) {
        returns[i] = return_dist(gen);
    }

    for (auto _ : state) {
        double var = simd_calculate_var(returns.data(), size, confidence_level);
        benchmark::DoNotOptimize(var);
    }

    state.counters["ReturnsProcessed"] = size;
}
BENCHMARK(BM_SIMD_VaR_Calculation)->Range(1000, 100000);

static void BM_SIMD_CVaR_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const double confidence_level = 0.95;

    std::vector<double> returns(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> return_dist(0.001, 0.02);

    for (size_t i = 0; i < size; ++i) {
        returns[i] = return_dist(gen);
    }

    for (auto _ : state) {
        double cvar = simd_calculate_cvar(returns.data(), size, confidence_level);
        benchmark::DoNotOptimize(cvar);
    }

    state.counters["ReturnsProcessed"] = size;
}
BENCHMARK(BM_SIMD_CVaR_Calculation)->Range(1000, 100000);

// ===== 矩阵运算SIMD优化测试 =====

static void BM_SIMD_Matrix_Multiply(benchmark::State& state) {
    const size_t size = state.range(0);

    std::vector<double> matrix_a(size * size);
    std::vector<double> matrix_b(size * size);
    std::vector<double> result(size * size);

    // 初始化矩阵
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1.0, 1.0);

    for (size_t i = 0; i < size * size; ++i) {
        matrix_a[i] = dist(gen);
        matrix_b[i] = dist(gen);
    }

    for (auto _ : state) {
        simd_matrix_multiply(matrix_a.data(), matrix_b.data(), result.data(), size, size, size);
        benchmark::DoNotOptimize(result.data());
    }

    state.counters["MatrixSize"] = size;
    state.counters["Operations"] = size * size * size; // 总操作数
}
BENCHMARK(BM_SIMD_Matrix_Multiply)->Range(32, 512);

static void BM_Standard_Matrix_Multiply(benchmark::State& state) {
    const size_t size = state.range(0);

    std::vector<double> matrix_a(size * size);
    std::vector<double> matrix_b(size * size);
    std::vector<double> result(size * size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1.0, 1.0);

    for (size_t i = 0; i < size * size; ++i) {
        matrix_a[i] = dist(gen);
        matrix_b[i] = dist(gen);
    }

    for (auto _ : state) {
        // 标准矩阵乘法
        for (size_t i = 0; i < size; ++i) {
            for (size_t j = 0; j < size; ++j) {
                double sum = 0.0;
                for (size_t k = 0; k < size; ++k) {
                    sum += matrix_a[i * size + k] * matrix_b[k * size + j];
                }
                result[i * size + j] = sum;
            }
        }
        benchmark::DoNotOptimize(result.data());
    }

    state.counters["MatrixSize"] = size;
    state.counters["Operations"] = size * size * size;
}
BENCHMARK(BM_Standard_Matrix_Multiply)->Range(32, 512);

// ===== 复杂金融指标SIMD优化测试 =====

static void BM_SIMD_RSI_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    const int period = 14;

    std::vector<double> prices(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_dist(100.0, 10.0);

    for (size_t i = 0; i < size; ++i) {
        prices[i] = price_dist(gen);
    }

    for (auto _ : state) {
        auto rsi_result = simd_calculate_rsi(prices.data(), size, period);
        benchmark::DoNotOptimize(rsi_result.data());
    }

    state.counters["RSICalculationsPerSecond"] = benchmark::Counter(
        size - period, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_RSI_Calculation)->Range(1000, 50000);

static void BM_SIMD_MACD_Calculation(benchmark::State& state) {
    const size_t size = state.range(0);

    std::vector<double> prices(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_dist(100.0, 10.0);

    for (size_t i = 0; i < size; ++i) {
        prices[i] = price_dist(gen);
    }

    for (auto _ : state) {
        auto macd_result = simd_calculate_macd(prices.data(), size, 12, 26, 9);
        benchmark::DoNotOptimize(macd_result.macd.data());
        benchmark::DoNotOptimize(macd_result.signal.data());
        benchmark::DoNotOptimize(macd_result.histogram.data());
    }

    state.counters["MACDCalculationsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_MACD_Calculation)->Range(1000, 50000);

// ===== 内存访问模式优化测试 =====

static void BM_SIMD_MemoryAlignment_Test(benchmark::State& state) {
    const size_t size = state.range(0);

    // 对齐内存分配
    alignas(32) std::vector<double> aligned_data(size, 1.5);
    std::vector<double> unaligned_data(size, 2.3);
    alignas(32) std::vector<double> result(size);

    for (auto _ : state) {
        simd_vector_add_aligned(aligned_data.data(), unaligned_data.data(), result.data(), size);
        benchmark::DoNotOptimize(result.data());
    }

    state.counters["ElementsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_MemoryAlignment_Test)->Range(1000, 1000000);

static void BM_SIMD_Cache_Friendly_Access(benchmark::State& state) {
    const size_t size = state.range(0);
    const size_t block_size = 64; // Cache line size

    std::vector<double> data(size);
    std::iota(data.begin(), data.end(), 1.0);

    for (auto _ : state) {
        double sum = 0.0;

        // 缓存友好的访问模式
        for (size_t block = 0; block < size; block += block_size) {
            size_t end = std::min(block + block_size, size);
            sum += simd_block_sum(data.data() + block, end - block);
        }

        benchmark::DoNotOptimize(sum);
    }

    state.counters["ElementsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_SIMD_Cache_Friendly_Access)->Range(1000, 1000000);

// ===== SIMD 指令集特定优化测试 =====

#ifdef __AVX2__
static void BM_AVX2_VectorSum(benchmark::State& state) {
    const size_t size = state.range(0);
    alignas(32) std::vector<double> data(size, 1.5);

    for (auto _ : state) {
        double sum = 0.0;
        const double* ptr = data.data();

        __m256d sum_vec = _mm256_setzero_pd();

        for (size_t i = 0; i < size; i += 4) {
            __m256d vec = _mm256_load_pd(ptr + i);
            sum_vec = _mm256_add_pd(sum_vec, vec);
        }

        // 水平相加
        __m128d sum_high = _mm256_extractf128_pd(sum_vec, 1);
        __m128d sum_low = _mm256_extractf128_pd(sum_vec, 0);
        __m128d sum_combined = _mm_add_pd(sum_high, sum_low);
        __m128d sum_shuffled = _mm_shuffle_pd(sum_combined, sum_combined, 1);
        __m128d final_sum = _mm_add_pd(sum_combined, sum_shuffled);

        _mm_store_sd(&sum, final_sum);
        benchmark::DoNotOptimize(sum);
    }

    state.counters["ElementsPerSecond"] = benchmark::Counter(
        size, benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_AVX2_VectorSum)->Range(1000, 1000000);
#endif

// ===== 比较不同SIMD指令集性能 =====

static void BM_Compare_SIMD_Instruction_Sets(benchmark::State& state) {
    const size_t size = 10000;
    std::vector<double> a(size, 1.5);
    std::vector<double> b(size, 2.3);
    std::vector<double> result(size);

    for (auto _ : state) {
        // 根据可用指令集选择最优实现
        #ifdef __AVX2__
            simd_vector_add_avx2(a.data(), b.data(), result.data(), size);
        #elif defined(__SSE2__)
            simd_vector_add_sse2(a.data(), b.data(), result.data(), size);
        #else
            simd_vector_add_scalar(a.data(), b.data(), result.data(), size);
        #endif

        benchmark::DoNotOptimize(result.data());
    }

    #ifdef __AVX2__
        state.SetLabel("AVX2");
    #elif defined(__SSE2__)
        state.SetLabel("SSE2");
    #else
        state.SetLabel("Scalar");
    #endif
}
BENCHMARK(BM_Compare_SIMD_Instruction_Sets);