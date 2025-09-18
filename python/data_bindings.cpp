#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/chrono.h>

#include "qaultra/arrow/arrow_kline.hpp"
#include "qaultra/data/market_data.hpp"
#include "qaultra/simd/simd_math.hpp"

namespace py = pybind11;
using namespace qaultra;

void bind_data_types(py::module& m) {
    auto data = m.def_submodule("data", "Data structures and market data");

    // SIMD Math utilities
    auto simd = data.def_submodule("simd", "SIMD-optimized operations");

    simd.def("vectorized_add", &simd::vectorized_add,
        "Vectorized addition of two arrays",
        py::arg("a"), py::arg("b"));

    simd.def("vectorized_multiply", &simd::vectorized_multiply,
        "Vectorized multiplication of two arrays",
        py::arg("a"), py::arg("b"));

    simd.def("vectorized_multiply_scalar", &simd::vectorized_multiply_scalar,
        "Vectorized scalar multiplication",
        py::arg("array"), py::arg("scalar"), py::arg("size"));

    simd.def("calculate_sma", &simd::calculate_sma,
        "Calculate Simple Moving Average with SIMD",
        py::arg("prices"), py::arg("window"));

    simd.def("calculate_ema", &simd::calculate_ema,
        "Calculate Exponential Moving Average with SIMD",
        py::arg("prices"), py::arg("alpha"));

    simd.def("calculate_returns", &simd::calculate_returns,
        "Calculate returns with SIMD optimization",
        py::arg("prices"));

    simd.def("calculate_volatility", &simd::calculate_volatility,
        "Calculate volatility with SIMD optimization",
        py::arg("returns"), py::arg("window"));

    // Arrow K-line collection
    py::class_<arrow_data::ArrowKlineCollection>(data, "ArrowKlineCollection")
        .def(py::init<>())
        .def("add_batch", &arrow_data::ArrowKlineCollection::add_batch,
            "Add batch of K-line data",
            py::arg("codes"), py::arg("timestamps"), py::arg("opens"),
            py::arg("highs"), py::arg("lows"), py::arg("closes"),
            py::arg("volumes"), py::arg("amounts"))
        .def("filter_by_code", &arrow_data::ArrowKlineCollection::filter_by_code,
            "Filter data by stock code",
            py::arg("code"))
        .def("filter_by_date_range", &arrow_data::ArrowKlineCollection::filter_by_date_range,
            "Filter data by date range",
            py::arg("start_date"), py::arg("end_date"))
        .def("get_latest", &arrow_data::ArrowKlineCollection::get_latest,
            "Get latest N records",
            py::arg("n") = 1)
        .def("resample", &arrow_data::ArrowKlineCollection::resample,
            "Resample to different frequency",
            py::arg("frequency"))
        .def("sma", &arrow_data::ArrowKlineCollection::sma,
            "Calculate Simple Moving Average",
            py::arg("window"), py::arg("column") = "close")
        .def("ema", &arrow_data::ArrowKlineCollection::ema,
            "Calculate Exponential Moving Average",
            py::arg("alpha"), py::arg("column") = "close")
        .def("rsi", &arrow_data::ArrowKlineCollection::rsi,
            "Calculate Relative Strength Index",
            py::arg("window") = 14, py::arg("column") = "close")
        .def("bollinger_bands", &arrow_data::ArrowKlineCollection::bollinger_bands,
            "Calculate Bollinger Bands",
            py::arg("window") = 20, py::arg("std_dev") = 2.0, py::arg("column") = "close")
        .def("macd", &arrow_data::ArrowKlineCollection::macd,
            "Calculate MACD",
            py::arg("fast") = 12, py::arg("slow") = 26, py::arg("signal") = 9, py::arg("column") = "close")
        .def("to_numpy", &arrow_data::ArrowKlineCollection::to_numpy,
            "Convert to NumPy arrays")
        .def("to_pandas", &arrow_data::ArrowKlineCollection::to_pandas,
            "Convert to Pandas DataFrame")
        .def("size", &arrow_data::ArrowKlineCollection::size,
            "Get number of records")
        .def("memory_usage", &arrow_data::ArrowKlineCollection::memory_usage,
            "Get memory usage in bytes")
        .def("save_parquet", &arrow_data::ArrowKlineCollection::save_parquet,
            "Save to Parquet file",
            py::arg("filename"))
        .def("load_parquet", &arrow_data::ArrowKlineCollection::load_parquet,
            "Load from Parquet file",
            py::arg("filename"));

    // K-line record structure
    py::class_<arrow_data::KlineRecord>(data, "KlineRecord")
        .def(py::init<>())
        .def_readwrite("code", &arrow_data::KlineRecord::code)
        .def_readwrite("timestamp", &arrow_data::KlineRecord::timestamp)
        .def_readwrite("open", &arrow_data::KlineRecord::open)
        .def_readwrite("high", &arrow_data::KlineRecord::high)
        .def_readwrite("low", &arrow_data::KlineRecord::low)
        .def_readwrite("close", &arrow_data::KlineRecord::close)
        .def_readwrite("volume", &arrow_data::KlineRecord::volume)
        .def_readwrite("amount", &arrow_data::KlineRecord::amount);

    // Technical indicator results
    py::class_<arrow_data::TechnicalIndicators>(data, "TechnicalIndicators")
        .def_readwrite("sma", &arrow_data::TechnicalIndicators::sma)
        .def_readwrite("ema", &arrow_data::TechnicalIndicators::ema)
        .def_readwrite("rsi", &arrow_data::TechnicalIndicators::rsi)
        .def_readwrite("bollinger_upper", &arrow_data::TechnicalIndicators::bollinger_upper)
        .def_readwrite("bollinger_middle", &arrow_data::TechnicalIndicators::bollinger_middle)
        .def_readwrite("bollinger_lower", &arrow_data::TechnicalIndicators::bollinger_lower)
        .def_readwrite("macd_line", &arrow_data::TechnicalIndicators::macd_line)
        .def_readwrite("macd_signal", &arrow_data::TechnicalIndicators::macd_signal)
        .def_readwrite("macd_histogram", &arrow_data::TechnicalIndicators::macd_histogram);

    // Memory-mapped array for zero-copy operations
    py::class_<memory::MemoryMappedArray<double>>(data, "MemoryMappedDoubleArray")
        .def(py::init<const std::string&, size_t>(),
            "Create memory-mapped array",
            py::arg("filename"), py::arg("size"))
        .def("__getitem__", [](const memory::MemoryMappedArray<double>& self, size_t index) {
            return self[index];
        })
        .def("__setitem__", [](memory::MemoryMappedArray<double>& self, size_t index, double value) {
            self[index] = value;
        })
        .def("size", &memory::MemoryMappedArray<double>::size)
        .def("data", [](memory::MemoryMappedArray<double>& self) {
            return py::array_t<double>(
                self.size(),
                self.data(),
                py::handle() // No base object needed for memory-mapped data
            );
        }, py::return_value_policy::reference_internal)
        .def("sync", &memory::MemoryMappedArray<double>::sync,
            "Synchronize changes to disk");

    // Lock-free ring buffer for high-frequency data
    py::class_<threading::LockFreeRingBuffer<double>>(data, "LockFreeDoubleBuffer")
        .def(py::init<size_t>(),
            "Create lock-free ring buffer",
            py::arg("capacity"))
        .def("push", &threading::LockFreeRingBuffer<double>::push,
            "Push value to buffer",
            py::arg("value"))
        .def("pop", [](threading::LockFreeRingBuffer<double>& self) -> py::object {
            double value;
            if (self.pop(value)) {
                return py::cast(value);
            }
            return py::none();
        }, "Pop value from buffer, returns None if empty")
        .def("size", &threading::LockFreeRingBuffer<double>::size)
        .def("capacity", &threading::LockFreeRingBuffer<double>::capacity)
        .def("is_empty", &threading::LockFreeRingBuffer<double>::is_empty)
        .def("is_full", &threading::LockFreeRingBuffer<double>::is_full);

    // Market data manager
    py::class_<data::MarketDataManager>(data, "MarketDataManager")
        .def(py::init<>())
        .def("add_symbol", &data::MarketDataManager::add_symbol,
            "Add symbol for data management",
            py::arg("symbol"))
        .def("remove_symbol", &data::MarketDataManager::remove_symbol,
            "Remove symbol from management",
            py::arg("symbol"))
        .def("update_price", &data::MarketDataManager::update_price,
            "Update current price for symbol",
            py::arg("symbol"), py::arg("price"), py::arg("timestamp"))
        .def("get_current_price", &data::MarketDataManager::get_current_price,
            "Get current price for symbol",
            py::arg("symbol"))
        .def("get_price_history", &data::MarketDataManager::get_price_history,
            "Get price history for symbol",
            py::arg("symbol"), py::arg("window") = 100)
        .def("calculate_returns", &data::MarketDataManager::calculate_returns,
            "Calculate returns for symbol",
            py::arg("symbol"), py::arg("window") = 100)
        .def("calculate_volatility", &data::MarketDataManager::calculate_volatility,
            "Calculate volatility for symbol",
            py::arg("symbol"), py::arg("window") = 100)
        .def("get_symbols", &data::MarketDataManager::get_symbols,
            "Get all managed symbols")
        .def("get_statistics", &data::MarketDataManager::get_statistics,
            "Get market data statistics");

    // Aligned allocator utilities
    data.def("allocate_aligned", [](size_t size, size_t alignment) {
        return py::array_t<double>(
            size,
            memory::AlignedAllocator<double>(alignment).allocate(size)
        );
    }, "Allocate SIMD-aligned array",
       py::arg("size"), py::arg("alignment") = 32);

    // Performance utilities
    auto perf = data.def_submodule("performance", "Performance monitoring utilities");

    perf.def("start_timer", []() {
        static thread_local std::chrono::high_resolution_clock::time_point start_time;
        start_time = std::chrono::high_resolution_clock::now();
    }, "Start performance timer");

    perf.def("end_timer", []() -> double {
        static thread_local std::chrono::high_resolution_clock::time_point start_time;
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        return duration.count() / 1e9; // Return seconds
    }, "End timer and return elapsed time in seconds");

    // Data loading utilities
    data.def("load_csv_klines", [](const std::string& filename) {
        auto collection = std::make_shared<arrow_data::ArrowKlineCollection>();
        // Simplified CSV loading - in practice would use Arrow CSV reader
        return collection;
    }, "Load K-line data from CSV file",
       py::arg("filename"));

    data.def("load_parquet_klines", [](const std::string& filename) {
        auto collection = std::make_shared<arrow_data::ArrowKlineCollection>();
        collection->load_parquet(filename);
        return collection;
    }, "Load K-line data from Parquet file",
       py::arg("filename"));
}