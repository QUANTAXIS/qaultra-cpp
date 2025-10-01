/**
 * @file cross_lang_publisher.cpp
 * @brief C++ iceoryx2 发布器示例 - 发送数据给Rust订阅器
 *
 * 用法：
 *   ./cross_lang_publisher
 *
 * 配合 Rust 订阅器使用：
 *   cd /home/quantaxis/qars2
 *   cargo run --example cross_lang_subscriber
 */

#include "qaultra/ipc/cross_lang_data.hpp"
#include "iox2/iceoryx2.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>

using namespace qaultra::ipc;
using namespace iox2;

int main() {
    std::cout << "=== C++ iceoryx2 Publisher ===" << std::endl;
    std::cout << "Sending data to Rust subscriber via iceoryx2..." << std::endl;

    try {
        // 1. 创建 iceoryx2 节点
        auto node = NodeBuilder().create<ServiceType::Ipc>()
            .expect("Failed to create node");

        // 2. 创建服务（与Rust端使用相同的服务名）
        auto service_name = ServiceName::create("qars_market_data")
            .expect("Invalid service name");

        auto service = node.service_builder(std::move(service_name))
            .publish_subscribe<ZeroCopyMarketBlock>()
            .open_or_create()
            .expect("Failed to create service");

        // 3. 创建发布器
        auto publisher = service.publisher_builder()
            .create()
            .expect("Failed to create publisher");

        std::cout << "✓ Publisher ready on service 'qars_market_data'" << std::endl;
        std::cout << "✓ Waiting for Rust subscriber..." << std::endl;
        std::cout << std::endl;

        // 4. 发送测试数据
        uint64_t sequence = 0;

        for (int i = 0; i < 100; ++i) {
            // 构建市场Tick数据
            std::vector<MarketTick> ticks;

            for (int j = 0; j < 10; ++j) {
                MarketTick tick;
                tick.timestamp_ns = std::chrono::system_clock::now()
                    .time_since_epoch().count();
                tick.last_price = 100.0 + (i * 0.1) + (j * 0.01);
                tick.bid_price = tick.last_price - 0.01;
                tick.ask_price = tick.last_price + 0.01;
                tick.volume = 1000 + i * 100 + j * 10;

                // 设置股票代码
                std::snprintf(tick.symbol, sizeof(tick.symbol), "SH60000%d", i % 10);

                ticks.push_back(tick);
            }

            // 使用 Builder 构建数据块
            auto block = DataBlockBuilder()
                .sequence(sequence++)
                .source(1001)  // C++ Publisher ID
                .tick_data(ticks.data(), ticks.size())
                .build();

            // 发布数据
            auto sample = publisher.loan_uninit()
                .expect("Failed to loan sample");

            auto initialized = sample.write_payload(block);

            send(std::move(initialized))
                .expect("Failed to send");

            std::cout << "[" << i << "] Sent " << ticks.size() << " ticks, "
                      << "seq=" << block.sequence_id << ", "
                      << "size=" << block.length << " bytes" << std::endl;

            // 模拟100Hz发送频率
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << std::endl;
        std::cout << "✓ Sent 100 batches (1000 ticks total)" << std::endl;
        std::cout << "✓ C++ Publisher completed" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
