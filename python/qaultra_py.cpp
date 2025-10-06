// QAULTRA Python Bindings
//
// PyBind11 bindings for qaultra-cpp
// Similar to qars_core3 pyo3 module in qars Rust project
//
// Module: qaultra_py
// Version: 1.0.0
// Date: 2025-10-05

#include <pybind11/pybind11.h>

namespace py = pybind11;

// Forward declarations from other binding files
void bind_kline(py::module& m);
void bind_marketcenter(py::module& m);
void bind_tick(py::module& m);
void bind_broadcast_stats(py::module& m);
void bind_subscriber(py::module& m);
void bind_tick_broadcaster(py::module& m);

// Main Python module
PYBIND11_MODULE(qaultra_py, m) {
    m.doc() = R"doc(
        QAULTRA Python Bindings
        =======================

        High-performance quantitative trading library with Arc zero-copy optimization.

        Modules:
        --------
        - data: Market data center with Arc zero-copy sharing
        - broadcast: High-performance tick broadcasting system

        Performance Features:
        --------------------
        - Arc zero-copy data sharing (901x speedup vs deep copy)
        - Tick broadcasting: ~3 ns per subscriber
        - 99.9% cache hit rate
        - Support for 1000+ concurrent subscribers

        Example:
        --------
        >>> import qaultra_py
        >>>
        >>> # Create market data center
        >>> market = qaultra_py.QAMarketCenter.new_for_realtime()
        >>>
        >>> # Get data with Arc zero-copy (fast!)
        >>> data = market.get_date_shared("2024-01-01")
        >>>
        >>> # Create tick broadcaster
        >>> broadcaster = qaultra_py.TickBroadcaster(market)
        >>> broadcaster.register_subscriber("strategy_1")
        >>>
        >>> # Push ticks (zero-copy broadcast)
        >>> tick = qaultra_py.Tick()
        >>> tick.instrument_id = "000001"
        >>> tick.datetime = "2024-01-01 09:30:00"
        >>> tick.last_price = 15.23
        >>> broadcaster.push_tick("2024-01-01", tick)
        >>>
        >>> # Check stats
        >>> stats = broadcaster.get_stats()
        >>> print(f"Cache hit rate: {stats.cache_hit_rate() * 100:.1f}%")

        See Also:
        ---------
        - C++ Arc Optimization Guide: docs/CPP_ARC_OPTIMIZATION_GUIDE.md
        - Rust vs C++ Arc Comparison: docs/RUST_CPP_ARC_COMPARISON.md
        - Usage Guide: docs/CPP_ARC_USAGE_GUIDE.md
    )doc";

    // Module version
    m.attr("__version__") = "1.0.0";

    // Bind data types
    bind_kline(m);
    bind_tick(m);

    // Bind market data center
    bind_marketcenter(m);

    // Bind tick broadcasting
    bind_broadcast_stats(m);
    bind_subscriber(m);
    bind_tick_broadcaster(m);

    // Submodules (for organization, optional)
    auto data_module = m.def_submodule("data", "Market data module");
    data_module.doc() = "Market data center with Arc zero-copy optimization";

    auto broadcast_module = m.def_submodule("broadcast", "Tick broadcasting module");
    broadcast_module.doc() = "High-performance tick broadcasting system";
}
