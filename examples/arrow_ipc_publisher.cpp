/**
 * @file arrow_ipc_publisher.cpp
 * @brief 使用标准 Apache Arrow C++ 库的发布器
 *
 * 正确的方案：
 * 1. 使用 Apache Arrow C++ 创建 RecordBatch
 * 2. 序列化为标准 Arrow IPC Stream 格式
 * 3. 通过 iceoryx2 共享内存传输
 */

#include <arrow/api.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/writer.h>
#include "iox2/iceoryx2.hpp"

#include <iostream>
#include <random>

using namespace iox2;

// 使用固定大小的缓冲区以满足 iceoryx2 要求
const size_t ARROW_IPC_BUFFER_SIZE = 4 * 1024 * 1024; // 4MB

struct alignas(64) ArrowIPCBuffer {
    uint8_t data[ARROW_IPC_BUFFER_SIZE];
    size_t length;
    uint64_t sequence_id;
    int64_t timestamp;
    size_t batch_size;
};

int main() {
    std::cout << "=== C++ Apache Arrow IPC Publisher ===" << std::endl;
    std::cout << "Using Apache Arrow C++ " << ARROW_VERSION_STRING << std::endl;
    std::cout << std::endl;

    try {
        // 1. 创建 iceoryx2 节点和服务
        auto node = NodeBuilder().create<ServiceType::Ipc>()
            .expect("Failed to create node");

        auto service_name = ServiceName::create("qars_arrow_ipc")
            .expect("Invalid service name");

        auto service = node.service_builder(std::move(service_name))
            .publish_subscribe<ArrowIPCBuffer>()
            .open_or_create()
            .expect("Failed to create service");

        auto publisher = service.publisher_builder()
            .create()
            .expect("Failed to create publisher");

        std::cout << "✓ Arrow IPC Publisher ready" << std::endl;
        std::cout << std::endl;

        // 2. 创建 Arrow Schema (OHLCV 市场数据)
        auto schema = arrow::schema({
            arrow::field("symbol", arrow::utf8()),
            arrow::field("timestamp", arrow::int64()),
            arrow::field("open", arrow::float64()),
            arrow::field("high", arrow::float64()),
            arrow::field("low", arrow::float64()),
            arrow::field("close", arrow::float64()),
            arrow::field("volume", arrow::float64()),
        });

        // 3. 发送 10 个批次
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> price_dist(100.0, 200.0);
        std::uniform_real_distribution<> volume_dist(1000.0, 100000.0);

        for (int batch_id = 0; batch_id < 10; ++batch_id) {
            const int num_rows = 1000;

            // 构建列数据
            arrow::StringBuilder symbol_builder;
            arrow::Int64Builder timestamp_builder;
            arrow::DoubleBuilder open_builder;
            arrow::DoubleBuilder high_builder;
            arrow::DoubleBuilder low_builder;
            arrow::DoubleBuilder close_builder;
            arrow::DoubleBuilder volume_builder;

            for (int i = 0; i < num_rows; ++i) {
                // Symbol
                symbol_builder.Append("AAPL");

                // Timestamp (纳秒)
                int64_t ts = 1727788800000000000LL + (batch_id * 1000 + i) * 1000000000LL;
                timestamp_builder.Append(ts);

                // OHLCV
                double base_price = price_dist(gen);
                open_builder.Append(base_price);
                high_builder.Append(base_price + 2.0);
                low_builder.Append(base_price - 1.0);
                close_builder.Append(base_price + 0.5);
                volume_builder.Append(volume_dist(gen));
            }

            // 完成构建
            std::shared_ptr<arrow::Array> symbol_array;
            std::shared_ptr<arrow::Array> timestamp_array;
            std::shared_ptr<arrow::Array> open_array;
            std::shared_ptr<arrow::Array> high_array;
            std::shared_ptr<arrow::Array> low_array;
            std::shared_ptr<arrow::Array> close_array;
            std::shared_ptr<arrow::Array> volume_array;

            symbol_builder.Finish(&symbol_array);
            timestamp_builder.Finish(&timestamp_array);
            open_builder.Finish(&open_array);
            high_builder.Finish(&high_array);
            low_builder.Finish(&low_array);
            close_builder.Finish(&close_array);
            volume_builder.Finish(&volume_array);

            // 创建 RecordBatch
            auto record_batch = arrow::RecordBatch::Make(
                schema,
                num_rows,
                {symbol_array, timestamp_array, open_array, high_array,
                 low_array, close_array, volume_array}
            );

            std::cout << "Batch " << batch_id << ": "
                      << record_batch->num_rows() << " rows, "
                      << record_batch->num_columns() << " columns" << std::endl;

            // 4. 序列化为 Arrow IPC Stream 格式
            auto output_stream = arrow::io::BufferOutputStream::Create(ARROW_IPC_BUFFER_SIZE)
                .ValueOrDie();

            auto writer = arrow::ipc::MakeStreamWriter(output_stream, schema)
                .ValueOrDie();

            writer->WriteRecordBatch(*record_batch);
            writer->Close();

            auto buffer = output_stream->Finish().ValueOrDie();

            if (static_cast<size_t>(buffer->size()) > ARROW_IPC_BUFFER_SIZE) {
                std::cerr << "Error: Buffer size exceeds maximum" << std::endl;
                continue;
            }

            // 5. 复制到 iceoryx2 缓冲区
            ArrowIPCBuffer ipc_buffer = {};
            std::memcpy(ipc_buffer.data, buffer->data(), buffer->size());
            ipc_buffer.length = buffer->size();
            ipc_buffer.sequence_id = batch_id;
            ipc_buffer.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            ipc_buffer.batch_size = num_rows;

            // 6. 发送
            auto sample = publisher.loan_uninit().expect("Failed to loan");
            send(sample.write_payload(ipc_buffer)).expect("Failed to send");

            std::cout << "  ✓ Sent batch " << batch_id
                      << ", size=" << buffer->size() << " bytes" << std::endl;
            std::cout << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "✓ Sent 10 Arrow IPC batches (10,000 rows total)" << std::endl;
        std::cout << "✓ C++ Arrow IPC Publisher completed" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
