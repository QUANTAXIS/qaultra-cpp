// Cross-Language C++ Publisher Example
//
// 演示使用 qadataswap 从 C++ 发送数据到 Rust/Python
// 基于零拷贝共享内存传输

#include <qadataswap_core.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>

arrow::Result<std::shared_ptr<arrow::RecordBatch>> create_sample_market_data(int num_rows) {
    // 创建 Arrow Schema
    auto schema = arrow::schema({
        arrow::field("timestamp", arrow::timestamp(arrow::TimeUnit::NANO)),
        arrow::field("symbol", arrow::utf8()),
        arrow::field("price", arrow::float64()),
        arrow::field("volume", arrow::int64())
    });

    // 创建 Array Builders
    arrow::TimestampBuilder timestamp_builder(arrow::timestamp(arrow::TimeUnit::NANO), arrow::default_memory_pool());
    arrow::StringBuilder symbol_builder;
    arrow::DoubleBuilder price_builder;
    arrow::Int64Builder volume_builder;

    // 生成数据
    auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOGL"};

    for (int i = 0; i < num_rows; ++i) {
        ARROW_RETURN_NOT_OK(timestamp_builder.Append(now + i * 1000000));  // 1ms intervals
        ARROW_RETURN_NOT_OK(symbol_builder.Append(symbols[i % 3]));
        ARROW_RETURN_NOT_OK(price_builder.Append(100.0 + i * 0.1));
        ARROW_RETURN_NOT_OK(volume_builder.Append(1000 + i * 10));
    }

    // Finish arrays
    std::shared_ptr<arrow::Array> timestamp_array, symbol_array, price_array, volume_array;
    ARROW_RETURN_NOT_OK(timestamp_builder.Finish(&timestamp_array));
    ARROW_RETURN_NOT_OK(symbol_builder.Finish(&symbol_array));
    ARROW_RETURN_NOT_OK(price_builder.Finish(&price_array));
    ARROW_RETURN_NOT_OK(volume_builder.Finish(&volume_array));

    // Create RecordBatch
    return arrow::RecordBatch::Make(
        schema,
        num_rows,
        {timestamp_array, symbol_array, price_array, volume_array}
    );
}

int main() {
    std::cout << "=============================================="<< std::endl;
    std::cout << "📡 QARS Cross-Language C++ Publisher" << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << std::endl;

    std::string stream_name = "qars_market_stream";
    constexpr size_t size_mb = 100;

    try {
        // 创建共享内存 Arena 并创建 Writer
        auto arena = std::make_unique<qadataswap::SharedMemoryArena>(
            stream_name,
            size_mb * 1024 * 1024,  // MB to bytes
            3  // buffer count
        );

        if (!arena->CreateWriter()) {
            std::cerr << "❌ Failed to create writer" << std::endl;
            return 1;
        }

        std::cout << "✓ Publisher created: " << stream_name << std::endl;
        std::cout << "✓ Shared memory size: " << size_mb << " MB" << std::endl;
        std::cout << "✓ Waiting for subscribers (Rust/Python)..." << std::endl;
        std::cout << std::endl;

        // 等待订阅器连接
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 发送 100 批数据
        for (int batch_idx = 0; batch_idx < 100; ++batch_idx) {
            // 创建示例市场数据
            auto batch_result = create_sample_market_data(1000);
            if (!batch_result.ok()) {
                std::cerr << "❌ Failed to create data: " << batch_result.status().ToString() << std::endl;
                return 1;
            }

            auto batch = batch_result.ValueOrDie();

            // 发送
            auto status = arena->WriteRecordBatch(batch);
            if (!status.ok()) {
                std::cerr << "❌ Failed to write batch: " << status.ToString() << std::endl;
                return 1;
            }

            arena->NotifyDataReady();

            std::cout << "[Batch " << (batch_idx + 1) << "] ✓ Sent "
                      << batch->num_rows() << " rows" << std::endl;

            // 显示第一批数据的样本
            if (batch_idx == 0) {
                std::cout << "\nSample data (first batch):" << std::endl;
                std::cout << "Schema: " << batch->schema()->ToString() << std::endl;
                std::cout << std::endl;
            }

            // 每秒发送一批
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << std::endl;
        std::cout << "=============================================="<< std::endl;
        std::cout << "✓ Publisher finished" << std::endl;
        std::cout << "  Total batches sent: 100" << std::endl;
        std::cout << "==============================================" << std::endl;

        arena->Close();

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
