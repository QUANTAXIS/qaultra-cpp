#include "qaultra/data/tick_broadcaster.hpp"
#include <iostream>
#include <iomanip>

namespace qaultra::data {

void TickBroadcaster::register_subscriber(const std::string& id) {
    subscribers_.emplace(id, Subscriber(id));
}

void TickBroadcaster::unregister_subscriber(const std::string& id) {
    subscribers_.erase(id);
}

void TickBroadcaster::push_tick(const std::string& date, const Tick& tick) {
    auto start = std::chrono::high_resolution_clock::now();

    // 检查日期是否变化
    std::shared_ptr<const std::unordered_map<std::string, Kline>> data_shared;

    if (date != current_date_) {
        // 日期变化：获取新数据
        current_date_ = date;
        data_shared = market_.get_date_shared(date);
        cached_data_ = data_shared;
        stats_.cache_misses++;
    } else {
        // 日期相同：使用缓存 (shared_ptr copy ~10-20 ns)
        stats_.cache_hits++;
        data_shared = cached_data_;
    }

    // 广播给所有订阅者 (零拷贝)
    for (auto& [id, subscriber] : subscribers_) {
        subscriber.receive(data_shared);  // shared_ptr copy
    }

    // 更新统计
    stats_.total_ticks++;
    stats_.total_broadcasts += subscribers_.size();

    auto end = std::chrono::high_resolution_clock::now();
    stats_.total_latency_ns +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

void TickBroadcaster::push_batch(const std::vector<Tick>& ticks) {
    for (const auto& tick : ticks) {
        // 从 datetime 提取日期部分 (YYYY-MM-DD)
        std::string date = tick.datetime.substr(0, 10);
        push_tick(date, tick);
    }
}

void TickBroadcaster::print_stats() const {
    std::cout << "\n📊 Tick 广播性能统计\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << "  总 Tick 数: " << stats_.total_ticks << "\n";
    std::cout << "  总广播次数: " << stats_.total_broadcasts << "\n";
    std::cout << "  订阅者数: " << subscribers_.size() << "\n";
    std::cout << "  缓存命中率: " << std::fixed << std::setprecision(2)
              << (stats_.cache_hit_rate() * 100) << "%\n";
    std::cout << "  平均延迟: " << std::fixed << std::setprecision(0)
              << stats_.avg_latency_ns() << " ns\n";

    if (stats_.total_broadcasts > 0 && stats_.total_latency_ns > 0) {
        double ticks_per_sec = (stats_.total_ticks * 1e9) / stats_.total_latency_ns;
        std::cout << "  吞吐量: " << std::fixed << std::setprecision(0)
                  << ticks_per_sec << " ticks/sec\n";
    }
}

void TickBroadcaster::clear_cache() {
    cached_data_.reset();
    current_date_.clear();
    market_.clear_shared_cache();
}

} // namespace qaultra::data
