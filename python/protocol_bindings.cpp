#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>

#include "qaultra/protocol/qifi.hpp"

namespace py = pybind11;
using namespace qaultra;

void bind_protocol_types(py::module& m) {
    auto protocol = m.def_submodule("protocol", "Trading protocols (QIFI/MIFI/TIFI)");

    // QIFI Position
    py::class_<protocol::QIFIPosition>(protocol, "QIFIPosition")
        .def(py::init<>())
        .def_readwrite("code", &protocol::QIFIPosition::code)
        .def_readwrite("volume_long", &protocol::QIFIPosition::volume_long)
        .def_readwrite("volume_short", &protocol::QIFIPosition::volume_short)
        .def_readwrite("price", &protocol::QIFIPosition::price)
        .def_readwrite("cost_long", &protocol::QIFIPosition::cost_long)
        .def_readwrite("cost_short", &protocol::QIFIPosition::cost_short)
        .def_readwrite("market_value", &protocol::QIFIPosition::market_value)
        .def_readwrite("float_profit", &protocol::QIFIPosition::float_profit)
        .def_readwrite("last_datetime", &protocol::QIFIPosition::last_datetime)
        .def("to_json", &protocol::QIFIPosition::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::QIFIPosition::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("__repr__", [](const protocol::QIFIPosition& pos) {
            return "QIFIPosition(code=" + pos.code +
                   ", long=" + std::to_string(pos.volume_long) +
                   ", short=" + std::to_string(pos.volume_short) +
                   ", price=" + std::to_string(pos.price) + ")";
        });

    // QIFI Order
    py::class_<protocol::QIFIOrder>(protocol, "QIFIOrder")
        .def(py::init<>())
        .def_readwrite("order_id", &protocol::QIFIOrder::order_id)
        .def_readwrite("code", &protocol::QIFIOrder::code)
        .def_readwrite("volume", &protocol::QIFIOrder::volume)
        .def_readwrite("price", &protocol::QIFIOrder::price)
        .def_readwrite("direction", &protocol::QIFIOrder::direction)
        .def_readwrite("status", &protocol::QIFIOrder::status)
        .def_readwrite("datetime", &protocol::QIFIOrder::datetime)
        .def_readwrite("trade_volume", &protocol::QIFIOrder::trade_volume)
        .def_readwrite("trade_price", &protocol::QIFIOrder::trade_price)
        .def_readwrite("commission", &protocol::QIFIOrder::commission)
        .def("to_json", &protocol::QIFIOrder::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::QIFIOrder::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("__repr__", [](const protocol::QIFIOrder& order) {
            return "QIFIOrder(id=" + order.order_id +
                   ", code=" + order.code +
                   ", volume=" + std::to_string(order.volume) +
                   ", price=" + std::to_string(order.price) + ")";
        });

    // QIFI Trade
    py::class_<protocol::QIFITrade>(protocol, "QIFITrade")
        .def(py::init<>())
        .def_readwrite("trade_id", &protocol::QIFITrade::trade_id)
        .def_readwrite("order_id", &protocol::QIFITrade::order_id)
        .def_readwrite("code", &protocol::QIFITrade::code)
        .def_readwrite("volume", &protocol::QIFITrade::volume)
        .def_readwrite("price", &protocol::QIFITrade::price)
        .def_readwrite("direction", &protocol::QIFITrade::direction)
        .def_readwrite("datetime", &protocol::QIFITrade::datetime)
        .def_readwrite("commission", &protocol::QIFITrade::commission)
        .def("to_json", &protocol::QIFITrade::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::QIFITrade::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("__repr__", [](const protocol::QIFITrade& trade) {
            return "QIFITrade(id=" + trade.trade_id +
                   ", code=" + trade.code +
                   ", volume=" + std::to_string(trade.volume) +
                   ", price=" + std::to_string(trade.price) + ")";
        });

    // QIFI Account
    py::class_<protocol::QIFIAccount>(protocol, "QIFIAccount")
        .def(py::init<>())
        .def_readwrite("account_cookie", &protocol::QIFIAccount::account_cookie)
        .def_readwrite("portfolio_cookie", &protocol::QIFIAccount::portfolio_cookie)
        .def_readwrite("user_cookie", &protocol::QIFIAccount::user_cookie)
        .def_readwrite("init_cash", &protocol::QIFIAccount::init_cash)
        .def_readwrite("cash", &protocol::QIFIAccount::cash)
        .def_readwrite("frozen_cash", &protocol::QIFIAccount::frozen_cash)
        .def_readwrite("balance", &protocol::QIFIAccount::balance)
        .def_readwrite("market_value", &protocol::QIFIAccount::market_value)
        .def_readwrite("float_profit", &protocol::QIFIAccount::float_profit)
        .def_readwrite("total_value", &protocol::QIFIAccount::total_value)
        .def_readwrite("positions", &protocol::QIFIAccount::positions)
        .def_readwrite("orders", &protocol::QIFIAccount::orders)
        .def_readwrite("trades", &protocol::QIFIAccount::trades)
        .def_readwrite("timestamp", &protocol::QIFIAccount::timestamp)
        .def("to_json", &protocol::QIFIAccount::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::QIFIAccount::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("save_to_file", &protocol::QIFIAccount::save_to_file,
            "Save account data to file",
            py::arg("filename"))
        .def("load_from_file", &protocol::QIFIAccount::load_from_file,
            "Load account data from file",
            py::arg("filename"))
        .def("__repr__", [](const protocol::QIFIAccount& account) {
            return "QIFIAccount(cookie=" + account.account_cookie +
                   ", cash=" + std::to_string(account.cash) +
                   ", total_value=" + std::to_string(account.total_value) + ")";
        });

    // QIFI Portfolio
    py::class_<protocol::QIFIPortfolio>(protocol, "QIFIPortfolio")
        .def(py::init<>())
        .def_readwrite("portfolio_cookie", &protocol::QIFIPortfolio::portfolio_cookie)
        .def_readwrite("user_cookie", &protocol::QIFIPortfolio::user_cookie)
        .def_readwrite("accounts", &protocol::QIFIPortfolio::accounts)
        .def_readwrite("total_value", &protocol::QIFIPortfolio::total_value)
        .def_readwrite("total_cash", &protocol::QIFIPortfolio::total_cash)
        .def_readwrite("total_market_value", &protocol::QIFIPortfolio::total_market_value)
        .def_readwrite("total_float_profit", &protocol::QIFIPortfolio::total_float_profit)
        .def_readwrite("timestamp", &protocol::QIFIPortfolio::timestamp)
        .def("to_json", &protocol::QIFIPortfolio::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::QIFIPortfolio::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("add_account", &protocol::QIFIPortfolio::add_account,
            "Add account to portfolio",
            py::arg("account"))
        .def("remove_account", &protocol::QIFIPortfolio::remove_account,
            "Remove account from portfolio",
            py::arg("account_cookie"))
        .def("get_account", &protocol::QIFIPortfolio::get_account,
            "Get account by cookie",
            py::arg("account_cookie"))
        .def("calculate_totals", &protocol::QIFIPortfolio::calculate_totals,
            "Recalculate portfolio totals")
        .def("__repr__", [](const protocol::QIFIPortfolio& portfolio) {
            return "QIFIPortfolio(cookie=" + portfolio.portfolio_cookie +
                   ", accounts=" + std::to_string(portfolio.accounts.size()) +
                   ", total_value=" + std::to_string(portfolio.total_value) + ")";
        });

    // MIFI Market Data
    py::class_<protocol::MIFITick>(protocol, "MIFITick")
        .def(py::init<>())
        .def_readwrite("code", &protocol::MIFITick::code)
        .def_readwrite("datetime", &protocol::MIFITick::datetime)
        .def_readwrite("price", &protocol::MIFITick::price)
        .def_readwrite("volume", &protocol::MIFITick::volume)
        .def_readwrite("bid_price", &protocol::MIFITick::bid_price)
        .def_readwrite("ask_price", &protocol::MIFITick::ask_price)
        .def_readwrite("bid_volume", &protocol::MIFITick::bid_volume)
        .def_readwrite("ask_volume", &protocol::MIFITick::ask_volume)
        .def("to_json", &protocol::MIFITick::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::MIFITick::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("__repr__", [](const protocol::MIFITick& tick) {
            return "MIFITick(code=" + tick.code +
                   ", price=" + std::to_string(tick.price) +
                   ", volume=" + std::to_string(tick.volume) + ")";
        });

    py::class_<protocol::MIFIKline>(protocol, "MIFIKline")
        .def(py::init<>())
        .def_readwrite("code", &protocol::MIFIKline::code)
        .def_readwrite("datetime", &protocol::MIFIKline::datetime)
        .def_readwrite("open", &protocol::MIFIKline::open)
        .def_readwrite("high", &protocol::MIFIKline::high)
        .def_readwrite("low", &protocol::MIFIKline::low)
        .def_readwrite("close", &protocol::MIFIKline::close)
        .def_readwrite("volume", &protocol::MIFIKline::volume)
        .def_readwrite("amount", &protocol::MIFIKline::amount)
        .def("to_json", &protocol::MIFIKline::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::MIFIKline::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("__repr__", [](const protocol::MIFIKline& kline) {
            return "MIFIKline(code=" + kline.code +
                   ", open=" + std::to_string(kline.open) +
                   ", close=" + std::to_string(kline.close) + ")";
        });

    // TIFI Trading Information
    py::class_<protocol::TIFIMessage>(protocol, "TIFIMessage")
        .def(py::init<>())
        .def_readwrite("message_type", &protocol::TIFIMessage::message_type)
        .def_readwrite("source", &protocol::TIFIMessage::source)
        .def_readwrite("destination", &protocol::TIFIMessage::destination)
        .def_readwrite("timestamp", &protocol::TIFIMessage::timestamp)
        .def_readwrite("sequence_number", &protocol::TIFIMessage::sequence_number)
        .def_readwrite("data", &protocol::TIFIMessage::data)
        .def("to_json", &protocol::TIFIMessage::to_json,
            "Convert to JSON representation")
        .def("from_json", &protocol::TIFIMessage::from_json,
            "Create from JSON",
            py::arg("json_obj"))
        .def("__repr__", [](const protocol::TIFIMessage& msg) {
            return "TIFIMessage(type=" + msg.message_type +
                   ", source=" + msg.source +
                   ", dest=" + msg.destination + ")";
        });

    // Protocol utilities
    auto utils = protocol.def_submodule("utils", "Protocol utility functions");

    utils.def("validate_qifi_account", [](const protocol::QIFIAccount& account) {
        // Basic validation logic
        return !account.account_cookie.empty() &&
               !account.portfolio_cookie.empty() &&
               !account.user_cookie.empty() &&
               account.init_cash > 0;
    }, "Validate QIFI account data",
       py::arg("account"));

    utils.def("calculate_portfolio_metrics", [](const protocol::QIFIPortfolio& portfolio) {
        py::dict metrics;

        double total_pnl = 0.0;
        double total_cash = 0.0;
        double total_market_value = 0.0;
        size_t total_positions = 0;
        size_t total_orders = 0;
        size_t total_trades = 0;

        for (const auto& account : portfolio.accounts) {
            total_pnl += account.float_profit;
            total_cash += account.cash;
            total_market_value += account.market_value;
            total_positions += account.positions.size();
            total_orders += account.orders.size();
            total_trades += account.trades.size();
        }

        metrics["total_pnl"] = total_pnl;
        metrics["total_cash"] = total_cash;
        metrics["total_market_value"] = total_market_value;
        metrics["total_value"] = total_cash + total_market_value;
        metrics["account_count"] = portfolio.accounts.size();
        metrics["total_positions"] = total_positions;
        metrics["total_orders"] = total_orders;
        metrics["total_trades"] = total_trades;

        return metrics;
    }, "Calculate portfolio performance metrics",
       py::arg("portfolio"));

    utils.def("merge_qifi_accounts", [](const std::vector<protocol::QIFIAccount>& accounts) {
        if (accounts.empty()) {
            return protocol::QIFIAccount{};
        }

        protocol::QIFIAccount merged = accounts[0];
        merged.account_cookie = "MERGED_ACCOUNT";

        for (size_t i = 1; i < accounts.size(); ++i) {
            const auto& account = accounts[i];

            merged.cash += account.cash;
            merged.frozen_cash += account.frozen_cash;
            merged.balance += account.balance;
            merged.market_value += account.market_value;
            merged.float_profit += account.float_profit;
            merged.total_value += account.total_value;

            // Merge positions (aggregate by code)
            std::map<std::string, protocol::QIFIPosition> position_map;
            for (const auto& pos : merged.positions) {
                position_map[pos.code] = pos;
            }

            for (const auto& pos : account.positions) {
                if (position_map.find(pos.code) != position_map.end()) {
                    auto& existing = position_map[pos.code];
                    existing.volume_long += pos.volume_long;
                    existing.volume_short += pos.volume_short;
                    existing.cost_long += pos.cost_long;
                    existing.cost_short += pos.cost_short;
                    existing.market_value += pos.market_value;
                    existing.float_profit += pos.float_profit;
                } else {
                    position_map[pos.code] = pos;
                }
            }

            merged.positions.clear();
            for (const auto& pair : position_map) {
                merged.positions.push_back(pair.second);
            }

            // Append orders and trades
            merged.orders.insert(merged.orders.end(), account.orders.begin(), account.orders.end());
            merged.trades.insert(merged.trades.end(), account.trades.begin(), account.trades.end());
        }

        return merged;
    }, "Merge multiple QIFI accounts into one",
       py::arg("accounts"));

    utils.def("convert_qifi_to_dataframe", [](const protocol::QIFIAccount& account) {
        py::dict result;

        // Positions DataFrame
        py::list position_data;
        for (const auto& pos : account.positions) {
            py::dict row;
            row["code"] = pos.code;
            row["volume_long"] = pos.volume_long;
            row["volume_short"] = pos.volume_short;
            row["price"] = pos.price;
            row["cost_long"] = pos.cost_long;
            row["cost_short"] = pos.cost_short;
            row["market_value"] = pos.market_value;
            row["float_profit"] = pos.float_profit;
            row["last_datetime"] = pos.last_datetime;
            position_data.append(row);
        }
        result["positions"] = position_data;

        // Orders DataFrame
        py::list order_data;
        for (const auto& order : account.orders) {
            py::dict row;
            row["order_id"] = order.order_id;
            row["code"] = order.code;
            row["volume"] = order.volume;
            row["price"] = order.price;
            row["direction"] = order.direction;
            row["status"] = order.status;
            row["datetime"] = order.datetime;
            row["trade_volume"] = order.trade_volume;
            row["trade_price"] = order.trade_price;
            row["commission"] = order.commission;
            order_data.append(row);
        }
        result["orders"] = order_data;

        // Trades DataFrame
        py::list trade_data;
        for (const auto& trade : account.trades) {
            py::dict row;
            row["trade_id"] = trade.trade_id;
            row["order_id"] = trade.order_id;
            row["code"] = trade.code;
            row["volume"] = trade.volume;
            row["price"] = trade.price;
            row["direction"] = trade.direction;
            row["datetime"] = trade.datetime;
            row["commission"] = trade.commission;
            trade_data.append(row);
        }
        result["trades"] = trade_data;

        return result;
    }, "Convert QIFI account to DataFrame-ready format",
       py::arg("account"));

    // Protocol serializers
    utils.def("serialize_qifi_binary", [](const protocol::QIFIAccount& account) {
        // Simplified binary serialization - in practice would use protobuf or similar
        auto json_str = account.to_json().dump();
        return py::bytes(json_str);
    }, "Serialize QIFI account to binary format",
       py::arg("account"));

    utils.def("deserialize_qifi_binary", [](const py::bytes& data) {
        std::string json_str = data;
        auto json_obj = nlohmann::json::parse(json_str);
        protocol::QIFIAccount account;
        account.from_json(json_obj);
        return account;
    }, "Deserialize QIFI account from binary format",
       py::arg("data"));
}