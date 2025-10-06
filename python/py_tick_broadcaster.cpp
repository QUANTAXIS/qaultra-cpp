// Python bindings for TickBroadcaster
//
// 高性能Tick数据广播器的Python接口
// Inspired by qars pybroadcast.rs pyo3 bindings

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "qaultra/data/tick_broadcaster.hpp"
#include "qaultra/protocol/mifi.hpp"

namespace py = pybind11;
using namespace qaultra::data;
using namespace qaultra::protocol::mifi;

// Python wrapper for Tick
void bind_tick(py::module& m) {
    py::class_<Tick>(m, "Tick")
        .def(py::init<>())
        .def_readwrite("instrument_id", &Tick::instrument_id)
        .def_readwrite("exchange_id", &Tick::exchange_id)
        .def_readwrite("datetime", &Tick::datetime)
        .def_readwrite("last_price", &Tick::last_price)
        .def_readwrite("volume", &Tick::volume)
        .def_readwrite("amount", &Tick::amount)
        .def_readwrite("open_interest", &Tick::open_interest)
        .def_readwrite("pre_close", &Tick::pre_close)
        .def_readwrite("open", &Tick::open)
        .def_readwrite("high", &Tick::high)
        .def_readwrite("low", &Tick::low)
        // Bid/ask are vectors
        .def_readwrite("bid_prices", &Tick::bid_prices)
        .def_readwrite("bid_volumes", &Tick::bid_volumes)
        .def_readwrite("ask_prices", &Tick::ask_prices)
        .def_readwrite("ask_volumes", &Tick::ask_volumes)
        // Helper methods
        .def("get_bid1", &Tick::get_bid1, "Get best bid price")
        .def("get_ask1", &Tick::get_ask1, "Get best ask price")
        .def("get_spread", &Tick::get_spread, "Get bid-ask spread")
        .def("get_mid_price", &Tick::get_mid_price, "Get mid price")
        .def("__repr__", [](const Tick& t) {
            return "Tick(id='" + t.instrument_id + "', datetime='" + t.datetime +
                   "', price=" + std::to_string(t.last_price) + ")";
        });
}

// Python wrapper for BroadcastStats
void bind_broadcast_stats(py::module& m) {
    py::class_<BroadcastStats>(m, "BroadcastStats")
        .def(py::init<>())
        .def_readwrite("total_ticks", &BroadcastStats::total_ticks)
        .def_readwrite("total_broadcasts", &BroadcastStats::total_broadcasts)
        .def_readwrite("cache_hits", &BroadcastStats::cache_hits)
        .def_readwrite("cache_misses", &BroadcastStats::cache_misses)
        .def_readwrite("total_latency_ns", &BroadcastStats::total_latency_ns)
        .def("avg_latency_ns", &BroadcastStats::avg_latency_ns,
             "Average latency in nanoseconds")
        .def("cache_hit_rate", &BroadcastStats::cache_hit_rate,
             "Cache hit rate (0.0 - 1.0)")
        .def("to_dict", [](const BroadcastStats& stats) {
            py::dict result;
            result["total_ticks"] = stats.total_ticks;
            result["total_broadcasts"] = stats.total_broadcasts;
            result["cache_hits"] = stats.cache_hits;
            result["cache_misses"] = stats.cache_misses;
            result["total_latency_ns"] = stats.total_latency_ns;
            result["avg_latency_ns"] = stats.avg_latency_ns();
            result["cache_hit_rate"] = stats.cache_hit_rate();
            return result;
        })
        .def("__repr__", [](const BroadcastStats& stats) {
            return "BroadcastStats(ticks=" + std::to_string(stats.total_ticks) +
                   ", broadcasts=" + std::to_string(stats.total_broadcasts) +
                   ", hit_rate=" + std::to_string(stats.cache_hit_rate() * 100) + "%)";
        });
}

// Python wrapper for Subscriber
void bind_subscriber(py::module& m) {
    py::class_<Subscriber>(m, "Subscriber")
        .def(py::init<const std::string&>(), py::arg("id"))
        .def_readwrite("id", &Subscriber::id)
        .def_readwrite("received_count", &Subscriber::received_count)
        .def("__repr__", [](const Subscriber& sub) {
            return "Subscriber(id='" + sub.id + "', received=" +
                   std::to_string(sub.received_count) + ")";
        });
}

// Python wrapper for TickBroadcaster
void bind_tick_broadcaster(py::module& m) {
    py::class_<TickBroadcaster>(m, "TickBroadcaster")
        .def(py::init([](QAMarketCenter& market) {
            // Move construct from QAMarketCenter
            // Note: This consumes the market object in Python
            return new TickBroadcaster(std::move(market));
        }), py::arg("market"),
        R"doc(
        Create TickBroadcaster with a market data center

        Args:
            market: QAMarketCenter instance (will be moved)

        Example:
            >>> market = QAMarketCenter.new_for_realtime()
            >>> broadcaster = TickBroadcaster(market)
            >>> broadcaster.register_subscriber("strategy_1")
        )doc")

        .def("register_subscriber", &TickBroadcaster::register_subscriber,
             py::arg("id"),
             "Register a new subscriber")

        .def("unregister_subscriber", &TickBroadcaster::unregister_subscriber,
             py::arg("id"),
             "Unregister a subscriber")

        .def("push_tick", &TickBroadcaster::push_tick,
             py::arg("date"), py::arg("tick"),
             R"doc(
             Push a single tick to all subscribers (零拷贝广播)

             Performance: ~3 ns per subscriber (with cache hit)

             Args:
                 date: Date string in format "YYYY-MM-DD"
                 tick: Tick data

             Example:
                 >>> tick = Tick()
                 >>> tick.instrument_id = "000001"
                 >>> tick.datetime = "2024-01-01 09:30:00"
                 >>> tick.last_price = 15.23
                 >>> broadcaster.push_tick("2024-01-01", tick)
             )doc")

        .def("push_batch",
            [](TickBroadcaster& self, const py::list& ticks) {
                // Convert Python list to C++ vector
                std::vector<Tick> tick_vec;
                for (auto item : ticks) {
                    tick_vec.push_back(item.cast<Tick>());
                }
                self.push_batch(tick_vec);
            },
            py::arg("ticks"),
            R"doc(
            Push a batch of ticks (automatically extracts dates)

            Args:
                ticks: List of Tick objects

            Example:
                >>> ticks = [create_tick("000001"), create_tick("000002")]
                >>> broadcaster.push_batch(ticks)
            )doc")

        .def("get_stats", &TickBroadcaster::get_stats,
             py::return_value_policy::reference,
             "Get broadcast statistics")

        .def("print_stats", &TickBroadcaster::print_stats,
             "Print statistics to console")

        .def("subscriber_count", &TickBroadcaster::subscriber_count,
             "Get number of active subscribers")

        .def("clear_cache", &TickBroadcaster::clear_cache,
             "Clear internal cache")

        .def("__repr__", [](const TickBroadcaster& self) {
            return "TickBroadcaster(subscribers=" +
                   std::to_string(self.subscriber_count()) + ")";
        });
}
