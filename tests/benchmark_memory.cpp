/**
 * @file benchmark_memory.cpp
 * @brief 内存管理和数据结构性能基准测试
 *
 * 测试内存分配、对象池、无锁数据结构等的性能
 */

#include <benchmark/benchmark.h>
#include "qaultra/util/object_pool.hpp"
#include "qaultra/util/lockfree_queue.hpp"
#include "qaultra/account/position.hpp"
#include "qaultra/account/order.hpp"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

using namespace qaultra::util;
using namespace qaultra::account;

// ===== 内存分配性能测试 =====

static void BM_StandardAllocation_Position(benchmark::State& state) {
    const int num_allocations = state.range(0);

    for (auto _ : state) {
        std::vector<std::unique_ptr<Position>> positions;
        positions.reserve(num_allocations);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_allocations; ++i) {
            auto pos = std::make_unique<Position>("STOCK_" + std::to_string(i));
            positions.push_back(std::move(pos));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["AllocationsPerSecond"] = benchmark::Counter(
            num_allocations * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(positions);
    }
}
BENCHMARK(BM_StandardAllocation_Position)->Range(100, 10000);

static void BM_ObjectPool_Position(benchmark::State& state) {
    const int num_allocations = state.range(0);
    ObjectPool<Position> pool(num_allocations);

    for (auto _ : state) {
        std::vector<Position*> positions;
        positions.reserve(num_allocations);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_allocations; ++i) {
            Position* pos = pool.acquire();
            if (pos) {
                pos->set_instrument_id("STOCK_" + std::to_string(i));
                positions.push_back(pos);
            }
        }

        // 归还对象到池中
        for (auto* pos : positions) {
            pool.release(pos);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["AllocationsPerSecond"] = benchmark::Counter(
            num_allocations * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(positions);
    }
}
BENCHMARK(BM_ObjectPool_Position)->Range(100, 10000);

// ===== 对象池vs标准分配对比测试 =====

static void BM_AllocationComparison_Order(benchmark::State& state) {
    const int operations = 1000;
    const bool use_pool = state.range(0); // 0: 标准分配, 1: 对象池

    if (use_pool) {
        ObjectPool<Order> pool(operations);

        for (auto _ : state) {
            std::vector<Order*> orders;
            orders.reserve(operations);

            for (int i = 0; i < operations; ++i) {
                Order* order = pool.acquire();
                if (order) {
                    order->order_id = "ORDER_" + std::to_string(i);
                    order->instrument_id = "000001.SZ";
                    order->volume_orign = 100.0;
                    order->price_order = 10.0;
                    orders.push_back(order);
                }
            }

            for (auto* order : orders) {
                pool.release(order);
            }

            benchmark::DoNotOptimize(orders);
        }
        state.SetLabel("ObjectPool");
    } else {
        for (auto _ : state) {
            std::vector<std::unique_ptr<Order>> orders;
            orders.reserve(operations);

            for (int i = 0; i < operations; ++i) {
                auto order = std::make_unique<Order>();
                order->order_id = "ORDER_" + std::to_string(i);
                order->instrument_id = "000001.SZ";
                order->volume_orign = 100.0;
                order->price_order = 10.0;
                orders.push_back(std::move(order));
            }

            benchmark::DoNotOptimize(orders);
        }
        state.SetLabel("StandardAllocation");
    }
}
BENCHMARK(BM_AllocationComparison_Order)->Arg(0)->Arg(1);

// ===== 无锁队列性能测试 =====

static void BM_LockFreeQueue_SingleProducerSingleConsumer(benchmark::State& state) {
    const int queue_size = 10000;
    const int operations = state.range(0);

    LockFreeQueue<int> queue(queue_size);

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        // 生产者
        for (int i = 0; i < operations; ++i) {
            while (!queue.enqueue(i)) {
                // 队列满，等待
                std::this_thread::yield();
            }
        }

        // 消费者
        for (int i = 0; i < operations; ++i) {
            int value;
            while (!queue.dequeue(value)) {
                // 队列空，等待
                std::this_thread::yield();
            }
            benchmark::DoNotOptimize(value);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["OperationsPerSecond"] = benchmark::Counter(
            operations * 2 * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_LockFreeQueue_SingleProducerSingleConsumer)->Range(1000, 100000);

static void BM_StandardQueue_SingleProducerSingleConsumer(benchmark::State& state) {
    const int operations = state.range(0);

    std::queue<int> queue;
    std::mutex queue_mutex;

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        // 生产者
        for (int i = 0; i < operations; ++i) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            queue.push(i);
        }

        // 消费者
        for (int i = 0; i < operations; ++i) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (!queue.empty()) {
                int value = queue.front();
                queue.pop();
                benchmark::DoNotOptimize(value);
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["OperationsPerSecond"] = benchmark::Counter(
            operations * 2 * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_StandardQueue_SingleProducerSingleConsumer)->Range(1000, 100000);

// ===== 多线程无锁队列测试 =====

static void BM_LockFreeQueue_MultiProducerMultiConsumer(benchmark::State& state) {
    const int queue_size = 100000;
    const int operations_per_thread = 10000;
    const int num_producers = state.range(0);
    const int num_consumers = state.range(0);

    for (auto _ : state) {
        LockFreeQueue<int> queue(queue_size);
        std::atomic<int> total_produced{0};
        std::atomic<int> total_consumed{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;

        // 启动生产者线程
        for (int p = 0; p < num_producers; ++p) {
            producers.emplace_back([&queue, &total_produced, operations_per_thread, p]() {
                for (int i = 0; i < operations_per_thread; ++i) {
                    int value = p * operations_per_thread + i;
                    while (!queue.enqueue(value)) {
                        std::this_thread::yield();
                    }
                    total_produced++;
                }
            });
        }

        // 启动消费者线程
        for (int c = 0; c < num_consumers; ++c) {
            consumers.emplace_back([&queue, &total_consumed, operations_per_thread]() {
                for (int i = 0; i < operations_per_thread; ++i) {
                    int value;
                    while (!queue.dequeue(value)) {
                        std::this_thread::yield();
                    }
                    total_consumed++;
                    benchmark::DoNotOptimize(value);
                }
            });
        }

        // 等待所有线程完成
        for (auto& producer : producers) {
            producer.join();
        }
        for (auto& consumer : consumers) {
            consumer.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["ProducerThreads"] = num_producers;
        state.counters["ConsumerThreads"] = num_consumers;
        state.counters["TotalOperationsPerSecond"] = benchmark::Counter(
            (total_produced + total_consumed) * 1000000.0 / elapsed.count(),
            benchmark::Counter::kIsRate
        );
    }
}
BENCHMARK(BM_LockFreeQueue_MultiProducerMultiConsumer)->Range(1, 8);

// ===== 内存访问模式性能测试 =====

static void BM_MemoryAccess_Sequential(benchmark::State& state) {
    const size_t size = state.range(0);
    std::vector<double> data(size);

    // 初始化数据
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<double>(i);
    }

    for (auto _ : state) {
        double sum = 0.0;

        auto start = std::chrono::high_resolution_clock::now();

        // 顺序访问
        for (size_t i = 0; i < size; ++i) {
            sum += data[i];
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["ElementsPerSecond"] = benchmark::Counter(
            size * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(sum);
    }
    state.SetLabel("Sequential");
}
BENCHMARK(BM_MemoryAccess_Sequential)->Range(10000, 10000000);

static void BM_MemoryAccess_Random(benchmark::State& state) {
    const size_t size = state.range(0);
    std::vector<double> data(size);
    std::vector<size_t> indices(size);

    // 初始化数据和随机索引
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<double>(i);
        indices[i] = i;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);

    for (auto _ : state) {
        double sum = 0.0;

        auto start = std::chrono::high_resolution_clock::now();

        // 随机访问
        for (size_t i = 0; i < size; ++i) {
            sum += data[indices[i]];
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["ElementsPerSecond"] = benchmark::Counter(
            size * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(sum);
    }
    state.SetLabel("Random");
}
BENCHMARK(BM_MemoryAccess_Random)->Range(10000, 10000000);

// ===== 缓存友好数据结构测试 =====

struct CacheFriendlyPosition {
    char instrument_id[16];      // 固定大小，避免指针
    double volume_long;
    double volume_short;
    double price_avg;
    double market_value;
    int64_t last_update_time;
    char padding[8];             // 对齐到64字节缓存行
};

static void BM_CacheFriendly_PositionAccess(benchmark::State& state) {
    const size_t num_positions = state.range(0);
    std::vector<CacheFriendlyPosition> positions(num_positions);

    // 初始化数据
    for (size_t i = 0; i < num_positions; ++i) {
        snprintf(positions[i].instrument_id, 16, "STOCK_%06zu", i);
        positions[i].volume_long = 100.0 * (i + 1);
        positions[i].volume_short = 0.0;
        positions[i].price_avg = 10.0 + i * 0.01;
        positions[i].market_value = positions[i].volume_long * positions[i].price_avg;
        positions[i].last_update_time = i;
    }

    for (auto _ : state) {
        double total_value = 0.0;

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < num_positions; ++i) {
            total_value += positions[i].market_value;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["PositionsPerSecond"] = benchmark::Counter(
            num_positions * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(total_value);
    }
}
BENCHMARK(BM_CacheFriendly_PositionAccess)->Range(1000, 100000);

static void BM_Standard_PositionAccess(benchmark::State& state) {
    const size_t num_positions = state.range(0);
    std::vector<std::unique_ptr<Position>> positions;

    // 初始化数据
    for (size_t i = 0; i < num_positions; ++i) {
        auto pos = std::make_unique<Position>("STOCK_" + std::to_string(i));
        pos->volume_long_today = 100.0 * (i + 1);
        pos->volume_short_today = 0.0;
        pos->price_settlement_today = 10.0 + i * 0.01;
        positions.push_back(std::move(pos));
    }

    for (auto _ : state) {
        double total_value = 0.0;

        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& pos : positions) {
            total_value += pos->calculate_market_value(pos->price_settlement_today, 1.0);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["PositionsPerSecond"] = benchmark::Counter(
            num_positions * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(total_value);
    }
}
BENCHMARK(BM_Standard_PositionAccess)->Range(1000, 100000);

// ===== 内存池分配器测试 =====

template<typename T>
class PoolAllocator {
public:
    using value_type = T;

    PoolAllocator(ObjectPool<T>* pool) : pool_(pool) {}

    template<typename U>
    PoolAllocator(const PoolAllocator<U>& other) : pool_(other.pool_) {}

    T* allocate(std::size_t n) {
        if (n == 1 && pool_) {
            return pool_->acquire();
        }
        return static_cast<T*>(std::malloc(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) {
        if (n == 1 && pool_) {
            pool_->release(p);
        } else {
            std::free(p);
        }
    }

private:
    ObjectPool<T>* pool_;
};

static void BM_PoolAllocator_Vector(benchmark::State& state) {
    const int num_elements = state.range(0);
    ObjectPool<int> pool(num_elements);
    PoolAllocator<int> allocator(&pool);

    for (auto _ : state) {
        std::vector<int, PoolAllocator<int>> vec(allocator);
        vec.reserve(num_elements);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_elements; ++i) {
            vec.push_back(i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["ElementsPerSecond"] = benchmark::Counter(
            num_elements * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_PoolAllocator_Vector)->Range(1000, 100000);

// ===== 内存压缩和打包测试 =====

struct PackedOrder {
    uint32_t order_id_hash;      // 哈希而不是字符串
    uint16_t instrument_id_hash; // 哈希而不是字符串
    uint8_t direction;           // 0=BUY, 1=SELL
    uint8_t status;              // 状态枚举
    float price;                 // 使用float而不是double
    float volume;
    uint32_t timestamp;          // Unix时间戳
} __attribute__((packed));

static void BM_PackedOrder_Processing(benchmark::State& state) {
    const size_t num_orders = state.range(0);
    std::vector<PackedOrder> orders(num_orders);

    // 初始化数据
    for (size_t i = 0; i < num_orders; ++i) {
        orders[i].order_id_hash = static_cast<uint32_t>(i);
        orders[i].instrument_id_hash = static_cast<uint16_t>(i % 1000);
        orders[i].direction = i % 2;
        orders[i].status = 0; // PENDING
        orders[i].price = 10.0f + i * 0.01f;
        orders[i].volume = 100.0f;
        orders[i].timestamp = static_cast<uint32_t>(1640995200 + i);
    }

    for (auto _ : state) {
        double total_value = 0.0;

        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& order : orders) {
            total_value += order.price * order.volume;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["OrdersPerSecond"] = benchmark::Counter(
            num_orders * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );

        benchmark::DoNotOptimize(total_value);
    }

    state.counters["OrderSizeBytes"] = sizeof(PackedOrder);
}
BENCHMARK(BM_PackedOrder_Processing)->Range(1000, 100000);

// ===== 原子操作性能测试 =====

static void BM_Atomic_Operations(benchmark::State& state) {
    std::atomic<int64_t> counter{0};
    const int operations = state.range(0);

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < operations; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["OperationsPerSecond"] = benchmark::Counter(
            operations * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );
    }

    benchmark::DoNotOptimize(counter.load());
}
BENCHMARK(BM_Atomic_Operations)->Range(10000, 1000000);

static void BM_Atomic_CAS_Operations(benchmark::State& state) {
    std::atomic<int64_t> value{0};
    const int operations = state.range(0);

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < operations; ++i) {
            int64_t expected = value.load();
            while (!value.compare_exchange_weak(expected, expected + 1,
                                               std::memory_order_relaxed)) {
                // CAS失败，重试
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        state.counters["CASOperationsPerSecond"] = benchmark::Counter(
            operations * 1000000.0 / elapsed.count(), benchmark::Counter::kIsRate
        );
    }

    benchmark::DoNotOptimize(value.load());
}
BENCHMARK(BM_Atomic_CAS_Operations)->Range(10000, 1000000);