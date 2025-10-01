/**
 * @file cpp_rust_bridge_example.cpp
 * @brief C++/Rust 跨语言通信桥接示例
 *
 * 由于 IceOryx (C++) 和 iceoryx2 (Rust) 不兼容，本示例展示如何使用 JSON 序列化
 * 作为桥接层实现跨语言通信。
 *
 * 架构:
 * C++ Publisher → IceOryx → JSON Bridge → Named Pipe → Rust Consumer
 */

#include "qaultra/ipc/broadcast_hub.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace qaultra::ipc;

// 市场数据结构
struct MarketTick {
    char symbol[16];
    double price;
    double volume;
    uint64_t timestamp;

    // 转换为 JSON
    json to_json() const {
        return json{
            {"symbol", symbol},
            {"price", price},
            {"volume", volume},
            {"timestamp", timestamp}
        };
    }

    // 从 JSON 构造
    static MarketTick from_json(const json& j) {
        MarketTick tick;
        std::strncpy(tick.symbol, j["symbol"].get<std::string>().c_str(), sizeof(tick.symbol) - 1);
        tick.symbol[sizeof(tick.symbol) - 1] = '\0';
        tick.price = j["price"];
        tick.volume = j["volume"];
        tick.timestamp = j["timestamp"];
        return tick;
    }
};

/**
 * @brief C++ Publisher: 发送二进制数据到 IceOryx
 */
void cpp_publisher_mode() {
    std::cout << "=== C++ Publisher Mode ===" << std::endl;
    std::cout << "发送数据到 IceOryx (C++ 生态)" << std::endl;

    // 初始化 IceOryx
    DataBroadcaster::initialize_runtime("cpp_publisher");

    BroadcastConfig config;
    config.service_name = "MarketData";
    config.max_subscribers = 10;

    DataBroadcaster broadcaster(config, "cpp_stream");

    // 发送市场数据
    for (int i = 0; i < 10; ++i) {
        MarketTick tick;
        std::snprintf(tick.symbol, sizeof(tick.symbol), "STOCK%03d", i);
        tick.price = 100.0 + i * 0.5;
        tick.volume = 1000.0 * (i + 1);
        tick.timestamp = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );

        bool success = broadcaster.broadcast(
            reinterpret_cast<const uint8_t*>(&tick),
            sizeof(tick),
            1,
            MarketDataType::Tick
        );

        std::cout << "Sent: " << tick.symbol
                  << " @ " << tick.price
                  << " - " << (success ? "OK" : "FAILED") << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    auto stats = broadcaster.get_stats();
    std::cout << "\nPublisher Stats:" << std::endl;
    std::cout << "  Blocks sent: " << stats.blocks_sent << std::endl;
    std::cout << "  Records sent: " << stats.records_sent << std::endl;
    std::cout << "  Avg latency: " << stats.avg_latency_ns / 1000.0 << " μs" << std::endl;
}

/**
 * @brief C++ Subscriber: 从 IceOryx 接收二进制数据
 */
void cpp_subscriber_mode() {
    std::cout << "=== C++ Subscriber Mode ===" << std::endl;
    std::cout << "接收数据来自 IceOryx (C++ 生态)" << std::endl;

    // 初始化 IceOryx
    DataSubscriber::initialize_runtime("cpp_subscriber");

    BroadcastConfig config;
    config.service_name = "MarketData";

    DataSubscriber subscriber(config, "cpp_stream");

    int received = 0;
    for (int i = 0; i < 15; ++i) {
        auto data = subscriber.receive();
        if (data) {
            // 解析为 MarketTick
            const MarketTick* tick = reinterpret_cast<const MarketTick*>(data->data());
            std::cout << "Received: " << tick->symbol
                      << " @ " << tick->price
                      << " vol=" << tick->volume << std::endl;
            received++;
        } else {
            std::cout << "No data available" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    auto stats = subscriber.get_receive_stats();
    std::cout << "\nSubscriber Stats:" << std::endl;
    std::cout << "  Blocks received: " << stats.blocks_received << std::endl;
    std::cout << "  Records received: " << stats.records_received << std::endl;
}

/**
 * @brief JSON Bridge: IceOryx → JSON → Named Pipe
 *
 * 从 IceOryx 接收二进制数据，转换为 JSON，写入 Named Pipe 供 Rust 消费
 */
void json_bridge_cpp_to_rust() {
    std::cout << "=== JSON Bridge: C++ → Rust ===" << std::endl;

    const char* pipe_path = "/tmp/cpp_to_rust_pipe";

    // 创建 Named Pipe
    unlink(pipe_path);
    if (mkfifo(pipe_path, 0666) != 0) {
        std::cerr << "Failed to create named pipe" << std::endl;
        return;
    }

    std::cout << "Created named pipe: " << pipe_path << std::endl;
    std::cout << "Waiting for Rust reader to connect..." << std::endl;

    // 初始化 IceOryx Subscriber
    DataSubscriber::initialize_runtime("json_bridge");

    BroadcastConfig config;
    config.service_name = "MarketData";
    DataSubscriber subscriber(config, "cpp_stream");

    // 打开 Named Pipe (阻塞直到有读者)
    int fd = open(pipe_path, O_WRONLY);
    if (fd < 0) {
        std::cerr << "Failed to open named pipe for writing" << std::endl;
        return;
    }

    std::cout << "Rust reader connected, starting bridge..." << std::endl;

    // 桥接循环
    for (int i = 0; i < 20; ++i) {
        auto data = subscriber.receive();
        if (data) {
            // 解析为 MarketTick
            const MarketTick* tick = reinterpret_cast<const MarketTick*>(data->data());

            // 转换为 JSON
            json j = tick->to_json();
            std::string json_str = j.dump() + "\n";

            // 写入 Named Pipe
            ssize_t written = write(fd, json_str.c_str(), json_str.size());
            if (written > 0) {
                std::cout << "Bridged: " << tick->symbol
                          << " → JSON → Rust" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    close(fd);
    unlink(pipe_path);
    std::cout << "Bridge closed" << std::endl;
}

/**
 * @brief 完整示例: C++ Publisher + JSON Bridge
 */
void full_cpp_to_rust_example() {
    std::cout << "\n==== 完整示例: C++ → JSON Bridge → Rust ====\n" << std::endl;

    // 启动 Publisher 线程
    std::thread publisher_thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        cpp_publisher_mode();
    });

    // 启动 Bridge 线程
    std::thread bridge_thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        json_bridge_cpp_to_rust();
    });

    // 等待完成
    publisher_thread.join();
    bridge_thread.join();

    std::cout << "\n示例完成!" << std::endl;
    std::cout << "要测试 Rust 端接收，请运行:" << std::endl;
    std::cout << "  cat /tmp/cpp_to_rust_pipe" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "\n=== C++/Rust 跨语言通信桥接示例 ===\n" << std::endl;
    std::cout << "说明: IceOryx (C++) 和 iceoryx2 (Rust) 不兼容，" << std::endl;
    std::cout << "本示例展示如何使用 JSON + Named Pipe 作为桥接。\n" << std::endl;

    if (argc < 2) {
        std::cout << "用法:" << std::endl;
        std::cout << "  " << argv[0] << " publisher    # C++ IceOryx 发布者" << std::endl;
        std::cout << "  " << argv[0] << " subscriber   # C++ IceOryx 订阅者" << std::endl;
        std::cout << "  " << argv[0] << " bridge       # JSON 桥接 (C++ → Rust)" << std::endl;
        std::cout << "  " << argv[0] << " full         # 完整示例" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "publisher") {
        cpp_publisher_mode();
    } else if (mode == "subscriber") {
        cpp_subscriber_mode();
    } else if (mode == "bridge") {
        json_bridge_cpp_to_rust();
    } else if (mode == "full") {
        full_cpp_to_rust_example();
    } else {
        std::cerr << "未知模式: " << mode << std::endl;
        return 1;
    }

    return 0;
}
