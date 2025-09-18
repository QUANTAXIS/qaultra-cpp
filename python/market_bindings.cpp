#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>

#include "qaultra/market/match_engine.hpp"
#include "qaultra/account/order.hpp"

namespace py = pybind11;
using namespace qaultra;

void bind_market_types(py::module& m) {
    auto market = m.def_submodule("market", "Market simulation and matching engine");

    // Order book depth level
    py::class_<market::OrderBook::DepthLevel>(market, "DepthLevel")
        .def(py::init<>())
        .def_readwrite("price", &market::OrderBook::DepthLevel::price)
        .def_readwrite("volume", &market::OrderBook::DepthLevel::volume)
        .def_readwrite("count", &market::OrderBook::DepthLevel::count)
        .def("__repr__", [](const market::OrderBook::DepthLevel& level) {
            return "DepthLevel(price=" + std::to_string(level.price) +
                   ", volume=" + std::to_string(level.volume) +
                   ", count=" + std::to_string(level.count) + ")";
        });

    // Order Book
    py::class_<market::OrderBook>(market, "OrderBook")
        .def(py::init<>())
        .def("add_order", &market::OrderBook::add_order,
            "Add order to book",
            py::arg("order"))
        .def("remove_order", &market::OrderBook::remove_order,
            "Remove order from book",
            py::arg("order_id"))
        .def("modify_order", &market::OrderBook::modify_order,
            "Modify existing order",
            py::arg("order_id"), py::arg("new_price"), py::arg("new_volume"))
        .def("get_order", &market::OrderBook::get_order,
            "Get order by ID",
            py::arg("order_id"))
        .def("get_best_bid", &market::OrderBook::get_best_bid,
            "Get best bid price and volume")
        .def("get_best_ask", &market::OrderBook::get_best_ask,
            "Get best ask price and volume")
        .def("get_spread", &market::OrderBook::get_spread,
            "Get bid-ask spread")
        .def("get_mid_price", &market::OrderBook::get_mid_price,
            "Get mid price")
        .def("get_last_trade", &market::OrderBook::get_last_trade,
            "Get last trade price and volume")
        .def("get_depth_bids", &market::OrderBook::get_depth_bids,
            "Get bid depth levels",
            py::arg("levels") = 10)
        .def("get_depth_asks", &market::OrderBook::get_depth_asks,
            "Get ask depth levels",
            py::arg("levels") = 10)
        .def("get_full_depth", &market::OrderBook::get_full_depth,
            "Get full market depth")
        .def("get_total_orders", &market::OrderBook::get_total_orders,
            "Get total number of orders")
        .def("get_total_volume", &market::OrderBook::get_total_volume,
            "Get total volume")
        .def("is_empty", &market::OrderBook::is_empty,
            "Check if order book is empty")
        .def("clear", &market::OrderBook::clear,
            "Clear all orders from book");

    // Trade Result
    py::class_<market::TradeResult>(market, "TradeResult")
        .def(py::init<>())
        .def_readwrite("trade_id", &market::TradeResult::trade_id)
        .def_readwrite("aggressive_order", &market::TradeResult::aggressive_order)
        .def_readwrite("passive_order", &market::TradeResult::passive_order)
        .def_readwrite("trade_price", &market::TradeResult::trade_price)
        .def_readwrite("trade_volume", &market::TradeResult::trade_volume)
        .def_readwrite("timestamp", &market::TradeResult::timestamp)
        .def("to_json", &market::TradeResult::to_json,
            "Convert to JSON representation")
        .def("__repr__", [](const market::TradeResult& trade) {
            return "TradeResult(id=" + trade.trade_id +
                   ", price=" + std::to_string(trade.trade_price) +
                   ", volume=" + std::to_string(trade.trade_volume) + ")";
        });

    // Matching Engine Statistics
    py::class_<market::MatchingEngine::EngineStats>(market, "EngineStats")
        .def(py::init<>())
        .def_readwrite("orders_processed", &market::MatchingEngine::EngineStats::orders_processed)
        .def_readwrite("trades_executed", &market::MatchingEngine::EngineStats::trades_executed)
        .def_readwrite("orders_rejected", &market::MatchingEngine::EngineStats::orders_rejected)
        .def_readwrite("active_symbols", &market::MatchingEngine::EngineStats::active_symbols)
        .def_readwrite("total_orders_in_book", &market::MatchingEngine::EngineStats::total_orders_in_book)
        .def_readwrite("avg_processing_time_ns", &market::MatchingEngine::EngineStats::avg_processing_time_ns)
        .def("__repr__", [](const market::MatchingEngine::EngineStats& stats) {
            return "EngineStats(processed=" + std::to_string(stats.orders_processed) +
                   ", executed=" + std::to_string(stats.trades_executed) +
                   ", rejected=" + std::to_string(stats.orders_rejected) + ")";
        });

    // Matching Engine
    py::class_<market::MatchingEngine>(market, "MatchingEngine")
        .def(py::init<size_t>(),
            "Create matching engine",
            py::arg("processing_threads") = 4)
        .def("add_trade_callback", &market::MatchingEngine::add_trade_callback,
            "Add callback for trade events",
            py::arg("callback"))
        .def("add_order_callback", &market::MatchingEngine::add_order_callback,
            "Add callback for order updates",
            py::arg("callback"))
        .def("submit_order", &market::MatchingEngine::submit_order,
            "Submit order for matching",
            py::arg("order"))
        .def("cancel_order", &market::MatchingEngine::cancel_order,
            "Cancel existing order",
            py::arg("symbol"), py::arg("order_id"))
        .def("modify_order", &market::MatchingEngine::modify_order,
            "Modify existing order",
            py::arg("symbol"), py::arg("order_id"), py::arg("new_price"), py::arg("new_volume"))
        .def("get_order_book", &market::MatchingEngine::get_order_book,
            "Get order book for symbol",
            py::arg("symbol"),
            py::return_value_policy::reference_internal)
        .def("get_best_bid_ask", &market::MatchingEngine::get_best_bid_ask,
            "Get best bid and ask prices",
            py::arg("symbol"))
        .def("get_market_depth", &market::MatchingEngine::get_market_depth,
            "Get market depth for symbol",
            py::arg("symbol"), py::arg("levels") = 10)
        .def("get_statistics", &market::MatchingEngine::get_statistics,
            "Get engine statistics")
        .def("start", &market::MatchingEngine::start,
            "Start order processing")
        .def("stop", &market::MatchingEngine::stop,
            "Stop order processing")
        .def("clear_all", &market::MatchingEngine::clear_all,
            "Clear all order books");

    // Market Data Feed Event
    py::class_<market::MarketDataFeed::MarketEvent>(market, "MarketEvent")
        .def(py::init<>())
        .def_readwrite("type", &market::MarketDataFeed::MarketEvent::type)
        .def_readwrite("symbol", &market::MarketDataFeed::MarketEvent::symbol)
        .def_readwrite("price", &market::MarketDataFeed::MarketEvent::price)
        .def_readwrite("volume", &market::MarketDataFeed::MarketEvent::volume)
        .def_readwrite("bid_price", &market::MarketDataFeed::MarketEvent::bid_price)
        .def_readwrite("ask_price", &market::MarketDataFeed::MarketEvent::ask_price)
        .def_readwrite("bid_volume", &market::MarketDataFeed::MarketEvent::bid_volume)
        .def_readwrite("ask_volume", &market::MarketDataFeed::MarketEvent::ask_volume)
        .def_readwrite("timestamp", &market::MarketDataFeed::MarketEvent::timestamp)
        .def_readwrite("metadata", &market::MarketDataFeed::MarketEvent::metadata);

    // Market Event Type enum
    py::enum_<market::MarketDataFeed::MarketEvent::Type>(market, "MarketEventType")
        .value("TRADE", market::MarketDataFeed::MarketEvent::Type::TRADE)
        .value("QUOTE", market::MarketDataFeed::MarketEvent::Type::QUOTE)
        .value("DEPTH_UPDATE", market::MarketDataFeed::MarketEvent::Type::DEPTH_UPDATE)
        .value("STATUS_CHANGE", market::MarketDataFeed::MarketEvent::Type::STATUS_CHANGE)
        .export_values();

    // Abstract Market Data Feed (interface)
    py::class_<market::MarketDataFeed>(market, "MarketDataFeed")
        .def("subscribe", &market::MarketDataFeed::subscribe,
            "Subscribe to symbol",
            py::arg("symbol"))
        .def("unsubscribe", &market::MarketDataFeed::unsubscribe,
            "Unsubscribe from symbol",
            py::arg("symbol"))
        .def("add_callback", &market::MarketDataFeed::add_callback,
            "Add event callback",
            py::arg("callback"))
        .def("start", &market::MarketDataFeed::start,
            "Start market data feed")
        .def("stop", &market::MarketDataFeed::stop,
            "Stop market data feed");

    // Market Simulator
    py::class_<market::MarketSimulator>(market, "MarketSimulator")
        .def(py::init<>())
        .def("load_market_data", &market::MarketSimulator::load_market_data,
            "Load market data from file",
            py::arg("filename"))
        .def("set_order_latency", &market::MarketSimulator::set_order_latency,
            "Set simulated order latency",
            py::arg("latency"))
        .def("set_trade_latency", &market::MarketSimulator::set_trade_latency,
            "Set simulated trade latency",
            py::arg("latency"))
        .def("get_matching_engine", &market::MarketSimulator::get_matching_engine,
            "Get underlying matching engine",
            py::return_value_policy::reference_internal)
        .def("run_simulation", &market::MarketSimulator::run_simulation,
            "Run complete simulation")
        .def("step_simulation", &market::MarketSimulator::step_simulation,
            "Step simulation by one tick")
        .def("reset_simulation", &market::MarketSimulator::reset_simulation,
            "Reset simulation to beginning")
        .def("get_current_time", &market::MarketSimulator::get_current_time,
            "Get current simulation time");

    // Factory functions
    auto factory = market.def_submodule("factory", "Factory functions for market components");

    factory.def("create_matching_engine", &market::factory::create_matching_engine,
        "Create matching engine with specified thread count",
        py::arg("thread_count") = std::thread::hardware_concurrency());

    factory.def("create_market_simulator", &market::factory::create_market_simulator,
        "Create market simulator");

    factory.def("create_order_book", &market::factory::create_order_book,
        "Create order book");

    // Utility functions for market analysis
    market.def("calculate_vwap", [](const std::vector<double>& prices, const std::vector<double>& volumes) {
        if (prices.size() != volumes.size() || prices.empty()) {
            return 0.0;
        }

        double total_value = 0.0;
        double total_volume = 0.0;

        for (size_t i = 0; i < prices.size(); ++i) {
            total_value += prices[i] * volumes[i];
            total_volume += volumes[i];
        }

        return total_volume > 0 ? total_value / total_volume : 0.0;
    }, "Calculate Volume Weighted Average Price",
       py::arg("prices"), py::arg("volumes"));

    market.def("calculate_twap", [](const std::vector<double>& prices) {
        if (prices.empty()) {
            return 0.0;
        }

        double sum = 0.0;
        for (double price : prices) {
            sum += price;
        }

        return sum / prices.size();
    }, "Calculate Time Weighted Average Price",
       py::arg("prices"));

    market.def("calculate_order_book_imbalance", [](const std::vector<market::OrderBook::DepthLevel>& bids,
                                                   const std::vector<market::OrderBook::DepthLevel>& asks) {
        double bid_volume = 0.0;
        double ask_volume = 0.0;

        for (const auto& bid : bids) {
            bid_volume += bid.volume;
        }

        for (const auto& ask : asks) {
            ask_volume += ask.volume;
        }

        double total_volume = bid_volume + ask_volume;
        return total_volume > 0 ? (bid_volume - ask_volume) / total_volume : 0.0;
    }, "Calculate order book imbalance",
       py::arg("bids"), py::arg("asks"));

    // Performance monitoring decorators
    market.def("monitor_matching_performance", [](market::MatchingEngine& engine) {
        return [&engine](py::function func) {
            return [&engine, func](py::args args, py::kwargs kwargs) {
                auto start = std::chrono::high_resolution_clock::now();
                auto result = func(*args, **kwargs);
                auto end = std::chrono::high_resolution_clock::now();

                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                // Could store performance metrics in engine

                return result;
            };
        };
    }, "Create performance monitoring decorator for matching engine functions",
       py::arg("engine"));

    // Market data generators for testing
    market.def("generate_random_walk", [](double initial_price, size_t steps, double volatility) {
        std::vector<double> prices;
        prices.reserve(steps);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> dist(0.0, volatility);

        double current_price = initial_price;
        prices.push_back(current_price);

        for (size_t i = 1; i < steps; ++i) {
            double change = dist(gen);
            current_price *= (1.0 + change);
            prices.push_back(current_price);
        }

        return prices;
    }, "Generate random walk price series for testing",
       py::arg("initial_price"), py::arg("steps"), py::arg("volatility") = 0.01);

    market.def("generate_synthetic_orderbook", [](const std::string& symbol, double mid_price, double spread, size_t levels) {
        auto book = std::make_unique<market::OrderBook>();

        for (size_t i = 0; i < levels; ++i) {
            // Create bid orders
            double bid_price = mid_price - spread/2 - i * 0.01;
            auto bid_order = std::make_shared<account::Order>();
            bid_order->order_id = "BID_" + std::to_string(i);
            bid_order->code = symbol;
            bid_order->volume = 100.0 * (i + 1);
            bid_order->price = bid_price;
            bid_order->direction = account::Direction::BUY;
            bid_order->status = account::OrderStatus::PENDING;
            book->add_order(bid_order);

            // Create ask orders
            double ask_price = mid_price + spread/2 + i * 0.01;
            auto ask_order = std::make_shared<account::Order>();
            ask_order->order_id = "ASK_" + std::to_string(i);
            ask_order->code = symbol;
            ask_order->volume = 100.0 * (i + 1);
            ask_order->price = ask_price;
            ask_order->direction = account::Direction::SELL;
            ask_order->status = account::OrderStatus::PENDING;
            book->add_order(ask_order);
        }

        return book;
    }, "Generate synthetic order book for testing",
       py::arg("symbol"), py::arg("mid_price"), py::arg("spread") = 0.02, py::arg("levels") = 10);
}