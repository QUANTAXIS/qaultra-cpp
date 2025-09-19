#pragma once

#include "datatype.hpp"
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>

namespace qaultra::data {

/**
 * @brief 市场数据中心类 - 完全匹配Rust QAMarketCenter
 * 使用Apache Arrow替代Polars进行高性能数据处理
 */
class QAMarketCenter {
private:
    int32_t dateidx_;                                               // 日期索引
    std::string date_;                                              // 当前日期
    std::unordered_map<int32_t, std::unordered_map<std::string, Kline>> data_;    // 日线数据缓存
    std::unordered_map<std::string, Kline> today_;                  // 今日数据
    std::unordered_map<int64_t, std::unordered_map<std::string, Kline>> minutes_; // 分钟数据缓存

    // Arrow 数据缓存
    std::shared_ptr<arrow::Table> daily_table_;                     // 日线数据表
    std::shared_ptr<arrow::Table> minute_table_;                    // 分钟数据表
    std::shared_ptr<arrow::Table> tick_table_;                      // Tick数据表

public:
    /**
     * @brief 构造函数 - 匹配Rust new方法
     */
    explicit QAMarketCenter(const std::string& path);

    /**
     * @brief 实时数据构造函数 - 匹配Rust new_for_realtime方法
     */
    static QAMarketCenter new_for_realtime();

    /**
     * @brief 析构函数
     */
    ~QAMarketCenter() = default;

    /**
     * @brief 禁止拷贝，允许移动
     */
    QAMarketCenter(const QAMarketCenter&) = delete;
    QAMarketCenter& operator=(const QAMarketCenter&) = delete;
    QAMarketCenter(QAMarketCenter&&) = default;
    QAMarketCenter& operator=(QAMarketCenter&&) = default;

    /**
     * @brief 使用内存映射加载Parquet文件 - 匹配Rust load_parquet_mmap方法
     */
    static std::shared_ptr<arrow::Table> load_parquet_mmap(const std::string& path);

    /**
     * @brief 获取指定日期和代码的数据 - 匹配Rust get_codedate方法
     */
    std::unordered_map<std::string, Kline> get_codedate(const std::string& date, const std::string& code);

    /**
     * @brief 获取指定日期的所有数据 - 匹配Rust get_date方法
     */
    std::unordered_map<std::string, Kline> get_date(const std::string& date);

    /**
     * @brief 尝试获取指定日期的数据引用 - 匹配Rust try_get_date方法
     */
    std::optional<std::reference_wrapper<const std::unordered_map<std::string, Kline>>>
    try_get_date(const std::string& date);

    /**
     * @brief 获取指定时间的分钟数据 - 匹配Rust get_minutes方法
     */
    std::unordered_map<std::string, Kline> get_minutes(const std::string& datetime);

    /**
     * @brief 获取指定时间的分钟数据引用 - 匹配Rust get_minutes_ref方法
     */
    const std::unordered_map<std::string, Kline>& get_minutes_ref(const std::string& datetime);

    /**
     * @brief 加载分钟数据 - 匹配Rust load_minutes方法
     */
    void load_minutes(const std::string& date, const std::string& freq);

    /**
     * @brief 加载Tick数据 - 匹配Rust load_tick方法
     */
    void load_tick(const std::string& date);

    /**
     * @brief 使用过滤器加载Tick数据 - 匹配Rust load_tick_with_filter方法
     */
    void load_tick_with_filter(const std::string& date, const std::vector<std::string>& order_book_id_list);

    /**
     * @brief 使用下推过滤器加载分钟数据 - 匹配Rust load_minutes_with_filter_pushdown方法
     */
    void load_minutes_with_filter_pushdown(const std::string& date,
                                          const std::string& freq,
                                          const std::string& order_book_id);

    /**
     * @brief 获取分钟数据时间范围 - 匹配Rust get_minutes_range方法
     */
    std::vector<std::string> get_minutes_range();

    /**
     * @brief 加载期货分钟数据 - 匹配Rust load_future_minutes方法
     */
    void load_future_minutes(const std::string& date, const std::string& freq);

    /**
     * @brief 加载转债分钟数据 - 匹配Rust load_convertbond_minutes方法
     */
    void load_convertbond_minutes(const std::string& date, const std::string& freq);

    /**
     * @brief 加载比特币分钟数据 - 匹配Rust load_btc_minutes方法
     */
    void load_btc_minutes(const std::string& date, const std::string& freq);

    /**
     * @brief 获取指定日期数据引用 - 匹配Rust get_date_ref方法
     */
    const std::unordered_map<std::string, Kline>& get_date_ref(const std::string& date);

    /**
     * @brief 保存数据到文件
     */
    bool save_to_file(const std::string& filename) const;

    /**
     * @brief 从文件加载数据
     */
    bool load_from_file(const std::string& filename);

    /**
     * @brief 获取数据统计信息
     */
    struct DataStats {
        size_t daily_dates_count = 0;          // 日线数据日期数
        size_t minute_timestamps_count = 0;    // 分钟数据时间戳数
        size_t total_symbols_count = 0;        // 总证券数量
        std::string date_range_start;          // 日期范围开始
        std::string date_range_end;            // 日期范围结束
    };

    DataStats get_stats() const;

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;

private:
    /**
     * @brief 分割日线数据 - 匹配Rust run_split_date方法
     */
    static std::pair<int32_t, std::unordered_map<std::string, Kline>>
    run_split_date(std::shared_ptr<arrow::Table> table);

    /**
     * @brief 分割分钟数据 - 匹配Rust run_split_minutes方法
     */
    static std::pair<int64_t, std::unordered_map<std::string, Kline>>
    run_split_minutes(std::shared_ptr<arrow::Table> table);

    /**
     * @brief 分割Tick数据 - 匹配Rust run_split_ticks方法
     */
    static std::pair<int64_t, std::unordered_map<std::string, Kline>>
    run_split_ticks(std::shared_ptr<arrow::Table> table);

    /**
     * @brief 日期字符串转时间戳
     */
    static int64_t date_string_to_timestamp(const std::string& date);

    /**
     * @brief 时间字符串转纳秒时间戳
     */
    static int64_t datetime_string_to_nanos(const std::string& datetime);

    /**
     * @brief 纳秒时间戳转格式化字符串
     */
    static std::string nanos_to_datetime_string(int64_t nanos);

    /**
     * @brief 按日期分组Arrow表数据
     */
    std::unordered_map<int32_t, std::shared_ptr<arrow::Table>>
    partition_by_date(std::shared_ptr<arrow::Table> table);

    /**
     * @brief 按时间分组Arrow表数据
     */
    std::unordered_map<int64_t, std::shared_ptr<arrow::Table>>
    partition_by_datetime(std::shared_ptr<arrow::Table> table);

    /**
     * @brief 应用过滤器到Arrow表
     */
    std::shared_ptr<arrow::Table> apply_filter(std::shared_ptr<arrow::Table> table,
                                              const std::string& column_name,
                                              const std::vector<std::string>& values);

    /**
     * @brief 从Arrow表提取字符串列
     */
    std::vector<std::string> extract_string_column(std::shared_ptr<arrow::Table> table,
                                                   const std::string& column_name);

    /**
     * @brief 从Arrow表提取浮点列
     */
    std::vector<double> extract_double_column(std::shared_ptr<arrow::Table> table,
                                             const std::string& column_name);

    /**
     * @brief 从Arrow表提取时间戳列
     */
    std::vector<int64_t> extract_timestamp_column(std::shared_ptr<arrow::Table> table,
                                                  const std::string& column_name);

    /**
     * @brief 构建缓存路径
     */
    std::string build_cache_path(const std::string& asset_type,
                                const std::string& freq,
                                const std::string& date) const;

    /**
     * @brief 验证数据完整性
     */
    bool validate_data_integrity() const;

    /**
     * @brief 优化内存使用
     */
    void optimize_memory();
};

/**
 * @brief 数据处理工具函数命名空间
 */
namespace marketcenter_utils {
    /**
     * @brief 创建空的Arrow Schema用于不同数据类型
     */
    std::shared_ptr<arrow::Schema> create_daily_schema();
    std::shared_ptr<arrow::Schema> create_minute_schema();
    std::shared_ptr<arrow::Schema> create_tick_schema();

    /**
     * @brief 验证Arrow表的Schema
     */
    bool validate_schema(std::shared_ptr<arrow::Table> table, const std::string& expected_type);

    /**
     * @brief Arrow表转换为Kline映射
     */
    std::unordered_map<std::string, Kline> table_to_kline_map(std::shared_ptr<arrow::Table> table);

    /**
     * @brief Kline映射转换为Arrow表
     */
    std::shared_ptr<arrow::Table> kline_map_to_table(const std::unordered_map<std::string, Kline>& klines,
                                                     const std::string& timestamp);

    /**
     * @brief 性能测试函数
     */
    struct BenchmarkResult {
        std::chrono::microseconds load_time;      // 加载时间
        std::chrono::microseconds process_time;   // 处理时间
        size_t memory_usage_mb;                   // 内存使用量(MB)
        size_t rows_processed;                    // 处理行数
    };

    BenchmarkResult benchmark_parquet_loading(const std::string& file_path);
}

} // namespace qaultra::data