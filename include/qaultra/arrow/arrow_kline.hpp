#pragma once

#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/csv/api.h>
#include <arrow/ipc/api.h>
#include <arrow/filesystem/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

namespace qaultra::arrow_data {

/// High-performance K-line data storage using Apache Arrow
class ArrowKlineCollection {
private:
    std::shared_ptr<::arrow::Table> table_;
    std::shared_ptr<::arrow::Schema> schema_;

    // Column indices for fast access
    static constexpr int COL_CODE = 0;
    static constexpr int COL_DATETIME = 1;
    static constexpr int COL_OPEN = 2;
    static constexpr int COL_HIGH = 3;
    static constexpr int COL_LOW = 4;
    static constexpr int COL_CLOSE = 5;
    static constexpr int COL_VOLUME = 6;
    static constexpr int COL_AMOUNT = 7;

public:
    /// Default constructor
    ArrowKlineCollection();

    /// Constructor with initial capacity
    explicit ArrowKlineCollection(size_t initial_capacity);

    /// Constructor from existing Arrow table
    explicit ArrowKlineCollection(std::shared_ptr<::arrow::Table> table);

    /// Copy constructor (shares underlying data)
    ArrowKlineCollection(const ArrowKlineCollection&) = default;

    /// Move constructor
    ArrowKlineCollection(ArrowKlineCollection&&) = default;

    /// Assignment operators
    ArrowKlineCollection& operator=(const ArrowKlineCollection&) = default;
    ArrowKlineCollection& operator=(ArrowKlineCollection&&) = default;

    /// Destructor
    ~ArrowKlineCollection() = default;

    /// Get the underlying Arrow table
    std::shared_ptr<::arrow::Table> table() const { return table_; }

    /// Get the schema
    std::shared_ptr<::arrow::Schema> schema() const { return schema_; }

    /// Data access methods
    /// @{

    /// Get number of rows
    int64_t size() const;

    /// Check if empty
    bool empty() const;

    /// Get asset codes (zero-copy view)
    std::shared_ptr<::arrow::StringArray> codes() const;

    /// Get datetime values (zero-copy view)
    std::shared_ptr<::arrow::TimestampArray> datetimes() const;

    /// Get OHLCV data (zero-copy views)
    std::shared_ptr<::arrow::DoubleArray> opens() const;
    std::shared_ptr<::arrow::DoubleArray> highs() const;
    std::shared_ptr<::arrow::DoubleArray> lows() const;
    std::shared_ptr<::arrow::DoubleArray> closes() const;
    std::shared_ptr<::arrow::DoubleArray> volumes() const;
    std::shared_ptr<::arrow::DoubleArray> amounts() const;

    /// @}

    /// Data manipulation methods
    /// @{

    /// Add single row
    ::arrow::Status add_row(const std::string& code, int64_t timestamp,
                           double open, double high, double low,
                           double close, double volume, double amount);

    /// Add multiple rows from vectors
    ::arrow::Status add_batch(const std::vector<std::string>& codes,
                             const std::vector<int64_t>& timestamps,
                             const std::vector<double>& opens,
                             const std::vector<double>& highs,
                             const std::vector<double>& lows,
                             const std::vector<double>& closes,
                             const std::vector<double>& volumes,
                             const std::vector<double>& amounts);

    /// Append another collection
    ::arrow::Status append(const ArrowKlineCollection& other);

    /// Filter by asset code (zero-copy)
    ::arrow::Result<ArrowKlineCollection> filter_by_code(const std::string& code) const;

    /// Filter by time range (zero-copy)
    ::arrow::Result<ArrowKlineCollection> filter_by_time(int64_t start_timestamp,
                                                         int64_t end_timestamp) const;

    /// Sort by datetime
    ::arrow::Result<ArrowKlineCollection> sort_by_datetime() const;

    /// Take rows by indices (zero-copy)
    ::arrow::Result<ArrowKlineCollection> take(const std::vector<int64_t>& indices) const;

    /// Get slice of data (zero-copy)
    ::arrow::Result<ArrowKlineCollection> slice(int64_t offset, int64_t length) const;

    /// @}

    /// Analytical methods with SIMD acceleration
    /// @{

    /// Calculate typical prices ((H+L+C)/3)
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> typical_prices() const;

    /// Calculate weighted close prices ((H+L+C+C)/4)
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> weighted_close() const;

    /// Calculate price ranges (H-L)
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> price_ranges() const;

    /// Calculate body sizes (abs(C-O))
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> body_sizes() const;

    /// Calculate returns (price changes)
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> returns() const;

    /// Calculate log returns
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> log_returns() const;

    /// Calculate rolling statistics
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> rolling_mean(int window) const;
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> rolling_std(int window) const;
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> rolling_min(int window) const;
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> rolling_max(int window) const;

    /// Calculate technical indicators
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> sma(int period) const;
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> ema(int period) const;
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> rsi(int period = 14) const;

    /// Calculate Bollinger Bands
    struct BollingerBands {
        std::shared_ptr<::arrow::DoubleArray> upper;
        std::shared_ptr<::arrow::DoubleArray> middle;
        std::shared_ptr<::arrow::DoubleArray> lower;
    };
    ::arrow::Result<BollingerBands> bollinger_bands(int period = 20, double std_dev = 2.0) const;

    /// Calculate MACD
    struct MACD {
        std::shared_ptr<::arrow::DoubleArray> macd_line;
        std::shared_ptr<::arrow::DoubleArray> signal_line;
        std::shared_ptr<::arrow::DoubleArray> histogram;
    };
    ::arrow::Result<MACD> macd(int fast_period = 12, int slow_period = 26, int signal_period = 9) const;

    /// @}

    /// Aggregation methods
    /// @{

    /// Group by asset code and aggregate
    ::arrow::Result<ArrowKlineCollection> group_by_code() const;

    /// Resample to different timeframe
    ::arrow::Result<ArrowKlineCollection> resample(const std::string& frequency) const;

    /// Get summary statistics
    struct Statistics {
        double min_price;
        double max_price;
        double mean_price;
        double std_price;
        double total_volume;
        double total_amount;
        int64_t count;
    };
    ::arrow::Result<Statistics> get_statistics() const;

    /// @}

    /// I/O operations
    /// @{

    /// Save to Parquet file
    ::arrow::Status to_parquet(const std::string& filename) const;

    /// Load from Parquet file
    static ::arrow::Result<ArrowKlineCollection> from_parquet(const std::string& filename);

    /// Save to CSV file
    ::arrow::Status to_csv(const std::string& filename) const;

    /// Load from CSV file
    static ::arrow::Result<ArrowKlineCollection> from_csv(const std::string& filename);

    /// Save to Arrow IPC format
    ::arrow::Status to_arrow(const std::string& filename) const;

    /// Load from Arrow IPC format
    static ::arrow::Result<ArrowKlineCollection> from_arrow(const std::string& filename);

    /// Convert to JSON
    ::arrow::Result<std::string> to_json() const;

    /// Load from JSON
    static ::arrow::Result<ArrowKlineCollection> from_json(const std::string& json_str);

    /// @}

    /// Memory operations
    /// @{

    /// Get memory usage in bytes
    int64_t memory_usage() const;

    /// Compact the table (remove unused space)
    ::arrow::Status compact();

    /// Convert to record batches for streaming
    ::arrow::Result<std::vector<std::shared_ptr<::arrow::RecordBatch>>> to_batches(
        int64_t batch_size = 10000) const;

    /// @}

    /// Parallel operations
    /// @{

    /// Parallel filter with custom predicate
    template<typename Predicate>
    ::arrow::Result<ArrowKlineCollection> parallel_filter(Predicate pred) const;

    /// Parallel transform
    template<typename Transform>
    ::arrow::Result<std::shared_ptr<::arrow::DoubleArray>> parallel_transform(Transform trans) const;

    /// Parallel group-by operation
    ::arrow::Result<std::unordered_map<std::string, ArrowKlineCollection>> parallel_group_by_code() const;

    /// @}

private:
    /// Create the default schema
    static std::shared_ptr<::arrow::Schema> create_schema();

    /// Build table from arrays
    ::arrow::Status build_table(const std::vector<std::shared_ptr<::arrow::Array>>& arrays);

    /// Validate table structure
    ::arrow::Status validate() const;
};

/// High-performance market data manager using Arrow
class ArrowMarketDataManager {
private:
    std::unordered_map<std::string, ArrowKlineCollection> data_by_symbol_;
    std::shared_ptr<::arrow::MemoryPool> memory_pool_;

public:
    /// Constructor
    explicit ArrowMarketDataManager(std::shared_ptr<::arrow::MemoryPool> pool = nullptr);

    /// Add data for a symbol
    ::arrow::Status add_symbol_data(const std::string& symbol, ArrowKlineCollection data);

    /// Get data for a symbol
    const ArrowKlineCollection* get_symbol_data(const std::string& symbol) const;
    ArrowKlineCollection* get_symbol_data(const std::string& symbol);

    /// Get all symbols
    std::vector<std::string> get_symbols() const;

    /// Remove symbol data
    bool remove_symbol(const std::string& symbol);

    /// Clear all data
    void clear();

    /// Get combined data for multiple symbols
    ::arrow::Result<ArrowKlineCollection> get_combined_data(
        const std::vector<std::string>& symbols) const;

    /// Load data from directory (each file is a symbol)
    ::arrow::Status load_from_directory(const std::string& directory_path);

    /// Save all data to directory
    ::arrow::Status save_to_directory(const std::string& directory_path) const;

    /// Get memory usage
    int64_t total_memory_usage() const;

    /// Compact all data
    ::arrow::Status compact_all();

    /// Get statistics for all symbols
    std::unordered_map<std::string, ArrowKlineCollection::Statistics> get_all_statistics() const;
};

/// Factory functions for creating Arrow-based collections
namespace factory {

/// Create collection from legacy data structures
ArrowKlineCollection from_legacy_klines(const std::vector<struct LegacyKline>& klines);

/// Create empty collection with schema
ArrowKlineCollection create_empty(size_t initial_capacity = 1000);

/// Create collection from raw data arrays
::arrow::Result<ArrowKlineCollection> from_arrays(
    const std::vector<std::string>& codes,
    const std::vector<int64_t>& timestamps,
    const std::vector<double>& opens,
    const std::vector<double>& highs,
    const std::vector<double>& lows,
    const std::vector<double>& closes,
    const std::vector<double>& volumes,
    const std::vector<double>& amounts);

} // namespace factory

} // namespace qaultra::arrow_data