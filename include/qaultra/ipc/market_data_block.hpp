#pragma once

#include <cstdint>
#include <cstring>
#include <array>

namespace qaultra::ipc {

/**
 * @brief 市场数据类型枚举
 */
enum class MarketDataType : uint8_t {
    Tick = 0,       // 逐笔数据
    Bar = 1,        // K线数据
    Kline = 2,      // K线数据 (别名)
    OrderBook = 3,  // 订单簿
    Trade = 4,      // 成交数据
    Unknown = 255
};

/**
 * @brief 零拷贝市场数据块
 *
 * 固定大小的数据块用于 IceOryx 零拷贝传输
 * 8KB 数据块可容纳约 100-200 条 tick 数据
 */
struct alignas(64) ZeroCopyMarketBlock {
    // 数据块大小常量
    static constexpr size_t BLOCK_SIZE = 8192;
    static constexpr size_t DATA_SIZE = BLOCK_SIZE - 32;  // 预留 32 字节元数据

    // 元数据 (32 bytes)
    uint64_t sequence_number;      // 序列号
    uint64_t timestamp_ns;         // 纳秒级时间戳
    uint64_t record_count;         // 数据记录数量
    MarketDataType data_type;      // 数据类型
    uint8_t flags;                 // 标志位
    uint8_t reserved[6];           // 保留字节

    // 数据区 (8160 bytes)
    uint8_t data[DATA_SIZE];

    /**
     * @brief 默认构造函数
     */
    ZeroCopyMarketBlock() noexcept
        : sequence_number(0)
        , timestamp_ns(0)
        , record_count(0)
        , data_type(MarketDataType::Unknown)
        , flags(0)
        , reserved{0}
        , data{0}
    {}

    /**
     * @brief 复制数据到块中
     * @param src 源数据指针
     * @param size 数据大小
     * @return 是否成功 (false 表示数据过大)
     */
    bool copy_data(const void* src, size_t size) noexcept {
        if (size > DATA_SIZE) {
            return false;
        }
        std::memcpy(data, src, size);
        return true;
    }

    /**
     * @brief 获取可用数据大小
     */
    static constexpr size_t available_data_size() noexcept {
        return DATA_SIZE;
    }

    /**
     * @brief 获取数据指针
     */
    const uint8_t* get_data() const noexcept {
        return data;
    }

    /**
     * @brief 获取可写数据指针
     */
    uint8_t* get_data_mut() noexcept {
        return data;
    }

    /**
     * @brief 清空数据块
     */
    void clear() noexcept {
        sequence_number = 0;
        timestamp_ns = 0;
        record_count = 0;
        data_type = MarketDataType::Unknown;
        flags = 0;
        std::memset(data, 0, DATA_SIZE);
    }
};

// 编译时验证大小
static_assert(sizeof(ZeroCopyMarketBlock) == ZeroCopyMarketBlock::BLOCK_SIZE,
              "ZeroCopyMarketBlock size must be exactly 8KB");
static_assert(alignof(ZeroCopyMarketBlock) == 64,
              "ZeroCopyMarketBlock must be 64-byte aligned for optimal cache performance");

/**
 * @brief 批量数据块 (用于高吞吐场景)
 */
struct MarketDataBatch {
    static constexpr size_t MAX_BATCH_SIZE = 100;

    size_t count;
    std::array<ZeroCopyMarketBlock, MAX_BATCH_SIZE> blocks;

    MarketDataBatch() : count(0) {}

    bool add_block(const ZeroCopyMarketBlock& block) {
        if (count >= MAX_BATCH_SIZE) {
            return false;
        }
        blocks[count++] = block;
        return true;
    }

    void clear() {
        count = 0;
    }
};

} // namespace qaultra::ipc
