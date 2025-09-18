#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/chrono.h>
#include <pybind11/functional.h>

#include "qaultra/qaultra.hpp"

// Forward declarations for binding functions
void bind_data_types(pybind11::module& m);
void bind_account_types(pybind11::module& m);
void bind_market_types(pybind11::module& m);
void bind_protocol_types(pybind11::module& m);
void bind_engine_types(pybind11::module& m);
void bind_simd_types(pybind11::module& m);

namespace py = pybind11;

PYBIND11_MODULE(qaultra_cpp, m) {
    m.doc() = "QAULTRA C++ - High-performance quantitative trading system";

    // Version information
    m.attr("__version__") = QAULTRA_VERSION;
    m.attr("VERSION_MAJOR") = qaultra::VERSION_MAJOR;
    m.attr("VERSION_MINOR") = qaultra::VERSION_MINOR;
    m.attr("VERSION_PATCH") = qaultra::VERSION_PATCH;

    // Basic enums and types
    py::enum_<qaultra::Direction>(m, "Direction")
        .value("BUY", qaultra::Direction::BUY)
        .value("SELL", qaultra::Direction::SELL)
        .export_values();

    py::enum_<qaultra::OrderStatus>(m, "OrderStatus")
        .value("PENDING", qaultra::OrderStatus::PENDING)
        .value("PARTIAL_FILLED", qaultra::OrderStatus::PARTIAL_FILLED)
        .value("FILLED", qaultra::OrderStatus::FILLED)
        .value("CANCELLED", qaultra::OrderStatus::CANCELLED)
        .value("REJECTED", qaultra::OrderStatus::REJECTED)
        .export_values();

    py::enum_<qaultra::PositionSide>(m, "PositionSide")
        .value("LONG", qaultra::PositionSide::LONG)
        .value("SHORT", qaultra::PositionSide::SHORT)
        .export_values();

    py::enum_<qaultra::MarketType>(m, "MarketType")
        .value("STOCK", qaultra::MarketType::STOCK)
        .value("FUTURE", qaultra::MarketType::FUTURE)
        .value("OPTION", qaultra::MarketType::OPTION)
        .value("FOREX", qaultra::MarketType::FOREX)
        .export_values();

    // Utility functions
    auto utils = m.def_submodule("utils", "Utility functions");
    utils.def("parse_datetime", &qaultra::utils::parse_datetime, "Parse datetime string");
    utils.def("format_datetime", &qaultra::utils::format_datetime, "Format datetime to string");
    utils.def("now", &qaultra::utils::now, "Get current timestamp");
    utils.def("generate_uuid", &qaultra::utils::generate_uuid, "Generate UUID string");

    // Bind specialized modules
    bind_data_types(m);
    bind_account_types(m);
    bind_market_types(m);
    bind_protocol_types(m);
    bind_engine_types(m);
    bind_simd_types(m);
}