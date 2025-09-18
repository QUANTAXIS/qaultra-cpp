#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "qaultra/account/account_full.hpp"
#include "qaultra/account/order.hpp"
#include "qaultra/account/position.hpp"

namespace py = pybind11;

void bind_account_types(py::module& m) {
    auto account_module = m.def_submodule("account", "Account management module");

    // Order class
    py::class_<qaultra::account::Order>(account_module, "Order")
        .def(py::init<const std::string&, const std::string&, const std::string&,
                     qaultra::account::Direction, double, double>(),
             "order_id"_a, "account_id"_a, "code"_a, "direction"_a, "price"_a, "volume"_a)
        .def_readwrite("order_id", &qaultra::account::Order::order_id)
        .def_readwrite("account_id", &qaultra::account::Order::account_id)
        .def_readwrite("code", &qaultra::account::Order::code)
        .def_readwrite("direction", &qaultra::account::Order::direction)
        .def_readwrite("price", &qaultra::account::Order::price)
        .def_readwrite("volume", &qaultra::account::Order::volume)
        .def_readwrite("volume_left", &qaultra::account::Order::volume_left)
        .def_readwrite("volume_filled", &qaultra::account::Order::volume_filled)
        .def_readwrite("status", &qaultra::account::Order::status)
        .def_readwrite("exchange_id", &qaultra::account::Order::exchange_id)
        .def_readwrite("user_id", &qaultra::account::Order::user_id)
        .def_readwrite("strategy_id", &qaultra::account::Order::strategy_id)
        .def_readwrite("last_msg", &qaultra::account::Order::last_msg)
        .def("is_active", &qaultra::account::Order::is_active)
        .def("is_completed", &qaultra::account::Order::is_completed)
        .def("is_partially_filled", &qaultra::account::Order::is_partially_filled)
        .def("fill_percentage", &qaultra::account::Order::fill_percentage)
        .def("get_remaining_volume", &qaultra::account::Order::get_remaining_volume)
        .def("fill", &qaultra::account::Order::fill, "volume"_a, "price"_a)
        .def("cancel", &qaultra::account::Order::cancel)
        .def("reject", &qaultra::account::Order::reject, "reason"_a)
        .def("get_order_value", &qaultra::account::Order::get_order_value)
        .def("get_filled_value", &qaultra::account::Order::get_filled_value)
        .def("to_json", &qaultra::account::Order::to_json)
        .def_static("from_json", &qaultra::account::Order::from_json)
        .def("to_string", &qaultra::account::Order::to_string)
        .def("__str__", &qaultra::account::Order::to_string);

    // Position class
    py::class_<qaultra::account::Position>(account_module, "Position")
        .def(py::init<const std::string&>(), "code"_a)
        .def(py::init<const std::string&, const std::string&, const std::string&, const std::string&>(),
             "code"_a, "account_id"_a, "user_id"_a, "exchange_id"_a)
        .def_readwrite("code", &qaultra::account::Position::code)
        .def_readwrite("account_id", &qaultra::account::Position::account_id)
        .def_readwrite("user_id", &qaultra::account::Position::user_id)
        .def_readwrite("exchange_id", &qaultra::account::Position::exchange_id)
        .def_readwrite("volume_long_today", &qaultra::account::Position::volume_long_today)
        .def_readwrite("volume_long_his", &qaultra::account::Position::volume_long_his)
        .def_readwrite("volume_short_today", &qaultra::account::Position::volume_short_today)
        .def_readwrite("volume_short_his", &qaultra::account::Position::volume_short_his)
        .def_readwrite("open_cost_long", &qaultra::account::Position::open_cost_long)
        .def_readwrite("open_cost_short", &qaultra::account::Position::open_cost_short)
        .def_readwrite("latest_price", &qaultra::account::Position::latest_price)
        .def_readwrite("latest_datetime", &qaultra::account::Position::latest_datetime)
        .def_readwrite("margin_long", &qaultra::account::Position::margin_long)
        .def_readwrite("margin_short", &qaultra::account::Position::margin_short)
        .def_readwrite("commission", &qaultra::account::Position::commission)
        .def("volume_long", &qaultra::account::Position::volume_long)
        .def("volume_short", &qaultra::account::Position::volume_short)
        .def("volume_net", &qaultra::account::Position::volume_net)
        .def("is_long", &qaultra::account::Position::is_long)
        .def("is_short", &qaultra::account::Position::is_short)
        .def("is_flat", &qaultra::account::Position::is_flat)
        .def("margin", &qaultra::account::Position::margin)
        .def("avg_price_long", &qaultra::account::Position::avg_price_long)
        .def("avg_price_short", &qaultra::account::Position::avg_price_short)
        .def("market_value_long", &qaultra::account::Position::market_value_long)
        .def("market_value_short", &qaultra::account::Position::market_value_short)
        .def("market_value", &qaultra::account::Position::market_value)
        .def("float_profit_long", &qaultra::account::Position::float_profit_long)
        .def("float_profit_short", &qaultra::account::Position::float_profit_short)
        .def("float_profit", &qaultra::account::Position::float_profit)
        .def("position_profit_long", &qaultra::account::Position::position_profit_long)
        .def("position_profit_short", &qaultra::account::Position::position_profit_short)
        .def("position_profit", &qaultra::account::Position::position_profit)
        .def("on_price_change", &qaultra::account::Position::on_price_change, "price"_a, "datetime"_a)
        .def("update_position", &qaultra::account::Position::update_position, "price"_a, "volume"_a, "towards"_a)
        .def("close_position", &qaultra::account::Position::close_position, "volume"_a, "price"_a, "is_today"_a = true)
        .def("settle", &qaultra::account::Position::settle)
        .def("calculate_commission", &qaultra::account::Position::calculate_commission, "volume"_a, "price"_a, "direction"_a)
        .def("to_json", &qaultra::account::Position::to_json)
        .def_static("from_json", &qaultra::account::Position::from_json)
        .def("to_string", &qaultra::account::Position::to_string)
        .def("__str__", &qaultra::account::Position::to_string);

    // MarketPreset
    py::class_<qaultra::account::MarketPreset>(account_module, "MarketPreset")
        .def(py::init<>())
        .def(py::init<const std::string&>(), "market_name"_a)
        .def_readwrite("name", &qaultra::account::MarketPreset::name)
        .def_readwrite("unit_table", &qaultra::account::MarketPreset::unit_table)
        .def_readwrite("price_tick", &qaultra::account::MarketPreset::price_tick)
        .def_readwrite("volume_tick", &qaultra::account::MarketPreset::volume_tick)
        .def_readwrite("buy_fee_ratio", &qaultra::account::MarketPreset::buy_fee_ratio)
        .def_readwrite("sell_fee_ratio", &qaultra::account::MarketPreset::sell_fee_ratio)
        .def_readwrite("min_fee", &qaultra::account::MarketPreset::min_fee)
        .def_readwrite("tax_ratio", &qaultra::account::MarketPreset::tax_ratio)
        .def_readwrite("margin_ratio", &qaultra::account::MarketPreset::margin_ratio)
        .def_readwrite("is_stock", &qaultra::account::MarketPreset::is_stock)
        .def_readwrite("allow_t0", &qaultra::account::MarketPreset::allow_t0)
        .def_readwrite("allow_sellopen", &qaultra::account::MarketPreset::allow_sellopen)
        .def_static("get_stock_preset", &qaultra::account::MarketPreset::get_stock_preset)
        .def_static("get_future_preset", &qaultra::account::MarketPreset::get_future_preset)
        .def_static("get_forex_preset", &qaultra::account::MarketPreset::get_forex_preset)
        .def("to_json", &qaultra::account::MarketPreset::to_json)
        .def_static("from_json", &qaultra::account::MarketPreset::from_json);

    // QA_Account class (main account interface matching Rust)
    py::class_<qaultra::account::QA_Account>(account_module, "QA_Account")
        .def(py::init<const std::string&, const std::string&, const std::string&, double, bool, const std::string&>(),
             "account_cookie"_a, "portfolio_cookie"_a, "user_cookie"_a, "init_cash"_a,
             "auto_reload"_a = false, "environment"_a = "real")
        .def_readwrite("init_cash", &qaultra::account::QA_Account::init_cash)
        .def_readwrite("allow_t0", &qaultra::account::QA_Account::allow_t0)
        .def_readwrite("allow_sellopen", &qaultra::account::QA_Account::allow_sellopen)
        .def_readwrite("allow_margin", &qaultra::account::QA_Account::allow_margin)
        .def_readwrite("auto_reload", &qaultra::account::QA_Account::auto_reload)
        .def_readwrite("time", &qaultra::account::QA_Account::time)
        .def_readwrite("account_cookie", &qaultra::account::QA_Account::account_cookie)
        .def_readwrite("portfolio_cookie", &qaultra::account::QA_Account::portfolio_cookie)
        .def_readwrite("user_cookie", &qaultra::account::QA_Account::user_cookie)
        .def_readwrite("environment", &qaultra::account::QA_Account::environment)
        .def_readwrite("commission_ratio", &qaultra::account::QA_Account::commission_ratio)
        .def_readwrite("tax_ratio", &qaultra::account::QA_Account::tax_ratio)

        // Configuration methods
        .def("set_sellopen", &qaultra::account::QA_Account::set_sellopen, "sellopen"_a)
        .def("set_t0", &qaultra::account::QA_Account::set_t0, "t0"_a)
        .def("set_portfolio_cookie", &qaultra::account::QA_Account::set_portfolio_cookie, "portfolio"_a)
        .def("set_commission_ratio", &qaultra::account::QA_Account::set_commission_ratio, "ratio"_a)
        .def("set_tax_ratio", &qaultra::account::QA_Account::set_tax_ratio, "ratio"_a)

        // Position management
        .def("init_position", &qaultra::account::QA_Account::init_position, "code"_a)
        .def("get_position", py::overload_cast<const std::string&>(&qaultra::account::QA_Account::get_position, py::const_),
             "code"_a, py::return_value_policy::reference_internal)
        .def("get_all_positions", &qaultra::account::QA_Account::get_all_positions)
        .def("get_volume_long", &qaultra::account::QA_Account::get_volume_long, "code"_a)
        .def("get_volume_short", &qaultra::account::QA_Account::get_volume_short, "code"_a)
        .def("get_volume_net", &qaultra::account::QA_Account::get_volume_net, "code"_a)

        // Account information
        .def("get_cash", &qaultra::account::QA_Account::get_cash)
        .def("get_frozen_margin", &qaultra::account::QA_Account::get_frozen_margin)
        .def("get_risk_ratio", &qaultra::account::QA_Account::get_risk_ratio)
        .def("get_position_profit", &qaultra::account::QA_Account::get_position_profit)
        .def("get_float_profit", &qaultra::account::QA_Account::get_float_profit)
        .def("get_balance", &qaultra::account::QA_Account::get_balance)
        .def("get_margin", &qaultra::account::QA_Account::get_margin)
        .def("get_available", &qaultra::account::QA_Account::get_available)

        // Trading operations
        .def("buy", &qaultra::account::QA_Account::buy, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("sell", &qaultra::account::QA_Account::sell, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("buy_open", &qaultra::account::QA_Account::buy_open, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("sell_open", &qaultra::account::QA_Account::sell_open, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("buy_close", &qaultra::account::QA_Account::buy_close, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("sell_close", &qaultra::account::QA_Account::sell_close, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("buy_close_today", &qaultra::account::QA_Account::buy_close_today, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("sell_close_today", &qaultra::account::QA_Account::sell_close_today, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("smart_buy", &qaultra::account::QA_Account::smart_buy, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)
        .def("smart_sell", &qaultra::account::QA_Account::smart_sell, "code"_a, "volume"_a, "datetime"_a, "price"_a,
             py::return_value_policy::take_ownership)

        // Market data updates
        .def("on_price_change", &qaultra::account::QA_Account::on_price_change, "code"_a, "new_price"_a, "datetime"_a)
        .def("update_market_data", &qaultra::account::QA_Account::update_market_data, "prices"_a, "datetime"_a)

        // Order management
        .def("get_order", py::overload_cast<const std::string&>(&qaultra::account::QA_Account::get_order, py::const_),
             "order_id"_a, py::return_value_policy::reference_internal)
        .def("get_orders", &qaultra::account::QA_Account::get_orders)
        .def("get_active_orders", &qaultra::account::QA_Account::get_active_orders)
        .def("cancel_order", &qaultra::account::QA_Account::cancel_order, "order_id"_a)
        .def("cancel_all_orders", &qaultra::account::QA_Account::cancel_all_orders)

        // Account operations
        .def("settle", &qaultra::account::QA_Account::settle)
        .def("change_datetime", &qaultra::account::QA_Account::change_datetime, "datetime"_a)
        .def("deposit", &qaultra::account::QA_Account::deposit, "amount"_a)
        .def("withdraw", &qaultra::account::QA_Account::withdraw, "amount"_a)
        .def("reload", &qaultra::account::QA_Account::reload)

        // Analytics
        .def("get_mom_slice", &qaultra::account::QA_Account::get_mom_slice)
        .def("get_account_slice", &qaultra::account::QA_Account::get_account_slice)
        .def("get_account_info", &qaultra::account::QA_Account::get_account_info)
        .def("get_account_pos_value", &qaultra::account::QA_Account::get_account_pos_value)
        .def("get_account_pos_weight", &qaultra::account::QA_Account::get_account_pos_weight)

        // Risk management
        .def("check_order_allowed", &qaultra::account::QA_Account::check_order_allowed,
             "code"_a, "volume"_a, "price"_a, "direction"_a)
        .def("check_available_funds", &qaultra::account::QA_Account::check_available_funds, "required_amount"_a)
        .def("get_max_order_size", &qaultra::account::QA_Account::get_max_order_size,
             "code"_a, "price"_a, "direction"_a)
        .def("calculate_required_margin", &qaultra::account::QA_Account::calculate_required_margin,
             "code"_a, "volume"_a)

        // Serialization
        .def("to_qifi", &qaultra::account::QA_Account::to_qifi)
        .def("to_json", &qaultra::account::QA_Account::to_json)
        .def_static("from_json", &qaultra::account::QA_Account::from_json, py::return_value_policy::take_ownership)
        .def_static("from_qifi", &qaultra::account::QA_Account::from_qifi, py::return_value_policy::take_ownership);

    // Order factory functions
    auto order_factory = account_module.def_submodule("order_factory", "Order factory functions");
    order_factory.def("create_market_buy", &qaultra::account::order_factory::create_market_buy,
                     "account_id"_a, "code"_a, "volume"_a, py::return_value_policy::take_ownership);
    order_factory.def("create_market_sell", &qaultra::account::order_factory::create_market_sell,
                     "account_id"_a, "code"_a, "volume"_a, py::return_value_policy::take_ownership);
    order_factory.def("create_limit_buy", &qaultra::account::order_factory::create_limit_buy,
                     "account_id"_a, "code"_a, "volume"_a, "price"_a, py::return_value_policy::take_ownership);
    order_factory.def("create_limit_sell", &qaultra::account::order_factory::create_limit_sell,
                     "account_id"_a, "code"_a, "volume"_a, "price"_a, py::return_value_policy::take_ownership);

    // Account factory functions
    auto account_factory = account_module.def_submodule("account_factory", "Account factory functions");
    account_factory.def("create_real_account", &qaultra::account::account_factory::create_real_account,
                       "account_id"_a, "broker_name"_a, "user_id"_a, "initial_balance"_a,
                       py::return_value_policy::take_ownership);
    account_factory.def("create_sim_account", &qaultra::account::account_factory::create_sim_account,
                       "account_id"_a, "user_id"_a, "initial_balance"_a,
                       py::return_value_policy::take_ownership);
    account_factory.def("create_backtest_account", &qaultra::account::account_factory::create_backtest_account,
                       "account_id"_a, "user_id"_a, "initial_balance"_a,
                       py::return_value_policy::take_ownership);
    account_factory.def("from_qifi_message", &qaultra::account::account_factory::from_qifi_message,
                       "qifi"_a, py::return_value_policy::take_ownership);
}