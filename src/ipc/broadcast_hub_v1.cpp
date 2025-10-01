#include "qaultra/ipc/broadcast_hub_v1.hpp"
#include "iox/signal_watcher.hpp"

#include <iostream>
#include <thread>
#include <cstring>

namespace qaultra::ipc::v1 {

//==============================================================================
// DataBroadcaster Implementation
//==============================================================================

static std::atomic<bool> runtime_initialized{false};

DataBroadcaster::DataBroadcaster(const BroadcastConfig& config, const std::string& stream_name)
    : config_(config)
    , stream_name_(stream_name)
    , start_time_(std::chrono::steady_clock::now())
{
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid BroadcastConfig");
    }

    // 创建 IceOryx Publisher
    // Service: config_.service_name, Instance: stream_name, Event: "Data"
    try {
        // IceOryx ServiceDescription 需要 IdString (fixed size string)
        publisher_ = std::make_unique<Publisher>(
            iox::capro::ServiceDescription{
                iox::capro::IdString_t(iox::TruncateToCapacity, config_.service_name.c_str()),
                iox::capro::IdString_t(iox::TruncateToCapacity, stream_name_.c_str()),
                iox::capro::IdString_t(iox::TruncateToCapacity, "Data")
            }
        );

        // Note: In IceOryx 2.x, publishers are automatically offered upon construction
        // No need to call offer() explicitly

        stats_.start_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            start_time_.time_since_epoch()
        ).count();

    } catch (const std::exception& e) {
        std::cerr << "Failed to create DataBroadcaster: " << e.what() << std::endl;
        throw;
    }
}

DataBroadcaster::~DataBroadcaster() {
    // Publisher automatically stops offering when destroyed
    // No need to manually call stopOffer() in IceOryx 2.x
}

bool DataBroadcaster::initialize_runtime(const std::string& app_name) {
    if (runtime_initialized.exchange(true)) {
        return true;  // Already initialized
    }

    try {
        // IceOryx RuntimeName 需要 fixed size string
        iox::runtime::PoshRuntime::initRuntime(
            iox::RuntimeName_t(iox::TruncateToCapacity, app_name.c_str())
        );
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize IceOryx runtime: " << e.what() << std::endl;
        runtime_initialized = false;
        return false;
    }
}

bool DataBroadcaster::broadcast(const uint8_t* data,
                                size_t data_size,
                                size_t record_count,
                                MarketDataType type)
{
    if (!publisher_) {
        return false;
    }

    if (data_size > ZeroCopyMarketBlock::DATA_SIZE) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.errors++;
        return false;
    }

    auto start = std::chrono::steady_clock::now();

    bool broadcast_success = false;
    // 请求共享内存块
    publisher_->loan()
        .and_then([&](auto& sample) {
            // 填充数据块
            sample->sequence_number = sequence_number_++;
            sample->timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            sample->record_count = record_count;
            sample->data_type = type;
            sample->flags = 0;

            // 拷贝数据
            std::memcpy(sample->data, data, data_size);

            // 计算延迟 (before publish, since publish consumes the sample)
            auto end = std::chrono::steady_clock::now();
            uint64_t latency_ns = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
            );

            // 更新统计 (before publish, since we need to access sample fields)
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.blocks_sent++;
                stats_.records_sent += record_count;
                stats_.bytes_sent += ZeroCopyMarketBlock::BLOCK_SIZE;
                stats_.avg_latency_ns = (stats_.avg_latency_ns * (stats_.blocks_sent - 1) + latency_ns) / stats_.blocks_sent;
                stats_.max_latency_ns = std::max(stats_.max_latency_ns, latency_ns);
                stats_.min_latency_ns = std::min(stats_.min_latency_ns, latency_ns);
            }

            // 发布 (零拷贝传输) - this consumes the sample
            sample.publish();

            broadcast_success = true;
        })
        .or_else([&](auto& error) {
            std::cerr << "Failed to loan sample: " << static_cast<uint64_t>(error) << std::endl;
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.errors++;
        });

    return broadcast_success;
}

size_t DataBroadcaster::broadcast_batch(const uint8_t* data,
                                       size_t data_size,
                                       size_t record_count,
                                       MarketDataType type)
{
    if (!publisher_) {
        return 0;
    }

    size_t sent = 0;
    size_t offset = 0;
    const size_t block_data_size = ZeroCopyMarketBlock::DATA_SIZE;

    while (offset < data_size) {
        size_t chunk_size = std::min(block_data_size, data_size - offset);
        size_t chunk_records = (chunk_size * record_count) / data_size;

        if (broadcast(data + offset, chunk_size, chunk_records, type)) {
            sent += chunk_records;
        }

        offset += chunk_size;
    }

    return sent;
}

BroadcastStats DataBroadcaster::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    // 更新经过时间
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_time_);

    BroadcastStats stats_copy = stats_;
    stats_copy.elapsed_time_ns = elapsed.count();

    return stats_copy;
}

void DataBroadcaster::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.reset();
    start_time_ = std::chrono::steady_clock::now();
    stats_.start_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        start_time_.time_since_epoch()
    ).count();
}

bool DataBroadcaster::has_subscribers() const {
    if (!publisher_) {
        return false;
    }
    return publisher_->hasSubscribers();
}

size_t DataBroadcaster::get_subscriber_count() const {
    // IceOryx 不直接提供订阅者计数，需要通过内省 API
    // 这里简化处理
    return has_subscribers() ? 1 : 0;
}

void DataBroadcaster::update_stats(const ZeroCopyMarketBlock& block,
                                   uint64_t latency_ns,
                                   bool success)
{
    std::lock_guard<std::mutex> lock(stats_mutex_);

    if (success) {
        stats_.blocks_sent++;
        stats_.records_sent += block.record_count;
        stats_.bytes_sent += ZeroCopyMarketBlock::BLOCK_SIZE;

        // 更新延迟统计
        stats_.avg_latency_ns = (stats_.avg_latency_ns * (stats_.blocks_sent - 1) + latency_ns) / stats_.blocks_sent;
        stats_.max_latency_ns = std::max(stats_.max_latency_ns, latency_ns);
        stats_.min_latency_ns = std::min(stats_.min_latency_ns, latency_ns);
    } else {
        stats_.errors++;
    }
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

    // 创建 IceOryx Subscriber
    try {
        // IceOryx ServiceDescription 需要 IdString (fixed size string)
        subscriber_ = std::make_unique<Subscriber>(
            iox::capro::ServiceDescription{
                iox::capro::IdString_t(iox::TruncateToCapacity, config_.service_name.c_str()),
                iox::capro::IdString_t(iox::TruncateToCapacity, stream_name_.c_str()),
                iox::capro::IdString_t(iox::TruncateToCapacity, "Data")
            }
        );

        // Note: In IceOryx 2.x, subscribers automatically subscribe upon construction
        // No need to call subscribe() explicitly

    } catch (const std::exception& e) {
        std::cerr << "Failed to create DataSubscriber: " << e.what() << std::endl;
        throw;
    }
}

DataSubscriber::~DataSubscriber() {
    // Subscriber automatically unsubscribes when destroyed
    // No need to manually call unsubscribe() in IceOryx 2.x
}

bool DataSubscriber::initialize_runtime(const std::string& app_name) {
    return DataBroadcaster::initialize_runtime(app_name);
}

std::optional<std::vector<uint8_t>> DataSubscriber::receive() {
    if (!subscriber_) {
        return std::nullopt;
    }

    std::optional<std::vector<uint8_t>> result;

    subscriber_->take()
        .and_then([&](auto& sample) {
            // 拷贝数据
            size_t data_size = sample->record_count > 0
                ? ZeroCopyMarketBlock::DATA_SIZE
                : 0;

            std::vector<uint8_t> data(sample->data, sample->data + data_size);
            result = std::move(data);

            // 更新统计
            std::lock_guard<std::mutex> lock(stats_mutex_);
            receive_stats_.blocks_received++;
            receive_stats_.records_received += sample->record_count;
            receive_stats_.bytes_received += ZeroCopyMarketBlock::BLOCK_SIZE;
        })
        .or_else([&](auto& error) {
            // NO_CHUNK_AVAILABLE 不是错误
            if (error != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE) {
                std::cerr << "Error receiving chunk: " << static_cast<uint64_t>(error) << std::endl;
                std::lock_guard<std::mutex> lock(stats_mutex_);
                receive_stats_.missed_samples++;
            }
        });

    return result;
}

std::optional<std::vector<uint8_t>> DataSubscriber::receive_nowait() {
    // IceOryx take() 本身就是非阻塞的
    return receive();
}

std::optional<const ZeroCopyMarketBlock*> DataSubscriber::receive_block() {
    if (!subscriber_) {
        return std::nullopt;
    }

    std::optional<const ZeroCopyMarketBlock*> result;

    subscriber_->take()
        .and_then([&](auto& sample) {
            // 返回指针 (零拷贝)
            // 注意: 调用者需要确保在 sample 生命周期内使用
            result = sample.get();

            // 更新统计
            std::lock_guard<std::mutex> lock(stats_mutex_);
            receive_stats_.blocks_received++;
            receive_stats_.records_received += sample->record_count;
            receive_stats_.bytes_received += ZeroCopyMarketBlock::BLOCK_SIZE;
        })
        .or_else([&](auto& error) {
            if (error != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE) {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                receive_stats_.missed_samples++;
            }
        });

    return result;
}

bool DataSubscriber::has_data() const {
    if (!subscriber_) {
        return false;
    }
    return subscriber_->hasData();
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
{}

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
    // 需要 mutable mutex 或者使用 const_cast
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    std::unordered_map<std::string, BroadcastStats> all_stats;

    for (const auto& [name, broadcaster] : broadcasters_) {
        all_stats[name] = broadcaster->get_stats();
    }

    return all_stats;
}

} // namespace qaultra::ipc::v1
