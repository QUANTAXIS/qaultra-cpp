#pragma once

#include <cstddef>
#include <string>
#include <optional>

namespace qaultra::ipc {

/**
 * @brief 数据广播配置
 *
 * 对应 QARS Rust 版本的 BroadcastConfig
 */
struct BroadcastConfig {
    // 订阅者管理
    size_t max_subscribers = 1000;          // 最大并发订阅者数量

    // 性能调优
    size_t batch_size = 10000;              // 批量处理大小
    size_t buffer_depth = 500;              // 缓冲区深度
    bool zero_copy_enabled = true;          // 启用零拷贝 (IceOryx always zero-copy)

    // 内存管理
    size_t memory_pool_size_mb = 1024;      // 内存池大小 (MB)
    bool compression_enabled = false;       // 数据压缩 (通常不需要)

    // 监控与心跳
    size_t heartbeat_interval_ms = 1000;    // 心跳间隔 (毫秒)
    bool stats_enabled = true;              // 启用统计监控

    // NUMA 和 CPU 亲和性
    bool numa_aware = false;                // NUMA 感知内存分配
    std::optional<int> cpu_affinity;        // CPU 亲和性绑定

    // IceOryx 特定配置
    std::string service_name = "QAULTRA";   // IceOryx 服务名称
    std::string instance_name = "Broadcast";// IceOryx 实例名称
    size_t queue_capacity = 1000;           // 队列容量

    /**
     * @brief 默认构造函数 - 优化配置
     */
    BroadcastConfig() = default;

    /**
     * @brief 创建高性能配置
     */
    static BroadcastConfig high_performance() {
        BroadcastConfig config;
        config.max_subscribers = 1500;
        config.batch_size = 20000;
        config.buffer_depth = 1000;
        config.memory_pool_size_mb = 2048;
        config.queue_capacity = 2000;
        return config;
    }

    /**
     * @brief 创建低延迟配置
     */
    static BroadcastConfig low_latency() {
        BroadcastConfig config;
        config.max_subscribers = 100;
        config.batch_size = 1000;
        config.buffer_depth = 100;
        config.memory_pool_size_mb = 512;
        config.queue_capacity = 200;
        return config;
    }

    /**
     * @brief 创建大规模配置
     */
    static BroadcastConfig massive_scale() {
        BroadcastConfig config;
        config.max_subscribers = 2000;
        config.batch_size = 50000;
        config.buffer_depth = 2000;
        config.memory_pool_size_mb = 4096;
        config.queue_capacity = 5000;
        return config;
    }

    /**
     * @brief 验证配置有效性
     */
    bool validate() const {
        if (max_subscribers == 0 || max_subscribers > 10000) {
            return false;
        }
        if (batch_size == 0 || batch_size > 1000000) {
            return false;
        }
        if (buffer_depth == 0 || buffer_depth > 10000) {
            return false;
        }
        if (memory_pool_size_mb == 0 || memory_pool_size_mb > 65536) {
            return false;
        }
        return true;
    }
};

/**
 * @brief 广播统计信息
 */
struct BroadcastStats {
    // 发送统计
    uint64_t blocks_sent = 0;           // 发送块数
    uint64_t records_sent = 0;          // 发送记录数
    uint64_t bytes_sent = 0;            // 发送字节数
    uint64_t errors = 0;                // 错误计数

    // 订阅者统计
    size_t active_subscribers = 0;      // 活跃订阅者数量
    size_t total_subscribers = 0;       // 总订阅者数量

    // 性能统计 (纳秒)
    uint64_t avg_latency_ns = 0;        // 平均延迟
    uint64_t max_latency_ns = 0;        // 最大延迟
    uint64_t min_latency_ns = UINT64_MAX; // 最小延迟

    // 资源统计
    size_t memory_usage_bytes = 0;      // 内存使用量
    double cpu_usage_percent = 0.0;     // CPU 使用率

    // 时间统计
    uint64_t start_time_ns = 0;         // 开始时间
    uint64_t elapsed_time_ns = 0;       // 运行时间

    /**
     * @brief 计算吞吐量 (记录/秒)
     */
    double throughput_records_per_sec() const {
        if (elapsed_time_ns == 0) return 0.0;
        return static_cast<double>(records_sent) * 1e9 / elapsed_time_ns;
    }

    /**
     * @brief 计算吞吐量 (MB/秒)
     */
    double throughput_mb_per_sec() const {
        if (elapsed_time_ns == 0) return 0.0;
        return static_cast<double>(bytes_sent) / (1024 * 1024) * 1e9 / elapsed_time_ns;
    }

    /**
     * @brief 计算成功率
     */
    double success_rate() const {
        uint64_t total = blocks_sent + errors;
        if (total == 0) return 100.0;
        return static_cast<double>(blocks_sent) * 100.0 / total;
    }

    /**
     * @brief 重置统计
     */
    void reset() {
        blocks_sent = 0;
        records_sent = 0;
        bytes_sent = 0;
        errors = 0;
        avg_latency_ns = 0;
        max_latency_ns = 0;
        min_latency_ns = UINT64_MAX;
    }
};

} // namespace qaultra::ipc
