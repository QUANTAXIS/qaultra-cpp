#include "qaultra/ipc/broadcast_hub_v2.hpp"

#include <iostream>
#include <thread>
#include <cstring>

namespace qaultra::ipc::v2 {

//==============================================================================
// DataBroadcaster Implementation
//==============================================================================

DataBroadcaster::DataBroadcaster(const BroadcastConfig& config, const std::string& stream_name)
    : config_(config)
    , stream_name_(stream_name)
    , start_time_(std::chrono::steady_clock::now())
{
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid BroadcastConfig");
    }

    try {
        // 创建 iceoryx2 Node
        auto node_result = iox2::NodeBuilder().create<iox2::ServiceType::Ipc>();
        if (node_result.has_error()) {
            throw std::runtime_error("Failed to create iceoryx2 node");
        }
        node_ = std::move(node_result.value());

        // 创建 Service Name
        auto service_name_result = iox2::ServiceName::create(stream_name_.c_str());
        if (service_name_result.has_error()) {
            throw std::runtime_error("Failed to create service name: " + stream_name_);
        }

        // 创建 Service (publish_subscribe)
        auto service_result = node_->service_builder(std::move(service_name_result.value()))
                                   .publish_subscribe<ZeroCopyMarketBlock>()
                                   .open_or_create();
        if (service_result.has_error()) {
            throw std::runtime_error("Failed to create/open service: " + stream_name_);
        }
        service_ = std::move(service_result.value());

        // 创建 Publisher
        auto publisher_result = service_->publisher_builder().create();
        if (publisher_result.has_error()) {
            throw std::runtime_error("Failed to create publisher");
        }
        publisher_ = std::move(publisher_result.value());

        stats_.start_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            start_time_.time_since_epoch()
        ).count();

    } catch (const std::exception& e) {
        std::cerr << "Failed to create DataBroadcaster (v2): " << e.what() << std::endl;
        throw;
    }
}

DataBroadcaster::~DataBroadcaster() {
    // iceoryx2 handles cleanup automatically
}

ZeroCopyMarketBlock DataBroadcaster::create_block(const uint8_t* data,
                                                   size_t data_size,
                                                   size_t record_count,
                                                   MarketDataType type) {
    ZeroCopyMarketBlock block{};
    block.sequence_number = sequence_number_.fetch_add(1, std::memory_order_relaxed);
    block.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    block.record_count = record_count;
    block.data_type = type;
    block.flags = 0;

    // 拷贝数据
    size_t copy_size = std::min(data_size, ZeroCopyMarketBlock::DATA_SIZE);
    std::memcpy(block.data, data, copy_size);

    return block;
}

bool DataBroadcaster::broadcast(const uint8_t* data,
                               size_t data_size,
                               size_t record_count,
                               MarketDataType type) {
    if (data_size > ZeroCopyMarketBlock::DATA_SIZE) {
        std::cerr << "Data size " << data_size << " exceeds maximum "
                  << ZeroCopyMarketBlock::DATA_SIZE << std::endl;
        return false;
    }

    auto start = std::chrono::steady_clock::now();

    try {
        // Loan sample from iceoryx2
        auto sample_result = publisher_->loan_uninit();
        if (sample_result.has_error()) {
            std::cerr << "Failed to loan sample" << std::endl;
            return false;
        }

        // Create block
        auto block = create_block(data, data_size, record_count, type);

        // Write payload and initialize
        auto initialized_sample = sample_result.value().write_payload(block);

        // Send sample
        auto send_result = iox2::send(std::move(initialized_sample));
        if (send_result.has_error()) {
            std::cerr << "Failed to send sample" << std::endl;
            return false;
        }

        // Calculate latency
        auto end = std::chrono::steady_clock::now();
        uint64_t latency_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        );

        // Update stats
        update_stats(block, latency_ns, true);

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception in broadcast: " << e.what() << std::endl;
        return false;
    }
}

size_t DataBroadcaster::broadcast_batch(const uint8_t* data,
                                       size_t data_size,
                                       size_t record_count,
                                       MarketDataType type) {
    size_t total_sent = 0;
    size_t remaining = data_size;
    size_t offset = 0;

    while (remaining > 0) {
        size_t chunk_size = std::min(remaining, ZeroCopyMarketBlock::DATA_SIZE);
        size_t chunk_records = (chunk_size * record_count) / data_size;

        if (broadcast(data + offset, chunk_size, chunk_records, type)) {
            total_sent += chunk_records;
            offset += chunk_size;
            remaining -= chunk_size;
        } else {
            break;
        }
    }

    return total_sent;
}

void DataBroadcaster::update_stats(const ZeroCopyMarketBlock& block, uint64_t latency_ns, bool success) {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    if (success) {
        stats_.blocks_sent++;
        stats_.records_sent += block.record_count;
        stats_.bytes_sent += ZeroCopyMarketBlock::BLOCK_SIZE;

        // 更新延迟统计
        if (stats_.blocks_sent == 1) {
            stats_.avg_latency_ns = latency_ns;
            stats_.max_latency_ns = latency_ns;
            stats_.min_latency_ns = latency_ns;
        } else {
            stats_.avg_latency_ns = (stats_.avg_latency_ns * (stats_.blocks_sent - 1) + latency_ns) / stats_.blocks_sent;
            stats_.max_latency_ns = std::max(stats_.max_latency_ns, latency_ns);
            stats_.min_latency_ns = std::min(stats_.min_latency_ns, latency_ns);
        }
    } else {
        stats_.errors++;
    }
}

BroadcastStats DataBroadcaster::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void DataBroadcaster::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = BroadcastStats{};
    stats_.start_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        start_time_.time_since_epoch()
    ).count();
}

bool DataBroadcaster::has_subscribers() const {
    // iceoryx2 doesn't provide direct subscriber count API
    // Always return true for now
    return true;
}

//==============================================================================
// DataSubscriber Implementation
//==============================================================================

DataSubscriber::DataSubscriber(const BroadcastConfig& config, const std::string& stream_name)
    : config_(config)
    , stream_name_(stream_name)
{
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid BroadcastConfig");
    }

    try {
        // 创建 iceoryx2 Node
        auto node_result = iox2::NodeBuilder().create<iox2::ServiceType::Ipc>();
        if (node_result.has_error()) {
            throw std::runtime_error("Failed to create iceoryx2 node");
        }
        node_ = std::move(node_result.value());

        // 创建 Service Name
        auto service_name_result = iox2::ServiceName::create(stream_name_.c_str());
        if (service_name_result.has_error()) {
            throw std::runtime_error("Failed to create service name: " + stream_name_);
        }

        // 创建 Service (publish_subscribe)
        auto service_result = node_->service_builder(std::move(service_name_result.value()))
                                   .publish_subscribe<ZeroCopyMarketBlock>()
                                   .open_or_create();
        if (service_result.has_error()) {
            throw std::runtime_error("Failed to create/open service: " + stream_name_);
        }
        service_ = std::move(service_result.value());

        // 创建 Subscriber
        auto subscriber_result = service_->subscriber_builder().create();
        if (subscriber_result.has_error()) {
            throw std::runtime_error("Failed to create subscriber");
        }
        subscriber_ = std::move(subscriber_result.value());

    } catch (const std::exception& e) {
        std::cerr << "Failed to create DataSubscriber (v2): " << e.what() << std::endl;
        throw;
    }
}

DataSubscriber::~DataSubscriber() {
    // iceoryx2 handles cleanup automatically
}

std::optional<std::vector<uint8_t>> DataSubscriber::receive() {
    return receive_nowait();  // iceoryx2 receive is non-blocking by default
}

std::optional<std::vector<uint8_t>> DataSubscriber::receive_nowait() {
    try {
        auto sample_result = subscriber_->receive();
        if (sample_result.has_error()) {
            return std::nullopt;
        }

        auto sample = std::move(sample_result.value());
        if (!sample.has_value()) {
            return std::nullopt;
        }

        // 获取数据并拷贝
        const auto& block = sample->payload();
        std::vector<uint8_t> data(block.data, block.data + ZeroCopyMarketBlock::DATA_SIZE);

        // 更新统计
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            receive_stats_.blocks_received++;
            receive_stats_.records_received += block.record_count;
            receive_stats_.bytes_received += ZeroCopyMarketBlock::BLOCK_SIZE;
        }

        return data;

    } catch (const std::exception& e) {
        std::cerr << "Exception in receive: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<const ZeroCopyMarketBlock*> DataSubscriber::receive_block() {
    // iceoryx2 C++ bindings manage sample lifetime automatically
    // Cannot return raw pointer safely
    // Users should use receive() or receive_nowait() instead
    return std::nullopt;
}

bool DataSubscriber::has_data() const {
    // iceoryx2 doesn't provide a non-consuming peek API
    // Always return false to avoid confusion
    return false;
}

DataSubscriber::ReceiveStats DataSubscriber::get_receive_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return receive_stats_;
}

void DataSubscriber::reset_receive_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    receive_stats_ = ReceiveStats{};
}

//==============================================================================
// BroadcastManager Implementation
//==============================================================================

BroadcastManager::BroadcastManager(const BroadcastConfig& default_config)
    : default_config_(default_config)
{
}

std::shared_ptr<DataBroadcaster> BroadcastManager::create_broadcaster(const std::string& stream_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = broadcasters_.find(stream_name);
    if (it != broadcasters_.end()) {
        return it->second;
    }

    auto broadcaster = std::make_shared<DataBroadcaster>(default_config_, stream_name);
    broadcasters_[stream_name] = broadcaster;
    return broadcaster;
}

std::shared_ptr<DataSubscriber> BroadcastManager::create_subscriber(const std::string& stream_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = subscribers_.find(stream_name);
    if (it != subscribers_.end()) {
        return it->second;
    }

    auto subscriber = std::make_shared<DataSubscriber>(default_config_, stream_name);
    subscribers_[stream_name] = subscriber;
    return subscriber;
}

std::shared_ptr<DataBroadcaster> BroadcastManager::get_broadcaster(const std::string& stream_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = broadcasters_.find(stream_name);
    return (it != broadcasters_.end()) ? it->second : nullptr;
}

std::shared_ptr<DataSubscriber> BroadcastManager::get_subscriber(const std::string& stream_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscribers_.find(stream_name);
    return (it != subscribers_.end()) ? it->second : nullptr;
}

std::unordered_map<std::string, BroadcastStats> BroadcastManager::get_all_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, BroadcastStats> all_stats;

    for (const auto& [name, broadcaster] : broadcasters_) {
        all_stats[name] = broadcaster->get_stats();
    }

    return all_stats;
}

} // namespace qaultra::ipc::v2
