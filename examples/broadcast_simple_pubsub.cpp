/**
 * @file broadcast_simple_pubsub.cpp
 * @brief QAULTRA IPC 简单的 Pub/Sub 示例
 *
 * 展示如何使用 DataBroadcaster 和 DataSubscriber 实现零拷贝数据传输
 */

#include "qaultra/ipc/broadcast_hub.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

using namespace qaultra::ipc;

// 全局标志用于优雅退出
std::atomic<bool> keep_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived signal, shutting down..." << std::endl;
        keep_running = false;
    }
}

// 简单的市场数据结构
struct MarketTick {
    char symbol[16];      // 股票代码
    double price;         // 价格
    double volume;        // 成交量
    uint64_t timestamp;   // 时间戳
};

//==============================================================================
// Publisher (发布者)
//==============================================================================

void run_publisher() {
    std::cout << "=== Publisher Started ===" << std::endl;

    // 初始化 IceOryx 运行时
    DataBroadcaster::initialize_runtime("market_publisher");

    // 创建配置
    BroadcastConfig config;
    config.max_subscribers = 10;
    config.batch_size = 1000;
    config.buffer_depth = 100;

    // 创建广播器
    DataBroadcaster broadcaster(config, "market_stream");

    std::cout << "Publisher: Broadcasting market data..." << std::endl;
    std::cout << "Publisher: Press Ctrl+C to stop\n" << std::endl;

    // 模拟市场数据
    uint64_t counter = 0;
    const char* symbols[] = {"SH600000", "SH600036", "SH600519", "SZ000001", "SZ000002"};
    const size_t num_symbols = sizeof(symbols) / sizeof(symbols[0]);

    while (keep_running) {
        // 创建市场数据
        MarketTick tick;
        size_t symbol_idx = counter % num_symbols;
        std::strncpy(tick.symbol, symbols[symbol_idx], sizeof(tick.symbol) - 1);
        tick.symbol[sizeof(tick.symbol) - 1] = '\0';

        tick.price = 100.0 + (counter % 100) * 0.5;
        tick.volume = 1000.0 + (counter % 10000);
        tick.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

        // 广播数据
        bool success = broadcaster.broadcast(
            reinterpret_cast<const uint8_t*>(&tick),
            sizeof(tick),
            1,
            MarketDataType::Tick
        );

        if (success) {
            std::cout << "Published: " << tick.symbol
                      << " Price=" << tick.price
                      << " Volume=" << tick.volume
                      << std::endl;
        } else {
            std::cerr << "Failed to publish tick!" << std::endl;
        }

        counter++;

        // 每秒发送 10 条
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 每 10 条打印统计
        if (counter % 10 == 0) {
            auto stats = broadcaster.get_stats();
            std::cout << "\n📊 Publisher Stats:" << std::endl;
            std::cout << "  Blocks sent: " << stats.blocks_sent << std::endl;
            std::cout << "  Records sent: " << stats.records_sent << std::endl;
            std::cout << "  Throughput: " << stats.throughput_records_per_sec() << " rec/s" << std::endl;
            std::cout << "  Success rate: " << stats.success_rate() << "%" << std::endl;
            std::cout << "  Avg latency: " << stats.avg_latency_ns / 1000.0 << " μs\n" << std::endl;
        }
    }

    std::cout << "Publisher: Stopped" << std::endl;
}

//==============================================================================
// Subscriber (订阅者)
//==============================================================================

void run_subscriber(int subscriber_id) {
    std::cout << "=== Subscriber " << subscriber_id << " Started ===" << std::endl;

    // 初始化 IceOryx 运行时
    std::string app_name = "market_subscriber_" + std::to_string(subscriber_id);
    DataSubscriber::initialize_runtime(app_name);

    // 创建配置
    BroadcastConfig config;
    config.max_subscribers = 10;

    // 创建订阅器
    DataSubscriber subscriber(config, "market_stream");

    std::cout << "Subscriber " << subscriber_id << ": Listening for market data..." << std::endl;

    uint64_t received_count = 0;

    while (keep_running) {
        // 非阻塞接收
        auto data = subscriber.receive_nowait();

        if (data && data->size() >= sizeof(MarketTick)) {
            // 解析数据
            const MarketTick* tick = reinterpret_cast<const MarketTick*>(data->data());

            std::cout << "[Sub" << subscriber_id << "] Received: "
                      << tick->symbol
                      << " Price=" << tick->price
                      << " Volume=" << tick->volume
                      << std::endl;

            received_count++;

            // 每 10 条打印统计
            if (received_count % 10 == 0) {
                auto stats = subscriber.get_receive_stats();
                std::cout << "\n📊 Subscriber " << subscriber_id << " Stats:" << std::endl;
                std::cout << "  Blocks received: " << stats.blocks_received << std::endl;
                std::cout << "  Records received: " << stats.records_received << std::endl;
                std::cout << "  Missed samples: " << stats.missed_samples << "\n" << std::endl;
            }
        }

        // 休眠以避免忙等
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Subscriber " << subscriber_id << ": Stopped" << std::endl;
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[]) {
    // 安装信号处理器
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "\n🚀 QAULTRA IPC Simple Pub/Sub Example" << std::endl;
    std::cout << "====================================\n" << std::endl;

    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " publisher    # Run as publisher" << std::endl;
        std::cout << "  " << argv[0] << " subscriber [id]  # Run as subscriber (default id=1)" << std::endl;
        std::cout << "\nExample:" << std::endl;
        std::cout << "  Terminal 1: " << argv[0] << " publisher" << std::endl;
        std::cout << "  Terminal 2: " << argv[0] << " subscriber 1" << std::endl;
        std::cout << "  Terminal 3: " << argv[0] << " subscriber 2" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    try {
        if (mode == "publisher") {
            run_publisher();
        } else if (mode == "subscriber") {
            int subscriber_id = (argc >= 3) ? std::atoi(argv[2]) : 1;
            run_subscriber(subscriber_id);
        } else {
            std::cerr << "Unknown mode: " << mode << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n✅ Example completed successfully" << std::endl;
    return 0;
}
