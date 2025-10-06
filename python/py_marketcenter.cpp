// Python bindings for QAMarketCenter Arc zero-copy methods
//
// 为 Python 提供高性能市场数据访问接口
// Inspired by qars pybroadcast.rs pyo3 bindings

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "qaultra/data/marketcenter.hpp"
#include "qaultra/data/datatype.hpp"

namespace py = pybind11;
using namespace qaultra::data;

// Python wrapper for Kline data (from datatype.hpp)
void bind_kline(py::module& m) {
    py::class_<Kline>(m, "Kline")
        .def(py::init<>())
        .def_readwrite("order_book_id", &Kline::order_book_id)
        .def_readwrite("open", &Kline::open)
        .def_readwrite("high", &Kline::high)
        .def_readwrite("low", &Kline::low)
        .def_readwrite("close", &Kline::close)
        .def_readwrite("volume", &Kline::volume)
        .def_readwrite("total_turnover", &Kline::total_turnover)
        .def_readwrite("limit_up", &Kline::limit_up)
        .def_readwrite("limit_down", &Kline::limit_down)
        .def_readwrite("split_coefficient_to", &Kline::split_coefficient_to)
        .def_readwrite("dividend_cash_before_tax", &Kline::dividend_cash_before_tax)
        .def("__repr__", [](const Kline& k) {
            return "Kline(id='" + k.order_book_id + "', close=" + std::to_string(k.close) + ")";
        });
}

// Python wrapper for QAMarketCenter
void bind_marketcenter(py::module& m) {
    // DataStats struct
    py::class_<QAMarketCenter::DataStats>(m, "DataStats")
        .def(py::init<>())
        .def_readwrite("daily_dates_count", &QAMarketCenter::DataStats::daily_dates_count)
        .def_readwrite("minute_timestamps_count", &QAMarketCenter::DataStats::minute_timestamps_count)
        .def_readwrite("total_symbols_count", &QAMarketCenter::DataStats::total_symbols_count)
        .def_readwrite("date_range_start", &QAMarketCenter::DataStats::date_range_start)
        .def_readwrite("date_range_end", &QAMarketCenter::DataStats::date_range_end)
        .def("__repr__", [](const QAMarketCenter::DataStats& stats) {
            return "DataStats(daily_dates=" + std::to_string(stats.daily_dates_count) +
                   ", symbols=" + std::to_string(stats.total_symbols_count) +
                   ", range=" + stats.date_range_start + " to " + stats.date_range_end + ")";
        });

    // QAMarketCenter class
    py::class_<QAMarketCenter>(m, "QAMarketCenter")
        .def(py::init<const std::string&>(), py::arg("path"),
             "Create QAMarketCenter from data path")
        .def_static("new_for_realtime", &QAMarketCenter::new_for_realtime,
             "Create QAMarketCenter for realtime data")

        // Arc zero-copy methods (核心优化功能)
        .def("get_date_shared",
            [](QAMarketCenter& self, const std::string& date) -> py::dict {
                auto data_shared = self.get_date_shared(date);
                if (!data_shared) {
                    return py::dict();
                }

                // Convert shared_ptr<const unordered_map> to Python dict
                py::dict result;
                for (const auto& [code, kline] : *data_shared) {
                    result[py::str(code)] = kline;
                }
                return result;
            },
            py::arg("date"),
            R"doc(
            Get date data using Arc zero-copy sharing (零拷贝获取日期数据)

            Performance:
            - First access: ~43 μs (creates shared_ptr)
            - Cached access: ~61 ns (clones shared_ptr reference)

            Args:
                date: Date string in format "YYYY-MM-DD"

            Returns:
                dict: Map of stock codes to Kline data

            Example:
                >>> market = QAMarketCenter.new_for_realtime()
                >>> data = market.get_date_shared("2024-01-01")
                >>> print(data["000001"])
            )doc")

        .def("get_minutes_shared",
            [](QAMarketCenter& self, const std::string& datetime) -> py::dict {
                auto data_shared = self.get_minutes_shared(datetime);
                if (!data_shared) {
                    return py::dict();
                }

                py::dict result;
                for (const auto& [code, kline] : *data_shared) {
                    result[py::str(code)] = kline;
                }
                return result;
            },
            py::arg("datetime"),
            R"doc(
            Get minute data using Arc zero-copy sharing

            Args:
                datetime: Datetime string in format "YYYY-MM-DD HH:MM:SS"

            Returns:
                dict: Map of stock codes to Kline data
            )doc")

        .def("clear_shared_cache", &QAMarketCenter::clear_shared_cache,
             "Clear Arc zero-copy cache")

        // Traditional methods (传统方法，用于兼容)
        .def("get_date",
            [](QAMarketCenter& self, const std::string& date) -> py::dict {
                auto data = self.get_date(date);
                py::dict result;
                for (const auto& [code, kline] : data) {
                    result[py::str(code)] = kline;
                }
                return result;
            },
            py::arg("date"),
            "Get date data (deep copy, slower than get_date_shared)")

        .def("get_minutes",
            [](QAMarketCenter& self, const std::string& datetime) -> py::dict {
                auto data = self.get_minutes(datetime);
                py::dict result;
                for (const auto& [code, kline] : data) {
                    result[py::str(code)] = kline;
                }
                return result;
            },
            py::arg("datetime"),
            "Get minute data (deep copy)")

        // Utility methods
        .def("get_stats", &QAMarketCenter::get_stats,
             "Get data statistics")

        .def("save_to_file", &QAMarketCenter::save_to_file,
             py::arg("filename"),
             "Save data to file")

        .def("load_from_file", &QAMarketCenter::load_from_file,
             py::arg("filename"),
             "Load data from file")

        .def("__repr__", [](const QAMarketCenter& self) {
            auto stats = self.get_stats();
            return "QAMarketCenter(dates=" + std::to_string(stats.daily_dates_count) +
                   ", symbols=" + std::to_string(stats.total_symbols_count) + ")";
        });
}
