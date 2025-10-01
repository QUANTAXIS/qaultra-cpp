// Cross-Language C++ Subscriber Example
//
// 演示使用 qadataswap 从 Rust/Python 接收数据
// 基于零拷贝共享内存传输

#include <qadataswap_core.h>
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "=============================================="<< std::endl;
    std::cout << "📨 QARS Cross-Language C++ Subscriber" << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << std::endl;

    std::string stream_name = "qars_market_stream";
    constexpr size_t size_mb = 100;
    constexpr int max_messages = 100;

    try {
        // 创建共享内存 Arena 并附加为 Reader
        auto arena = std::make_unique<qadataswap::SharedMemoryArena>(
            stream_name,
            size_mb * 1024 * 1024,  // MB to bytes
            3  // buffer count
        );

        if (!arena->AttachReader()) {
            std::cerr << "❌ Failed to attach reader" << std::endl;
            return 1;
        }

        std::cout << "✓ Subscriber created: " << stream_name << std::endl;
        std::cout << "✓ Waiting for data from Rust/Python..." << std::endl;
        std::cout << std::endl;

        int received_count = 0;

        while (received_count < max_messages) {
            // 接收 RecordBatch (5秒超时)
            auto result = arena->ReadRecordBatch(5000);

            if (result.ok()) {
                auto batch = result.ValueOrDie();
                received_count++;

                std::cout << "[Message " << received_count << "] ✓ Received "
                          << batch->num_rows() << " rows × "
                          << batch->num_columns() << " columns" << std::endl;

                // 显示第一批数据的样本
                if (received_count == 1) {
                    std::cout << "\nSample data (first message):" << std::endl;
                    std::cout << "Schema: " << batch->schema()->ToString() << std::endl;
                    std::cout << "First row preview:" << std::endl;

                    for (int col = 0; col < batch->num_columns(); ++col) {
                        auto column = batch->column(col);
                        auto field = batch->schema()->field(col);
                        std::cout << "  " << field->name() << ": "
                                  << column->ToString() << std::endl;
                    }
                    std::cout << std::endl;
                }

                // 简单统计 - 查找 price 列
                for (int col = 0; col < batch->num_columns(); ++col) {
                    auto field = batch->schema()->field(col);
                    if (field->name() == "price") {
                        auto column = batch->column(col);
                        if (column->type()->id() == arrow::Type::DOUBLE) {
                            auto typed_column = std::static_pointer_cast<arrow::DoubleArray>(column);
                            double sum = 0.0;
                            for (int64_t i = 0; i < typed_column->length(); ++i) {
                                if (!typed_column->IsNull(i)) {
                                    sum += typed_column->Value(i);
                                }
                            }
                            double avg = sum / typed_column->length();
                            std::cout << "  → Average price: " << avg << std::endl;
                        }
                        break;
                    }
                }

            } else {
                std::cout << "⏳ Timeout or error, waiting for more data..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        std::cout << std::endl;
        std::cout << "=============================================="<< std::endl;
        std::cout << "✓ Subscriber finished" << std::endl;
        std::cout << "  Total messages received: " << received_count << std::endl;
        std::cout << "==============================================" << std::endl;

        arena->Close();

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
