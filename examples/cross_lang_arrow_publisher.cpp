/**
 * @file cross_lang_arrow_publisher.cpp
 * @brief C++ iceoryx2 Arrow格式发布器
 *
 * 发送类似Polars DataFrame的列式数据给Rust
 */

#include "qaultra/ipc/cross_lang_data.hpp"
#include "qaultra/ipc/arrow_compat.hpp"
#include "iox2/iceoryx2.hpp"

#include <iostream>
#include <vector>
#include <random>

using namespace qaultra::ipc;
using namespace iox2;

int main() {
    std::cout << "=== C++ iceoryx2 Arrow Publisher ===" << std::endl;
    std::cout << "Sending Arrow-format data to Rust..." << std::endl;

    try {
        // 1. 创建节点和服务
        auto node = NodeBuilder().create<ServiceType::Ipc>()
            .expect("Failed to create node");

        auto service_name = ServiceName::create("qars_arrow_data")
            .expect("Invalid service name");

        auto service = node.service_builder(std::move(service_name))
            .publish_subscribe<ZeroCopyMarketBlock>()
            .open_or_create()
            .expect("Failed to create service");

        auto publisher = service.publisher_builder()
            .create()
            .expect("Failed to create publisher");

        std::cout << "✓ Arrow Publisher ready" << std::endl;
        std::cout << std::endl;

        // 2. 生成测试数据（模拟市场数据）
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> price_dist(100.0, 200.0);
        std::uniform_int_distribution<> volume_dist(1000, 100000);

        for (int batch_id = 0; batch_id < 10; ++batch_id) {
            // 构建Arrow RecordBatch（类似Polars DataFrame）
            size_t num_rows = 1000;

            std::vector<int64_t> timestamps;
            std::vector<double> open_prices;
            std::vector<double> high_prices;
            std::vector<double> low_prices;
            std::vector<double> close_prices;
            std::vector<int64_t> volumes;

            for (size_t i = 0; i < num_rows; ++i) {
                int64_t ts = std::chrono::system_clock::now().time_since_epoch().count() + i * 1000000;
                double base_price = price_dist(gen);

                timestamps.push_back(ts);
                open_prices.push_back(base_price);
                high_prices.push_back(base_price + price_dist(gen) * 0.02);
                low_prices.push_back(base_price - price_dist(gen) * 0.02);
                close_prices.push_back(base_price + price_dist(gen) * 0.01);
                volumes.push_back(volume_dist(gen));
            }

            // 使用Builder构建Arrow Batch
            auto arrow_batch = ArrowRecordBatchBuilder()
                .add_int64("timestamp", timestamps)
                .add_double("open", open_prices)
                .add_double("high", high_prices)
                .add_double("low", low_prices)
                .add_double("close", close_prices)
                .add_int64("volume", volumes)
                .build();

            std::cout << "Built Arrow RecordBatch:" << std::endl;
            std::cout << "  Rows: " << arrow_batch.num_rows() << std::endl;
            std::cout << "  Columns: " << arrow_batch.num_columns() << std::endl;

            // 3. 序列化到 ZeroCopyMarketBlock
            auto block = ZeroCopyMarketBlock();
            block.sequence_id = batch_id;
            block.source_id = 2001;  // Arrow Publisher ID

            if (!arrow_batch.serialize_to(block, 1)) {
                std::cerr << "Failed to serialize Arrow batch" << std::endl;
                continue;
            }

            block.calculate_checksum();

            // 4. 通过iceoryx2发送
            auto sample = publisher.loan_uninit().expect("Failed to loan");
            auto initialized = sample.write_payload(block);
            send(std::move(initialized)).expect("Failed to send");

            std::cout << "  ✓ Sent batch " << batch_id
                      << ", size=" << block.length << " bytes" << std::endl;
            std::cout << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "✓ Sent 10 Arrow batches (10,000 rows total)" << std::endl;
        std::cout << "✓ C++ Arrow Publisher completed" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
