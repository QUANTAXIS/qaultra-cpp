#pragma once

#include "market_data_block.hpp"
#include "broadcast_config.hpp"

#include "iox2/iceoryx2.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <optional>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace qaultra::ipc::v2 {

/**
 * @brief 广播错误类型 (iceoryx2 v2)
 */
enum class BroadcastError {
    ServiceCreationFailed,
    NodeCreationFailed,
    PublisherCreationFailed,
    SubscriberCreationFailed,
    DataOverflow,
    InvalidConfiguration,
    LoanFailed,
    SendFailed,
    ReceiveFailed,
    Unknown
};

/**
 * @brief 数据广播器 (Publisher - iceoryx2)
 *
 * 基于 iceoryx2 实现零拷贝数据分发
 * 支持 C++/Rust 跨语言通信
 */
class DataBroadcaster {
public:
    static constexpr auto SERVICE_TYPE = iox2::ServiceType::Ipc;
    using Node = iox2::Node<SERVICE_TYPE>;
    using Service = iox2::PortFactoryPublishSubscribe<SERVICE_TYPE, ZeroCopyMarketBlock, void>;
    using Publisher = iox2::Publisher<SERVICE_TYPE, ZeroCopyMarketBlock, void>;

    /**
     * @brief 构造函数
     * @param config 广播配置
     * @param stream_name 数据流名称 (对应 iceoryx2 service name)
     */
    explicit DataBroadcaster(const BroadcastConfig& config,
                            const std::string& stream_name = "market_data");

    /**
     * @brief 析构函数
     */
    ~DataBroadcaster();

    // 禁用拷贝
    DataBroadcaster(const DataBroadcaster&) = delete;
    DataBroadcaster& operator=(const DataBroadcaster&) = delete;

    // 允许移动
    DataBroadcaster(DataBroadcaster&&) noexcept = default;
    DataBroadcaster& operator=(DataBroadcaster&&) noexcept = default;

    /**
     * @brief 广播单条数据
     * @param data 数据指针
     * @param data_size 数据大小
     * @param record_count 记录数量
     * @param type 数据类型
     * @return 是否成功
     */
    bool broadcast(const uint8_t* data,
                   size_t data_size,
                   size_t record_count,
                   MarketDataType type);

    /**
     * @brief 批量广播数据
     * @param data 数据指针
     * @param data_size 数据大小
     * @param record_count 记录数量
     * @param type 数据类型
     * @return 成功发送的记录数
     */
    size_t broadcast_batch(const uint8_t* data,
                          size_t data_size,
                          size_t record_count,
                          MarketDataType type);

    /**
     * @brief 获取统计信息
     */
    BroadcastStats get_stats() const;

    /**
     * @brief 重置统计信息
     */
    void reset_stats();

    /**
     * @brief 获取配置
     */
    const BroadcastConfig& get_config() const { return config_; }

    /**
     * @brief 获取流名称
     */
    const std::string& get_stream_name() const { return stream_name_; }

    /**
     * @brief 检查是否有订阅者
     */
    bool has_subscribers() const;

    /**
     * @brief 获取订阅者数量 (iceoryx2 不直接提供此功能)
     */
    size_t get_subscriber_count() const { return 0; }

private:
    BroadcastConfig config_;
    std::string stream_name_;

    // iceoryx2 对象 (stored by value)
    std::optional<Node> node_;
    std::optional<Service> service_;
    std::optional<Publisher> publisher_;

    // 统计信息
    mutable std::mutex stats_mutex_;
    BroadcastStats stats_;
    std::atomic<uint64_t> sequence_number_{0};

    // 计时器
    std::chrono::steady_clock::time_point start_time_;

    /**
     * @brief 创建数据块
     */
    ZeroCopyMarketBlock create_block(const uint8_t* data,
                                      size_t data_size,
                                      size_t record_count,
                                      MarketDataType type);

    /**
     * @brief 更新统计信息
     */
    void update_stats(const ZeroCopyMarketBlock& block, uint64_t latency_ns, bool success);
};

/**
 * @brief 数据订阅器 (Subscriber - iceoryx2)
 *
 * 基于 iceoryx2 实现零拷贝数据接收
 * 支持 C++/Rust 跨语言通信
 */
class DataSubscriber {
public:
    static constexpr auto SERVICE_TYPE = iox2::ServiceType::Ipc;
    using Node = iox2::Node<SERVICE_TYPE>;
    using Service = iox2::PortFactoryPublishSubscribe<SERVICE_TYPE, ZeroCopyMarketBlock, void>;
    using Subscriber = iox2::Subscriber<SERVICE_TYPE, ZeroCopyMarketBlock, void>;

    /**
     * @brief 构造函数
     * @param config 广播配置
     * @param stream_name 数据流名称
     */
    explicit DataSubscriber(const BroadcastConfig& config,
                           const std::string& stream_name = "market_data");

    /**
     * @brief 析构函数
     */
    ~DataSubscriber();

    // 禁用拷贝
    DataSubscriber(const DataSubscriber&) = delete;
    DataSubscriber& operator=(const DataSubscriber&) = delete;

    // 允许移动
    DataSubscriber(DataSubscriber&&) noexcept = default;
    DataSubscriber& operator=(DataSubscriber&&) noexcept = default;

    /**
     * @brief 接收数据 (非阻塞)
     * @return 数据块的拷贝 (可选)
     */
    std::optional<std::vector<uint8_t>> receive();

    /**
     * @brief 接收数据 (非阻塞)
     * @return 数据块的拷贝 (可选)
     */
    std::optional<std::vector<uint8_t>> receive_nowait();

    /**
     * @brief 接收原始数据块 (零拷贝，但需要手动释放)
     * @return 数据块指针 (可选)
     */
    std::optional<const ZeroCopyMarketBlock*> receive_block();

    /**
     * @brief 获取配置
     */
    const BroadcastConfig& get_config() const { return config_; }

    /**
     * @brief 获取流名称
     */
    const std::string& get_stream_name() const { return stream_name_; }

    /**
     * @brief 检查是否有数据可用
     */
    bool has_data() const;

    /**
     * @brief 获取接收统计
     */
    struct ReceiveStats {
        uint64_t blocks_received = 0;
        uint64_t records_received = 0;
        uint64_t bytes_received = 0;
        uint64_t missed_samples = 0;
    };

    ReceiveStats get_receive_stats() const;

    /**
     * @brief 重置接收统计
     */
    void reset_receive_stats();

private:
    BroadcastConfig config_;
    std::string stream_name_;

    // iceoryx2 对象 (stored by value)
    std::optional<Node> node_;
    std::optional<Service> service_;
    std::optional<Subscriber> subscriber_;

    // 接收统计
    mutable std::mutex stats_mutex_;
    ReceiveStats receive_stats_;
};

/**
 * @brief 多流广播管理器 (iceoryx2)
 *
 * 管理多个数据流的广播和订阅
 */
class BroadcastManager {
public:
    explicit BroadcastManager(const BroadcastConfig& default_config);

    /**
     * @brief 创建广播器
     */
    std::shared_ptr<DataBroadcaster> create_broadcaster(const std::string& stream_name);

    /**
     * @brief 创建订阅器
     */
    std::shared_ptr<DataSubscriber> create_subscriber(const std::string& stream_name);

    /**
     * @brief 获取广播器
     */
    std::shared_ptr<DataBroadcaster> get_broadcaster(const std::string& stream_name);

    /**
     * @brief 获取订阅器
     */
    std::shared_ptr<DataSubscriber> get_subscriber(const std::string& stream_name);

    /**
     * @brief 获取所有广播器统计
     */
    std::unordered_map<std::string, BroadcastStats> get_all_stats() const;

private:
    BroadcastConfig default_config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<DataBroadcaster>> broadcasters_;
    std::unordered_map<std::string, std::shared_ptr<DataSubscriber>> subscribers_;
};

} // namespace qaultra::ipc::v2
