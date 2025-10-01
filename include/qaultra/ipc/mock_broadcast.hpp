#pragma once

/**
 * @file mock_broadcast.hpp
 * @brief Mock implementation of broadcast system for testing without IceOryx
 *
 * 这是一个模拟实现，用于在没有 IceOryx 依赖的情况下进行开发和测试
 */

#include "market_data_block.hpp"
#include "broadcast_config.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <optional>
#include <vector>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

namespace qaultra::ipc::mock {

/**
 * @brief 模拟的共享内存队列
 */
class MockSharedQueue {
public:
    MockSharedQueue(size_t capacity) : capacity_(capacity) {}

    bool push(const ZeroCopyMarketBlock& block) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= capacity_) {
            return false;
        }
        queue_.push(block);
        cv_.notify_all();
        return true;
    }

    std::optional<ZeroCopyMarketBlock> pop(bool wait = false) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (wait) {
            cv_.wait(lock, [this] { return !queue_.empty(); });
        } else {
            if (queue_.empty()) {
                return std::nullopt;
            }
        }

        auto block = queue_.front();
        queue_.pop();
        return block;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    size_t capacity_;
    std::queue<ZeroCopyMarketBlock> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

/**
 * @brief 模拟的数据广播器
 */
class MockDataBroadcaster {
public:
    explicit MockDataBroadcaster(const BroadcastConfig& config, const std::string& stream_name);
    ~MockDataBroadcaster();

    MockDataBroadcaster(const MockDataBroadcaster&) = delete;
    MockDataBroadcaster& operator=(const MockDataBroadcaster&) = delete;

    static bool initialize_runtime(const std::string& app_name);

    bool broadcast(const uint8_t* data, size_t data_size, size_t record_count, MarketDataType type);
    size_t broadcast_batch(const uint8_t* data, size_t data_size, size_t record_count, MarketDataType type);

    BroadcastStats get_stats() const;
    void reset_stats();

    const BroadcastConfig& get_config() const { return config_; }
    const std::string& get_stream_name() const { return stream_name_; }

    bool has_subscribers() const;
    size_t get_subscriber_count() const { return subscribers_.size(); }

    // Internal API for mock implementation
    std::shared_ptr<MockSharedQueue> get_queue() { return queue_; }

private:
    BroadcastConfig config_;
    std::string stream_name_;
    std::shared_ptr<MockSharedQueue> queue_;

    mutable std::mutex stats_mutex_;
    BroadcastStats stats_;
    std::atomic<uint64_t> sequence_number_{0};

    std::chrono::steady_clock::time_point start_time_;

    // Mock subscriber registry
    static std::mutex subscribers_mutex_;
    static std::unordered_map<std::string, std::vector<std::weak_ptr<MockSharedQueue>>> subscribers_;

    void update_stats(const ZeroCopyMarketBlock& block, uint64_t latency_ns, bool success);
};

/**
 * @brief 模拟的数据订阅器
 */
class MockDataSubscriber {
public:
    explicit MockDataSubscriber(const BroadcastConfig& config, const std::string& stream_name);
    ~MockDataSubscriber();

    MockDataSubscriber(const MockDataSubscriber&) = delete;
    MockDataSubscriber& operator=(const MockDataSubscriber&) = delete;

    static bool initialize_runtime(const std::string& app_name);

    std::optional<std::vector<uint8_t>> receive();
    std::optional<std::vector<uint8_t>> receive_nowait();
    std::optional<const ZeroCopyMarketBlock*> receive_block();

    const BroadcastConfig& get_config() const { return config_; }
    const std::string& get_stream_name() const { return stream_name_; }

    bool has_data() const;

    struct ReceiveStats {
        uint64_t blocks_received = 0;
        uint64_t records_received = 0;
        uint64_t bytes_received = 0;
        uint64_t missed_samples = 0;
    };

    ReceiveStats get_receive_stats() const;
    void reset_receive_stats();

private:
    BroadcastConfig config_;
    std::string stream_name_;
    std::shared_ptr<MockSharedQueue> queue_;

    mutable std::mutex stats_mutex_;
    ReceiveStats receive_stats_;

    // For zero-copy receive
    std::optional<ZeroCopyMarketBlock> cached_block_;
};

} // namespace qaultra::ipc::mock
