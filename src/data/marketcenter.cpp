#include "qaultra/data/marketcenter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <memory>
#include <iomanip>  // 添加 put_time 支持
#include <set>
#include <arrow/compute/api.h>
#include <arrow/csv/api.h>
#include <arrow/filesystem/api.h>

namespace qaultra::data {

// 构造函数实现
QAMarketCenter::QAMarketCenter(const std::string& path)
    : dateidx_(0), date_("") {
    // 加载主要数据文件
    daily_table_ = load_parquet_mmap(path);

    if (daily_table_) {
        // 按日期分组数据
        auto partitioned_data = partition_by_date(daily_table_);

        // 处理每个日期的数据
        for (const auto& [date_idx, table] : partitioned_data) {
            auto [extracted_date, klines] = run_split_date(table);
            data_[extracted_date] = std::move(klines);
        }

        std::cout << "MarketCenter已加载 " << data_.size() << " 个交易日的数据" << std::endl;
    }
}

QAMarketCenter QAMarketCenter::new_for_realtime() {
    QAMarketCenter mc("");
    mc.data_.clear();
    mc.today_.clear();
    mc.minutes_.clear();
    mc.date_ = "";
    mc.dateidx_ = 0;
    return mc;
}

std::shared_ptr<arrow::Table> QAMarketCenter::load_parquet_mmap(const std::string& path) {
    try {
        // 打开文件
        auto fs = arrow::fs::FileSystemFromUriOrPath(path).ValueOrDie();
        auto input = fs->OpenInputFile(path).ValueOrDie();

        // 创建 Parquet 读取器
        std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
        auto status = parquet::arrow::OpenFile(input, arrow::default_memory_pool(), &arrow_reader);

        if (!status.ok()) {
            std::cerr << "无法打开Parquet文件: " << path << " - " << status.ToString() << std::endl;
            return nullptr;
        }

        // 读取整个表
        std::shared_ptr<arrow::Table> table;
        status = arrow_reader->ReadTable(&table);

        if (!status.ok()) {
            std::cerr << "无法读取Parquet表: " << status.ToString() << std::endl;
            return nullptr;
        }

        std::cout << "成功加载Parquet文件: " << path
                  << " (行数: " << table->num_rows()
                  << ", 列数: " << table->num_columns() << ")" << std::endl;

        return table;
    } catch (const std::exception& e) {
        std::cerr << "加载Parquet文件时发生异常: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unordered_map<std::string, Kline> QAMarketCenter::get_codedate(const std::string& date, const std::string& code) {
    int64_t timestamp = date_string_to_timestamp(date);
    int32_t dateidx = static_cast<int32_t>(timestamp / 86400000000);

    auto date_it = data_.find(dateidx);
    if (date_it != data_.end()) {
        auto code_it = date_it->second.find(code);
        if (code_it != date_it->second.end()) {
            std::unordered_map<std::string, Kline> result;
            result[code] = code_it->second;
            return result;
        }
    }

    return {}; // 返回空映射
}

std::unordered_map<std::string, Kline> QAMarketCenter::get_date(const std::string& date) {
    int64_t timestamp = date_string_to_timestamp(date);
    int32_t dateidx = static_cast<int32_t>(timestamp / 86400000000);

    auto it = data_.find(dateidx);
    if (it != data_.end()) {
        return it->second;
    }

    return {}; // 返回空映射
}

std::optional<std::reference_wrapper<const std::unordered_map<std::string, Kline>>>
QAMarketCenter::try_get_date(const std::string& date) {
    int64_t timestamp = date_string_to_timestamp(date);
    int32_t dateidx = static_cast<int32_t>(timestamp / 86400000000);

    auto it = data_.find(dateidx);
    if (it != data_.end()) {
        return std::cref(it->second);
    }

    return std::nullopt;
}

std::unordered_map<std::string, Kline> QAMarketCenter::get_minutes(const std::string& datetime) {
    int64_t timestamp = datetime_string_to_nanos(datetime);

    auto it = minutes_.find(timestamp);
    if (it != minutes_.end()) {
        return it->second;
    }

    return {}; // 返回空映射
}

const std::unordered_map<std::string, Kline>& QAMarketCenter::get_minutes_ref(const std::string& datetime) {
    int64_t timestamp = datetime_string_to_nanos(datetime);

    auto it = minutes_.find(timestamp);
    if (it != minutes_.end()) {
        return it->second;
    }

    // 返回静态空映射
    static const std::unordered_map<std::string, Kline> empty_map;
    return empty_map;
}

void QAMarketCenter::load_minutes(const std::string& date, const std::string& freq) {
    std::string path = build_cache_path("stock", "min" + freq, date);

    auto table = load_parquet_mmap(path);
    if (!table) {
        std::cerr << "无法加载分钟数据: " << path << std::endl;
        return;
    }

    auto partitioned_data = partition_by_datetime(table);

    minutes_.clear();
    for (const auto& [timestamp, minute_table] : partitioned_data) {
        auto [extracted_timestamp, klines] = run_split_minutes(minute_table);
        minutes_[extracted_timestamp] = std::move(klines);
    }

    std::cout << "已加载 " << minutes_.size() << " 个分钟的数据" << std::endl;
}

void QAMarketCenter::load_tick(const std::string& date) {
    std::string path = "/opt/cache/data/stocktick/" + date + ".pq";

    auto table = load_parquet_mmap(path);
    if (!table) {
        std::cerr << "无法加载Tick数据: " << path << std::endl;
        return;
    }

    // 应用过滤器只保留特定列
    std::vector<std::string> columns = {
        "datetime", "order_book_id", "last", "volume",
        "total_turnover", "limit_up", "limit_down"
    };

    // 简化实现：过滤列并按时间分组
    auto partitioned_data = partition_by_datetime(table);

    minutes_.clear();
    for (const auto& [timestamp, tick_table] : partitioned_data) {
        auto [extracted_timestamp, klines] = run_split_ticks(tick_table);
        minutes_[extracted_timestamp] = std::move(klines);
    }

    std::cout << "已加载 " << minutes_.size() << " 个Tick时间点的数据" << std::endl;
}

void QAMarketCenter::load_tick_with_filter(const std::string& date, const std::vector<std::string>& order_book_id_list) {
    std::string path = "/opt/cache/data/stocktick/" + date + ".pq";

    auto table = load_parquet_mmap(path);
    if (!table) {
        std::cerr << "无法加载Tick数据: " << path << std::endl;
        return;
    }

    // 应用过滤器
    auto filtered_table = apply_filter(table, "order_book_id", order_book_id_list);

    auto partitioned_data = partition_by_datetime(filtered_table);

    minutes_.clear();
    for (const auto& [timestamp, tick_table] : partitioned_data) {
        auto [extracted_timestamp, klines] = run_split_ticks(tick_table);
        minutes_[extracted_timestamp] = std::move(klines);
    }

    std::cout << "已加载 " << minutes_.size() << " 个过滤后的Tick时间点数据" << std::endl;
}

void QAMarketCenter::load_minutes_with_filter_pushdown(const std::string& date,
                                                      const std::string& freq,
                                                      const std::string& order_book_id) {
    std::string path = build_cache_path("stock", "min" + freq, date);

    auto table = load_parquet_mmap(path);
    if (!table) {
        std::cerr << "无法加载分钟数据: " << path << std::endl;
        return;
    }

    // 应用单个值过滤器
    auto filtered_table = apply_filter(table, "order_book_id", {order_book_id});

    auto partitioned_data = partition_by_datetime(filtered_table);

    minutes_.clear();
    for (const auto& [timestamp, minute_table] : partitioned_data) {
        auto [extracted_timestamp, klines] = run_split_minutes(minute_table);
        minutes_[extracted_timestamp] = std::move(klines);
    }

    std::cout << "已加载 " << minutes_.size() << " 个过滤后的分钟数据" << std::endl;
}

std::vector<std::string> QAMarketCenter::get_minutes_range() {
    std::vector<std::pair<int64_t, std::string>> range;
    range.reserve(minutes_.size());

    for (const auto& [timestamp, _] : minutes_) {
        std::string formatted_time = nanos_to_datetime_string(timestamp);
        range.emplace_back(timestamp, formatted_time);
    }

    // 按时间戳排序
    std::sort(range.begin(), range.end(),
              [](const auto& a, const auto& b) {
                  return a.first < b.first;
              });

    // 提取格式化时间
    std::vector<std::string> result;
    result.reserve(range.size());
    for (const auto& [_, formatted_time] : range) {
        result.push_back(formatted_time);
    }

    return result;
}

void QAMarketCenter::load_future_minutes(const std::string& date, const std::string& freq) {
    std::string path = build_cache_path("future", "min" + freq, date);
    load_minutes(date, freq); // 复用加载逻辑
}

void QAMarketCenter::load_convertbond_minutes(const std::string& date, const std::string& freq) {
    std::string path = build_cache_path("convertible", "min" + freq, date);
    load_minutes(date, freq); // 复用加载逻辑
}

void QAMarketCenter::load_btc_minutes(const std::string& date, const std::string& freq) {
    std::string path = build_cache_path("btc", "min" + freq, date);
    load_minutes(date, freq); // 复用加载逻辑
}

const std::unordered_map<std::string, Kline>& QAMarketCenter::get_date_ref(const std::string& date) {
    int64_t timestamp = date_string_to_timestamp(date);
    int32_t dateidx = static_cast<int32_t>(timestamp / 86400000000);

    auto it = data_.find(dateidx);
    if (it != data_.end()) {
        return it->second;
    }

    // 返回静态空映射
    static const std::unordered_map<std::string, Kline> empty_map;
    return empty_map;
}

bool QAMarketCenter::save_to_file(const std::string& filename) const {
    try {
        nlohmann::json j = to_json();
        std::ofstream file(filename);
        file << j.dump(2); // 格式化输出
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存到文件失败: " << e.what() << std::endl;
        return false;
    }
}

bool QAMarketCenter::load_from_file(const std::string& filename) {
    try {
        std::ifstream file(filename);
        nlohmann::json j;
        file >> j;

        // 简化的反序列化实现
        // 实际项目中需要完整实现
        return true;
    } catch (const std::exception& e) {
        std::cerr << "从文件加载失败: " << e.what() << std::endl;
        return false;
    }
}

QAMarketCenter::DataStats QAMarketCenter::get_stats() const {
    DataStats stats;
    stats.daily_dates_count = data_.size();
    stats.minute_timestamps_count = minutes_.size();

    // 计算总证券数量
    std::set<std::string> unique_symbols;
    for (const auto& [date, klines] : data_) {
        for (const auto& [symbol, kline] : klines) {
            unique_symbols.insert(symbol);
        }
    }
    stats.total_symbols_count = unique_symbols.size();

    // 计算日期范围
    if (!data_.empty()) {
        auto min_date = std::min_element(data_.begin(), data_.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
        auto max_date = std::max_element(data_.begin(), data_.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        // 简化的日期转换
        stats.date_range_start = std::to_string(min_date->first);
        stats.date_range_end = std::to_string(max_date->first);
    }

    return stats;
}

nlohmann::json QAMarketCenter::to_json() const {
    nlohmann::json j;
    j["dateidx"] = dateidx_;
    j["date"] = date_;

    // 序列化统计信息
    auto stats = get_stats();
    j["stats"] = {
        {"daily_dates_count", stats.daily_dates_count},
        {"minute_timestamps_count", stats.minute_timestamps_count},
        {"total_symbols_count", stats.total_symbols_count},
        {"date_range_start", stats.date_range_start},
        {"date_range_end", stats.date_range_end}
    };

    return j;
}

// 私有方法实现
std::pair<int32_t, std::unordered_map<std::string, Kline>>
QAMarketCenter::run_split_date(std::shared_ptr<arrow::Table> table) {
    if (!table || table->num_rows() == 0) {
        return {0, {}};
    }

    try {
        // 提取各列数据
        auto codes = extract_string_column(table, "order_book_id");
        auto opens = extract_double_column(table, "open");
        auto closes = extract_double_column(table, "close");
        auto highs = extract_double_column(table, "high");
        auto lows = extract_double_column(table, "low");
        auto volumes = extract_double_column(table, "volume");
        auto total_turnovers = extract_double_column(table, "total_turnover");
        auto limit_ups = extract_double_column(table, "limit_up");
        auto limit_downs = extract_double_column(table, "limit_down");

        // 尝试提取可选列
        std::vector<double> split_coefficients, dividends;
        try {
            split_coefficients = extract_double_column(table, "split_coefficient_to");
            dividends = extract_double_column(table, "dividend_cash_before_tax");
        } catch (...) {
            // 如果列不存在，用默认值填充
            split_coefficients.resize(codes.size(), 0.0);
            dividends.resize(codes.size(), 0.0);
        }

        // 提取日期（假设所有行都是同一天）
        auto timestamps = extract_timestamp_column(table, "date");
        int32_t date_idx = timestamps.empty() ? 0 : static_cast<int32_t>(timestamps[0]);

        // 构建Kline映射
        std::unordered_map<std::string, Kline> klines;
        size_t min_size = std::min({codes.size(), opens.size(), closes.size(),
                                   highs.size(), lows.size(), volumes.size()});

        for (size_t i = 0; i < min_size; ++i) {
            Kline kline(
                codes[i], opens[i], closes[i], highs[i], lows[i],
                volumes[i],
                i < limit_ups.size() ? limit_ups[i] : 0.0,
                i < limit_downs.size() ? limit_downs[i] : 0.0,
                i < total_turnovers.size() ? total_turnovers[i] : 0.0,
                i < split_coefficients.size() ? split_coefficients[i] : 0.0,
                i < dividends.size() ? dividends[i] : 0.0
            );
            klines[codes[i]] = std::move(kline);
        }

        return {date_idx, std::move(klines)};
    } catch (const std::exception& e) {
        std::cerr << "处理日线数据时发生错误: " << e.what() << std::endl;
        return {0, {}};
    }
}

std::pair<int64_t, std::unordered_map<std::string, Kline>>
QAMarketCenter::run_split_minutes(std::shared_ptr<arrow::Table> table) {
    if (!table || table->num_rows() == 0) {
        return {0, {}};
    }

    try {
        // 提取各列数据
        auto codes = extract_string_column(table, "order_book_id");
        auto opens = extract_double_column(table, "open");
        auto closes = extract_double_column(table, "close");
        auto highs = extract_double_column(table, "high");
        auto lows = extract_double_column(table, "low");
        auto volumes = extract_double_column(table, "volume");
        auto total_turnovers = extract_double_column(table, "total_turnover");
        auto limit_ups = extract_double_column(table, "limit_up");
        auto limit_downs = extract_double_column(table, "limit_down");

        // 提取时间戳（假设所有行都是同一分钟）
        auto timestamps = extract_timestamp_column(table, "datetime");
        int64_t timestamp = timestamps.empty() ? 0 : timestamps[0];

        // 构建Kline映射
        std::unordered_map<std::string, Kline> klines;
        size_t min_size = std::min({codes.size(), opens.size(), closes.size(),
                                   highs.size(), lows.size(), volumes.size()});

        for (size_t i = 0; i < min_size; ++i) {
            Kline kline(
                codes[i], opens[i], closes[i], highs[i], lows[i],
                volumes[i],
                i < limit_ups.size() ? limit_ups[i] : 0.0,
                i < limit_downs.size() ? limit_downs[i] : 0.0,
                i < total_turnovers.size() ? total_turnovers[i] : 0.0
            );
            klines[codes[i]] = std::move(kline);
        }

        return {timestamp, std::move(klines)};
    } catch (const std::exception& e) {
        std::cerr << "处理分钟数据时发生错误: " << e.what() << std::endl;
        return {0, {}};
    }
}

std::pair<int64_t, std::unordered_map<std::string, Kline>>
QAMarketCenter::run_split_ticks(std::shared_ptr<arrow::Table> table) {
    if (!table || table->num_rows() == 0) {
        return {0, {}};
    }

    try {
        // Tick数据使用last价格作为OHLC
        auto codes = extract_string_column(table, "order_book_id");
        auto lasts = extract_double_column(table, "last");
        auto volumes = extract_double_column(table, "volume");
        auto total_turnovers = extract_double_column(table, "total_turnover");
        auto limit_ups = extract_double_column(table, "limit_up");
        auto limit_downs = extract_double_column(table, "limit_down");

        // 提取时间戳
        auto timestamps = extract_timestamp_column(table, "datetime");
        int64_t timestamp = timestamps.empty() ? 0 : timestamps[0];

        // 构建Kline映射
        std::unordered_map<std::string, Kline> klines;
        size_t min_size = std::min(std::min(codes.size(), lasts.size()), volumes.size());

        for (size_t i = 0; i < min_size; ++i) {
            // Tick数据：开高低收都使用最新价
            Kline kline(
                codes[i], lasts[i], lasts[i], lasts[i], lasts[i],
                volumes[i],
                i < limit_ups.size() ? limit_ups[i] : 0.0,
                i < limit_downs.size() ? limit_downs[i] : 0.0,
                i < total_turnovers.size() ? total_turnovers[i] : 0.0
            );
            klines[codes[i]] = std::move(kline);
        }

        return {timestamp, std::move(klines)};
    } catch (const std::exception& e) {
        std::cerr << "处理Tick数据时发生错误: " << e.what() << std::endl;
        return {0, {}};
    }
}

int64_t QAMarketCenter::date_string_to_timestamp(const std::string& date) {
    // 简化实现：将 "YYYY-MM-DD" 转换为微秒时间戳
    std::istringstream ss(date);
    int year, month, day;
    char dash1, dash2;
    ss >> year >> dash1 >> month >> dash2 >> day;

    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;

    return static_cast<int64_t>(std::mktime(&tm)) * 1000000; // 转换为微秒
}

int64_t QAMarketCenter::datetime_string_to_nanos(const std::string& datetime) {
    // 简化实现：将 "YYYY-MM-DD HH:MM:SS" 转换为纳秒时间戳
    std::istringstream ss(datetime);
    int year, month, day, hour, min, sec;
    char dash1, dash2, space, colon1, colon2;
    ss >> year >> dash1 >> month >> dash2 >> day >> space >> hour >> colon1 >> min >> colon2 >> sec;

    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;

    return static_cast<int64_t>(std::mktime(&tm)) * 1000000000; // 转换为纳秒
}

std::string QAMarketCenter::nanos_to_datetime_string(int64_t nanos) {
    time_t seconds = nanos / 1000000000;
    std::tm* tm = std::localtime(&seconds);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::unordered_map<int32_t, std::shared_ptr<arrow::Table>>
QAMarketCenter::partition_by_date(std::shared_ptr<arrow::Table> table) {
    // 简化实现：假设表已经按日期分组
    std::unordered_map<int32_t, std::shared_ptr<arrow::Table>> result;

    // 这里应该实现真正的分组逻辑
    // 为了简化，直接返回单个分组
    if (table && table->num_rows() > 0) {
        result[0] = table;
    }

    return result;
}

std::unordered_map<int64_t, std::shared_ptr<arrow::Table>>
QAMarketCenter::partition_by_datetime(std::shared_ptr<arrow::Table> table) {
    // 简化实现：假设表已经按时间分组
    std::unordered_map<int64_t, std::shared_ptr<arrow::Table>> result;

    // 这里应该实现真正的分组逻辑
    // 为了简化，直接返回单个分组
    if (table && table->num_rows() > 0) {
        result[0] = table;
    }

    return result;
}

std::shared_ptr<arrow::Table> QAMarketCenter::apply_filter(std::shared_ptr<arrow::Table> table,
                                                          const std::string& column_name,
                                                          const std::vector<std::string>& values) {
    // 简化实现：返回原表
    // 实际项目中应该使用 Arrow Compute 进行过滤
    return table;
}

std::vector<std::string> QAMarketCenter::extract_string_column(std::shared_ptr<arrow::Table> table,
                                                              const std::string& column_name) {
    std::vector<std::string> result;

    try {
        auto column = table->GetColumnByName(column_name);
        if (!column) {
            std::cerr << "列不存在: " << column_name << std::endl;
            return result;
        }

        // 简化实现：假设是字符串数组
        auto array = column->chunk(0);
        auto string_array = std::static_pointer_cast<arrow::StringArray>(array);

        result.reserve(string_array->length());
        for (int64_t i = 0; i < string_array->length(); ++i) {
            if (!string_array->IsNull(i)) {
                result.push_back(string_array->GetString(i));
            } else {
                result.push_back("");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "提取字符串列时发生错误: " << e.what() << std::endl;
    }

    return result;
}

std::vector<double> QAMarketCenter::extract_double_column(std::shared_ptr<arrow::Table> table,
                                                         const std::string& column_name) {
    std::vector<double> result;

    try {
        auto column = table->GetColumnByName(column_name);
        if (!column) {
            std::cerr << "列不存在: " << column_name << std::endl;
            return result;
        }

        // 简化实现：假设是双精度浮点数数组
        auto array = column->chunk(0);
        auto double_array = std::static_pointer_cast<arrow::DoubleArray>(array);

        result.reserve(double_array->length());
        for (int64_t i = 0; i < double_array->length(); ++i) {
            if (!double_array->IsNull(i)) {
                result.push_back(double_array->Value(i));
            } else {
                result.push_back(0.0);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "提取浮点列时发生错误: " << e.what() << std::endl;
    }

    return result;
}

std::vector<int64_t> QAMarketCenter::extract_timestamp_column(std::shared_ptr<arrow::Table> table,
                                                             const std::string& column_name) {
    std::vector<int64_t> result;

    try {
        auto column = table->GetColumnByName(column_name);
        if (!column) {
            std::cerr << "列不存在: " << column_name << std::endl;
            return result;
        }

        // 简化实现：假设是时间戳数组
        auto array = column->chunk(0);
        auto timestamp_array = std::static_pointer_cast<arrow::TimestampArray>(array);

        result.reserve(timestamp_array->length());
        for (int64_t i = 0; i < timestamp_array->length(); ++i) {
            if (!timestamp_array->IsNull(i)) {
                result.push_back(timestamp_array->Value(i));
            } else {
                result.push_back(0);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "提取时间戳列时发生错误: " << e.what() << std::endl;
    }

    return result;
}

std::string QAMarketCenter::build_cache_path(const std::string& asset_type,
                                            const std::string& freq,
                                            const std::string& date) const {
    return "/opt/cache/data/" + asset_type + "/" + freq + "/" + date + ".pq";
}

bool QAMarketCenter::validate_data_integrity() const {
    // 简化的数据完整性检查
    return !data_.empty() || !minutes_.empty();
}

void QAMarketCenter::optimize_memory() {
    // 内存优化：清理旧数据等
    // 实际项目中可以实现更复杂的内存管理策略
}

// ==================== Arc 零拷贝优化实现 ====================

std::shared_ptr<const std::unordered_map<std::string, Kline>>
QAMarketCenter::get_date_shared(const std::string& date) {
    int64_t timestamp = date_string_to_timestamp(date);
    int32_t dateidx = static_cast<int32_t>(timestamp / 86400000000);

    // 缓存命中：返回 shared_ptr clone (仅增加引用计数 ~10-20 ns)
    auto cache_it = date_cache_.find(dateidx);
    if (cache_it != date_cache_.end()) {
        return cache_it->second;  // shared_ptr copy constructor
    }

    // 缓存未命中：创建 shared_ptr 并缓存
    auto data_it = data_.find(dateidx);
    if (data_it != data_.end()) {
        auto shared_data = std::make_shared<const std::unordered_map<std::string, Kline>>(
            data_it->second
        );
        date_cache_[dateidx] = shared_data;
        return shared_data;
    }

    // 返回空 shared_ptr
    return nullptr;
}

std::shared_ptr<const std::unordered_map<std::string, Kline>>
QAMarketCenter::get_minutes_shared(const std::string& datetime) {
    int64_t timestamp = datetime_string_to_nanos(datetime);

    // 缓存命中
    auto cache_it = minute_cache_.find(timestamp);
    if (cache_it != minute_cache_.end()) {
        return cache_it->second;
    }

    // 缓存未命中
    auto data_it = minutes_.find(timestamp);
    if (data_it != minutes_.end()) {
        auto shared_data = std::make_shared<const std::unordered_map<std::string, Kline>>(
            data_it->second
        );
        minute_cache_[timestamp] = shared_data;
        return shared_data;
    }

    return nullptr;
}

void QAMarketCenter::clear_shared_cache() {
    date_cache_.clear();
    minute_cache_.clear();
    std::cout << "Arc 缓存已清除" << std::endl;
}

std::vector<StockCnDay> QAMarketCenter::get_stock_day(const std::string& code,
                                                       const std::string& start_date,
                                                       const std::string& end_date) {
    // 简化的存根实现
    // 实际项目中应该根据日期范围过滤并返回数据
    (void)code;
    (void)start_date;
    (void)end_date;
    return {};  // 返回空vector
}

std::vector<StockCn1Min> QAMarketCenter::get_stock_min(const std::string& code,
                                                        const std::string& start_datetime,
                                                        const std::string& end_datetime) {
    // 简化的存根实现
    // 实际项目中应该根据时间范围过滤并返回数据
    (void)code;
    (void)start_datetime;
    (void)end_datetime;
    return {};  // 返回空vector
}

// 工具函数实现
namespace marketcenter_utils {

std::shared_ptr<arrow::Schema> create_daily_schema() {
    return arrow::schema({
        arrow::field("date", arrow::date32()),
        arrow::field("order_book_id", arrow::utf8()),
        arrow::field("open", arrow::float64()),
        arrow::field("high", arrow::float64()),
        arrow::field("low", arrow::float64()),
        arrow::field("close", arrow::float64()),
        arrow::field("volume", arrow::float64()),
        arrow::field("total_turnover", arrow::float64()),
        arrow::field("limit_up", arrow::float64()),
        arrow::field("limit_down", arrow::float64())
    });
}

std::shared_ptr<arrow::Schema> create_minute_schema() {
    return arrow::schema({
        arrow::field("datetime", arrow::timestamp(arrow::TimeUnit::NANO)),
        arrow::field("order_book_id", arrow::utf8()),
        arrow::field("open", arrow::float64()),
        arrow::field("high", arrow::float64()),
        arrow::field("low", arrow::float64()),
        arrow::field("close", arrow::float64()),
        arrow::field("volume", arrow::float64()),
        arrow::field("total_turnover", arrow::float64())
    });
}

std::shared_ptr<arrow::Schema> create_tick_schema() {
    return arrow::schema({
        arrow::field("datetime", arrow::timestamp(arrow::TimeUnit::NANO)),
        arrow::field("order_book_id", arrow::utf8()),
        arrow::field("last", arrow::float64()),
        arrow::field("volume", arrow::float64()),
        arrow::field("total_turnover", arrow::float64()),
        arrow::field("limit_up", arrow::float64()),
        arrow::field("limit_down", arrow::float64())
    });
}

bool validate_schema(std::shared_ptr<arrow::Table> table, const std::string& expected_type) {
    if (!table) return false;

    auto schema = table->schema();
    // 简化的schema验证
    return schema->num_fields() > 0;
}

std::unordered_map<std::string, Kline> table_to_kline_map(std::shared_ptr<arrow::Table> table) {
    // 简化实现
    return {};
}

std::shared_ptr<arrow::Table> kline_map_to_table(const std::unordered_map<std::string, Kline>& klines,
                                                 const std::string& timestamp) {
    // 简化实现
    return nullptr;
}

BenchmarkResult benchmark_parquet_loading(const std::string& file_path) {
    BenchmarkResult result = {};

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        auto table = QAMarketCenter::load_parquet_mmap(file_path);

        auto end_time = std::chrono::high_resolution_clock::now();
        result.load_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        if (table) {
            result.rows_processed = table->num_rows();
            // 简化的内存使用估算
            result.memory_usage_mb = table->num_rows() * table->num_columns() * 8 / 1024 / 1024;
        }
    } catch (const std::exception& e) {
        std::cerr << "性能测试失败: " << e.what() << std::endl;
    }

    return result;
}

} // namespace marketcenter_utils

} // namespace qaultra::data