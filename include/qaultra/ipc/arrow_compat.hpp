#pragma once

#include "cross_lang_data.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <array>

namespace qaultra::ipc {

/**
 * @brief Arrow元数据（简化版）
 *
 * 匹配Rust端的 ArrowHeader 定义
 */
struct ArrowMetadata {
    uint32_t schema_id;      // Schema ID
    uint32_t num_rows;       // 行数
    uint32_t num_columns;    // 列数
    uint32_t payload_size;   // 实际数据大小
    uint64_t timestamp_ns;   // 时间戳
};

/**
 * @brief 简化的Arrow列数据
 *
 * 支持基本类型的列式存储
 */
template<typename T>
struct ArrowColumn {
    std::string name;
    std::vector<T> data;
    std::vector<uint8_t> null_bitmap;  // null标记

    size_t size() const { return data.size(); }

    bool is_null(size_t index) const {
        if (index >= data.size()) return true;
        size_t byte_idx = index / 8;
        size_t bit_idx = index % 8;
        if (byte_idx >= null_bitmap.size()) return false;
        return (null_bitmap[byte_idx] & (1 << bit_idx)) == 0;
    }
};

/**
 * @brief 简化的Arrow RecordBatch
 *
 * 类似于 Polars DataFrame 的内存表示
 */
class ArrowRecordBatch {
public:
    ArrowRecordBatch() : num_rows_(0) {}

    /**
     * @brief 添加整数列
     */
    void add_int64_column(const std::string& name, const std::vector<int64_t>& data) {
        int64_columns_.push_back({name, data, {}});
        if (num_rows_ == 0) {
            num_rows_ = data.size();
        }
    }

    /**
     * @brief 添加浮点列
     */
    void add_double_column(const std::string& name, const std::vector<double>& data) {
        double_columns_.push_back({name, data, {}});
        if (num_rows_ == 0) {
            num_rows_ = data.size();
        }
    }

    /**
     * @brief 添加字符串列（定长）
     */
    void add_string_column(const std::string& name, const std::vector<std::string>& data, size_t max_len = 32) {
        // 转换为定长字符数组
        std::vector<std::array<char, 64>> fixed_strings;
        for (const auto& str : data) {
            std::array<char, 64> fixed_str{};
            size_t copy_len = std::min(str.size(), max_len);
            std::memcpy(fixed_str.data(), str.c_str(), copy_len);
            fixed_strings.push_back(fixed_str);
        }
        string_columns_.push_back({name, fixed_strings, {}});
        if (num_rows_ == 0) {
            num_rows_ = data.size();
        }
    }

    /**
     * @brief 序列化到 ZeroCopyMarketBlock
     *
     * 将Arrow RecordBatch序列化为字节流，存入共享内存块
     */
    bool serialize_to(ZeroCopyMarketBlock& block, uint32_t schema_id = 1) {
        // 计算所需空间
        size_t required_size = sizeof(ArrowMetadata);

        // 列元数据
        struct ColumnMeta {
            uint8_t type;      // 0=int64, 1=double, 2=string
            uint16_t name_len;
            uint32_t data_len;
        };

        std::vector<ColumnMeta> column_metas;
        std::vector<std::vector<uint8_t>> serialized_data;

        // 序列化int64列
        for (const auto& col : int64_columns_) {
            ColumnMeta meta;
            meta.type = 0;
            meta.name_len = col.name.size();
            meta.data_len = col.data.size() * sizeof(int64_t);

            std::vector<uint8_t> data;
            data.insert(data.end(), col.name.begin(), col.name.end());
            const uint8_t* col_data = reinterpret_cast<const uint8_t*>(col.data.data());
            data.insert(data.end(), col_data, col_data + meta.data_len);

            column_metas.push_back(meta);
            serialized_data.push_back(std::move(data));

            required_size += sizeof(ColumnMeta) + meta.name_len + meta.data_len;
        }

        // 序列化double列
        for (const auto& col : double_columns_) {
            ColumnMeta meta;
            meta.type = 1;
            meta.name_len = col.name.size();
            meta.data_len = col.data.size() * sizeof(double);

            std::vector<uint8_t> data;
            data.insert(data.end(), col.name.begin(), col.name.end());
            const uint8_t* col_data = reinterpret_cast<const uint8_t*>(col.data.data());
            data.insert(data.end(), col_data, col_data + meta.data_len);

            column_metas.push_back(meta);
            serialized_data.push_back(std::move(data));

            required_size += sizeof(ColumnMeta) + meta.name_len + meta.data_len;
        }

        // 检查空间
        if (required_size > sizeof(block.data)) {
            return false;
        }

        // 写入元数据
        ArrowMetadata arrow_meta;
        arrow_meta.schema_id = schema_id;
        arrow_meta.num_rows = num_rows_;
        arrow_meta.num_columns = column_metas.size();
        arrow_meta.payload_size = required_size;
        arrow_meta.timestamp_ns = std::chrono::system_clock::now().time_since_epoch().count();

        size_t offset = 0;
        std::memcpy(block.data + offset, &arrow_meta, sizeof(arrow_meta));
        offset += sizeof(arrow_meta);

        // 写入列数据
        for (size_t i = 0; i < column_metas.size(); ++i) {
            std::memcpy(block.data + offset, &column_metas[i], sizeof(ColumnMeta));
            offset += sizeof(ColumnMeta);

            std::memcpy(block.data + offset, serialized_data[i].data(), serialized_data[i].size());
            offset += serialized_data[i].size();
        }

        // 设置block元数据
        block.data_type = 4;  // ArrowBatch
        block.length = offset;
        block.record_count = num_rows_;
        block.update_timestamp();

        return true;
    }

    /**
     * @brief 从 ZeroCopyMarketBlock 反序列化
     */
    static ArrowRecordBatch deserialize_from(const ZeroCopyMarketBlock& block) {
        ArrowRecordBatch batch;

        if (block.data_type != 4) {
            return batch;  // 类型不匹配
        }

        size_t offset = 0;

        // 读取元数据
        ArrowMetadata arrow_meta;
        std::memcpy(&arrow_meta, block.data + offset, sizeof(arrow_meta));
        offset += sizeof(arrow_meta);

        batch.num_rows_ = arrow_meta.num_rows;

        // 读取列数据
        struct ColumnMeta {
            uint8_t type;
            uint16_t name_len;
            uint32_t data_len;
        };

        for (uint32_t i = 0; i < arrow_meta.num_columns; ++i) {
            ColumnMeta meta;
            std::memcpy(&meta, block.data + offset, sizeof(meta));
            offset += sizeof(meta);

            // 读取列名
            std::string name(reinterpret_cast<const char*>(block.data + offset), meta.name_len);
            offset += meta.name_len;

            // 根据类型读取数据
            if (meta.type == 0) {  // int64
                size_t count = meta.data_len / sizeof(int64_t);
                std::vector<int64_t> data(count);
                std::memcpy(data.data(), block.data + offset, meta.data_len);
                batch.add_int64_column(name, data);
                offset += meta.data_len;
            } else if (meta.type == 1) {  // double
                size_t count = meta.data_len / sizeof(double);
                std::vector<double> data(count);
                std::memcpy(data.data(), block.data + offset, meta.data_len);
                batch.add_double_column(name, data);
                offset += meta.data_len;
            }
        }

        return batch;
    }

    size_t num_rows() const { return num_rows_; }
    size_t num_columns() const {
        return int64_columns_.size() + double_columns_.size();
    }

    const std::vector<ArrowColumn<int64_t>>& int64_columns() const { return int64_columns_; }
    const std::vector<ArrowColumn<double>>& double_columns() const { return double_columns_; }

private:
    size_t num_rows_;
    std::vector<ArrowColumn<int64_t>> int64_columns_;
    std::vector<ArrowColumn<double>> double_columns_;
    std::vector<ArrowColumn<std::array<char, 64>>> string_columns_;
};

/**
 * @brief Arrow Builder - 链式构建器
 */
class ArrowRecordBatchBuilder {
public:
    ArrowRecordBatchBuilder() = default;

    ArrowRecordBatchBuilder& add_int64(const std::string& name, const std::vector<int64_t>& data) {
        batch_.add_int64_column(name, data);
        return *this;
    }

    ArrowRecordBatchBuilder& add_double(const std::string& name, const std::vector<double>& data) {
        batch_.add_double_column(name, data);
        return *this;
    }

    ArrowRecordBatchBuilder& add_string(const std::string& name,
                                       const std::vector<std::string>& data,
                                       size_t max_len = 32) {
        batch_.add_string_column(name, data, max_len);
        return *this;
    }

    ArrowRecordBatch build() {
        return std::move(batch_);
    }

private:
    ArrowRecordBatch batch_;
};

} // namespace qaultra::ipc
