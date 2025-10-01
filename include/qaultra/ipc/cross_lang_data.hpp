#pragma once

#include <cstdint>
#include <cstring>
#include <chrono>

namespace qaultra::ipc {

/**
 * @brief 零拷贝市场数据块（C++/Rust跨语言兼容）
 *
 * 此结构完全匹配 Rust 端的定义：
 * src/qadata/broadcast_hub.rs::ZeroCopyMarketBlock
 *
 * 内存布局保证：
 * - #[repr(C, align(64))] in Rust
 * - alignas(64) in C++
 * - 固定大小数组，无动态分配
 * - POD类型，可安全memcpy
 */
struct alignas(64) ZeroCopyMarketBlock {
    /// 数据缓冲区 (64KB)
    uint8_t data[65536];

    /// 实际数据长度
    size_t length;

    /// 数据条数
    size_t record_count;

    /// 数据类型编码
    /// 1 = Tick
    /// 2 = Bar1Min
    /// 3 = Bar5Min
    /// 4 = Bar15Min
    /// 5 = Bar1Hour
    /// 6 = Bar1Day
    /// 7 = OrderBook
    /// 8 = Trade
    uint32_t data_type;

    /// 时间戳 (纳秒)
    int64_t timestamp;

    /// 序列号
    uint64_t sequence_id;

    /// 源标识
    uint32_t source_id;

    /// 校验和
    uint64_t checksum;

    /// 元数据 (256 bytes)
    uint8_t metadata[256];

    // ========== C++ 辅助方法 ==========

    ZeroCopyMarketBlock() {
        std::memset(this, 0, sizeof(ZeroCopyMarketBlock));
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    /**
     * @brief 写入数据到缓冲区
     */
    bool write_data(const void* src, size_t size) {
        if (size > sizeof(data)) {
            return false;
        }
        length = size;
        std::memcpy(data, src, size);
        return true;
    }

    /**
     * @brief 设置数据类型
     */
    void set_tick() { data_type = 1; }
    void set_bar_1min() { data_type = 2; }
    void set_bar_5min() { data_type = 3; }
    void set_bar_1day() { data_type = 6; }

    /**
     * @brief 更新时间戳为当前时间
     */
    void update_timestamp() {
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    /**
     * @brief 计算简单校验和
     */
    void calculate_checksum() {
        checksum = 0;
        for (size_t i = 0; i < length && i < sizeof(data); ++i) {
            checksum += data[i];
        }
    }

    /**
     * @brief 验证校验和
     */
    bool verify_checksum() const {
        uint64_t sum = 0;
        for (size_t i = 0; i < length && i < sizeof(data); ++i) {
            sum += data[i];
        }
        return sum == checksum;
    }
};

// 编译时检查：确保与Rust定义匹配
static_assert(sizeof(ZeroCopyMarketBlock::data) == 65536,
              "data buffer must be 65536 bytes");
static_assert(sizeof(ZeroCopyMarketBlock::metadata) == 256,
              "metadata must be 256 bytes");
static_assert(alignof(ZeroCopyMarketBlock) == 64,
              "ZeroCopyMarketBlock must be 64-byte aligned");
static_assert(std::is_standard_layout<ZeroCopyMarketBlock>::value,
              "ZeroCopyMarketBlock must be standard layout for C/Rust compatibility");

/**
 * @brief 简单的市场Tick数据（示例）
 *
 * 可以序列化到 ZeroCopyMarketBlock::data 中
 */
struct MarketTick {
    int64_t timestamp_ns;
    double last_price;
    double bid_price;
    double ask_price;
    uint64_t volume;
    char symbol[32];

    MarketTick() {
        std::memset(this, 0, sizeof(MarketTick));
    }
};

/**
 * @brief 数据块构建器
 */
class DataBlockBuilder {
public:
    DataBlockBuilder() : block_() {
        block_.sequence_id = 0;
    }

    DataBlockBuilder& sequence(uint64_t seq) {
        block_.sequence_id = seq;
        return *this;
    }

    DataBlockBuilder& source(uint32_t src) {
        block_.source_id = src;
        return *this;
    }

    DataBlockBuilder& tick_data(const MarketTick* ticks, size_t count) {
        block_.set_tick();
        block_.record_count = count;
        block_.write_data(ticks, count * sizeof(MarketTick));
        return *this;
    }

    DataBlockBuilder& raw_data(const void* data, size_t size, uint32_t type) {
        block_.data_type = type;
        block_.write_data(data, size);
        return *this;
    }

    ZeroCopyMarketBlock build() {
        block_.update_timestamp();
        block_.calculate_checksum();
        return block_;
    }

    ZeroCopyMarketBlock& get() {
        return block_;
    }

private:
    ZeroCopyMarketBlock block_;
};

} // namespace qaultra::ipc
