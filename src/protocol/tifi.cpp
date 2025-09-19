#include "../../include/qaultra/protocol/tifi.hpp"
#include "../../include/qaultra/util/uuid_generator.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace qaultra::protocol::tifi {

// Order 实现
nlohmann::json Order::to_json() const {
    nlohmann::json j;

    // 基本信息
    j["order_id"] = order_id;
    j["account_id"] = account_id;
    j["user_id"] = user_id;
    j["strategy_id"] = strategy_id;

    // 合约信息
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["product_id"] = product_id;

    // 交易信息
    j["direction"] = utils::direction_to_string(direction);
    j["offset"] = utils::offset_to_string(offset);
    j["volume"] = volume;
    j["price"] = price;
    j["price_type"] = utils::price_type_to_string(price_type);
    j["time_condition"] = utils::time_condition_to_string(time_condition);

    // 状态信息
    j["status"] = utils::order_status_to_string(status);
    j["volume_traded"] = volume_traded;
    j["volume_left"] = volume_left;
    j["avg_price"] = avg_price;

    // 时间信息
    j["insert_time"] = insert_time;
    j["update_time"] = update_time;
    j["cancel_time"] = cancel_time;

    // 错误信息
    j["error_code"] = error_code;
    j["error_message"] = error_message;

    // 交易所信息
    j["exchange_order_id"] = exchange_order_id;
    j["parent_order_id"] = parent_order_id;

    // 费用信息
    j["commission"] = commission;
    j["tax"] = tax;

    return j;
}

Order Order::from_json(const nlohmann::json& j) {
    Order order;

    // 基本信息
    if (j.contains("order_id")) j.at("order_id").get_to(order.order_id);
    if (j.contains("account_id")) j.at("account_id").get_to(order.account_id);
    if (j.contains("user_id")) j.at("user_id").get_to(order.user_id);
    if (j.contains("strategy_id")) j.at("strategy_id").get_to(order.strategy_id);

    // 合约信息
    if (j.contains("instrument_id")) j.at("instrument_id").get_to(order.instrument_id);
    if (j.contains("exchange_id")) j.at("exchange_id").get_to(order.exchange_id);
    if (j.contains("product_id")) j.at("product_id").get_to(order.product_id);

    // 交易信息
    if (j.contains("direction")) {
        order.direction = utils::string_to_direction(j.at("direction"));
    }
    if (j.contains("offset")) {
        order.offset = utils::string_to_offset(j.at("offset"));
    }
    if (j.contains("volume")) j.at("volume").get_to(order.volume);
    if (j.contains("price")) j.at("price").get_to(order.price);
    if (j.contains("price_type")) {
        order.price_type = utils::string_to_price_type(j.at("price_type"));
    }
    if (j.contains("time_condition")) {
        order.time_condition = utils::string_to_time_condition(j.at("time_condition"));
    }

    // 状态信息
    if (j.contains("status")) {
        order.status = utils::string_to_order_status(j.at("status"));
    }
    if (j.contains("volume_traded")) j.at("volume_traded").get_to(order.volume_traded);
    if (j.contains("volume_left")) j.at("volume_left").get_to(order.volume_left);
    if (j.contains("avg_price")) j.at("avg_price").get_to(order.avg_price);

    // 时间信息
    if (j.contains("insert_time")) j.at("insert_time").get_to(order.insert_time);
    if (j.contains("update_time")) j.at("update_time").get_to(order.update_time);
    if (j.contains("cancel_time")) j.at("cancel_time").get_to(order.cancel_time);

    // 错误信息
    if (j.contains("error_code")) j.at("error_code").get_to(order.error_code);
    if (j.contains("error_message")) j.at("error_message").get_to(order.error_message);

    // 交易所信息
    if (j.contains("exchange_order_id")) j.at("exchange_order_id").get_to(order.exchange_order_id);
    if (j.contains("parent_order_id")) j.at("parent_order_id").get_to(order.parent_order_id);

    // 费用信息
    if (j.contains("commission")) j.at("commission").get_to(order.commission);
    if (j.contains("tax")) j.at("tax").get_to(order.tax);

    return order;
}

// Trade 实现
nlohmann::json Trade::to_json() const {
    nlohmann::json j;
    j["trade_id"] = trade_id;
    j["order_id"] = order_id;
    j["account_id"] = account_id;
    j["user_id"] = user_id;
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["direction"] = utils::direction_to_string(direction);
    j["offset"] = utils::offset_to_string(offset);
    j["volume"] = volume;
    j["price"] = price;
    j["trade_time"] = trade_time;
    j["commission"] = commission;
    j["tax"] = tax;
    j["exchange_trade_id"] = exchange_trade_id;
    return j;
}

Trade Trade::from_json(const nlohmann::json& j) {
    Trade trade;
    if (j.contains("trade_id")) j.at("trade_id").get_to(trade.trade_id);
    if (j.contains("order_id")) j.at("order_id").get_to(trade.order_id);
    if (j.contains("account_id")) j.at("account_id").get_to(trade.account_id);
    if (j.contains("user_id")) j.at("user_id").get_to(trade.user_id);
    if (j.contains("instrument_id")) j.at("instrument_id").get_to(trade.instrument_id);
    if (j.contains("exchange_id")) j.at("exchange_id").get_to(trade.exchange_id);
    if (j.contains("direction")) {
        trade.direction = utils::string_to_direction(j.at("direction"));
    }
    if (j.contains("offset")) {
        trade.offset = utils::string_to_offset(j.at("offset"));
    }
    if (j.contains("volume")) j.at("volume").get_to(trade.volume);
    if (j.contains("price")) j.at("price").get_to(trade.price);
    if (j.contains("trade_time")) j.at("trade_time").get_to(trade.trade_time);
    if (j.contains("commission")) j.at("commission").get_to(trade.commission);
    if (j.contains("tax")) j.at("tax").get_to(trade.tax);
    if (j.contains("exchange_trade_id")) j.at("exchange_trade_id").get_to(trade.exchange_trade_id);
    return trade;
}

// Position 实现
nlohmann::json Position::to_json() const {
    nlohmann::json j;
    j["account_id"] = account_id;
    j["user_id"] = user_id;
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["product_id"] = product_id;
    j["long_position"] = long_position;
    j["short_position"] = short_position;
    j["long_position_today"] = long_position_today;
    j["short_position_today"] = short_position_today;
    j["long_position_yesterday"] = long_position_yesterday;
    j["short_position_yesterday"] = short_position_yesterday;
    j["long_avg_price"] = long_avg_price;
    j["short_avg_price"] = short_avg_price;
    j["last_price"] = last_price;
    j["pre_settle_price"] = pre_settle_price;
    j["settle_price"] = settle_price;
    j["position_pnl"] = position_pnl;
    j["close_pnl"] = close_pnl;
    j["realized_pnl"] = realized_pnl;
    j["unrealized_pnl"] = unrealized_pnl;
    j["margin"] = margin;
    j["margin_ratio"] = margin_ratio;
    j["update_time"] = update_time;
    return j;
}

Position Position::from_json(const nlohmann::json& j) {
    Position pos;
    if (j.contains("account_id")) j.at("account_id").get_to(pos.account_id);
    if (j.contains("user_id")) j.at("user_id").get_to(pos.user_id);
    if (j.contains("instrument_id")) j.at("instrument_id").get_to(pos.instrument_id);
    if (j.contains("exchange_id")) j.at("exchange_id").get_to(pos.exchange_id);
    if (j.contains("product_id")) j.at("product_id").get_to(pos.product_id);
    if (j.contains("long_position")) j.at("long_position").get_to(pos.long_position);
    if (j.contains("short_position")) j.at("short_position").get_to(pos.short_position);
    if (j.contains("long_position_today")) j.at("long_position_today").get_to(pos.long_position_today);
    if (j.contains("short_position_today")) j.at("short_position_today").get_to(pos.short_position_today);
    if (j.contains("long_position_yesterday")) j.at("long_position_yesterday").get_to(pos.long_position_yesterday);
    if (j.contains("short_position_yesterday")) j.at("short_position_yesterday").get_to(pos.short_position_yesterday);
    if (j.contains("long_avg_price")) j.at("long_avg_price").get_to(pos.long_avg_price);
    if (j.contains("short_avg_price")) j.at("short_avg_price").get_to(pos.short_avg_price);
    if (j.contains("last_price")) j.at("last_price").get_to(pos.last_price);
    if (j.contains("pre_settle_price")) j.at("pre_settle_price").get_to(pos.pre_settle_price);
    if (j.contains("settle_price")) j.at("settle_price").get_to(pos.settle_price);
    if (j.contains("position_pnl")) j.at("position_pnl").get_to(pos.position_pnl);
    if (j.contains("close_pnl")) j.at("close_pnl").get_to(pos.close_pnl);
    if (j.contains("realized_pnl")) j.at("realized_pnl").get_to(pos.realized_pnl);
    if (j.contains("unrealized_pnl")) j.at("unrealized_pnl").get_to(pos.unrealized_pnl);
    if (j.contains("margin")) j.at("margin").get_to(pos.margin);
    if (j.contains("margin_ratio")) j.at("margin_ratio").get_to(pos.margin_ratio);
    if (j.contains("update_time")) j.at("update_time").get_to(pos.update_time);
    return pos;
}

// Account 实现
nlohmann::json Account::to_json() const {
    nlohmann::json j;
    j["account_id"] = account_id;
    j["user_id"] = user_id;
    j["account_name"] = account_name;
    j["account_type"] = account_type;
    j["total_asset"] = total_asset;
    j["available_cash"] = available_cash;
    j["frozen_cash"] = frozen_cash;
    j["margin"] = margin;
    j["position_value"] = position_value;
    j["realized_pnl"] = realized_pnl;
    j["unrealized_pnl"] = unrealized_pnl;
    j["total_pnl"] = total_pnl;
    j["close_pnl"] = close_pnl;
    j["risk_ratio"] = risk_ratio;
    j["margin_ratio"] = margin_ratio;
    j["commission"] = commission;
    j["tax"] = tax;
    j["trading_day"] = trading_day;
    j["update_time"] = update_time;
    return j;
}

Account Account::from_json(const nlohmann::json& j) {
    Account acc;
    if (j.contains("account_id")) j.at("account_id").get_to(acc.account_id);
    if (j.contains("user_id")) j.at("user_id").get_to(acc.user_id);
    if (j.contains("account_name")) j.at("account_name").get_to(acc.account_name);
    if (j.contains("account_type")) j.at("account_type").get_to(acc.account_type);
    if (j.contains("total_asset")) j.at("total_asset").get_to(acc.total_asset);
    if (j.contains("available_cash")) j.at("available_cash").get_to(acc.available_cash);
    if (j.contains("frozen_cash")) j.at("frozen_cash").get_to(acc.frozen_cash);
    if (j.contains("margin")) j.at("margin").get_to(acc.margin);
    if (j.contains("position_value")) j.at("position_value").get_to(acc.position_value);
    if (j.contains("realized_pnl")) j.at("realized_pnl").get_to(acc.realized_pnl);
    if (j.contains("unrealized_pnl")) j.at("unrealized_pnl").get_to(acc.unrealized_pnl);
    if (j.contains("total_pnl")) j.at("total_pnl").get_to(acc.total_pnl);
    if (j.contains("close_pnl")) j.at("close_pnl").get_to(acc.close_pnl);
    if (j.contains("risk_ratio")) j.at("risk_ratio").get_to(acc.risk_ratio);
    if (j.contains("margin_ratio")) j.at("margin_ratio").get_to(acc.margin_ratio);
    if (j.contains("commission")) j.at("commission").get_to(acc.commission);
    if (j.contains("tax")) j.at("tax").get_to(acc.tax);
    if (j.contains("trading_day")) j.at("trading_day").get_to(acc.trading_day);
    if (j.contains("update_time")) j.at("update_time").get_to(acc.update_time);
    return acc;
}

// RiskMetrics 实现
nlohmann::json RiskMetrics::to_json() const {
    nlohmann::json j;
    j["account_id"] = account_id;
    j["datetime"] = datetime;
    j["total_return"] = total_return;
    j["annual_return"] = annual_return;
    j["daily_return"] = daily_return;
    j["volatility"] = volatility;
    j["max_drawdown"] = max_drawdown;
    j["sharpe_ratio"] = sharpe_ratio;
    j["sortino_ratio"] = sortino_ratio;
    j["win_rate"] = win_rate;
    j["profit_loss_ratio"] = profit_loss_ratio;
    j["total_trades"] = total_trades;
    j["win_trades"] = win_trades;
    return j;
}

RiskMetrics RiskMetrics::from_json(const nlohmann::json& j) {
    RiskMetrics metrics;
    if (j.contains("account_id")) j.at("account_id").get_to(metrics.account_id);
    if (j.contains("datetime")) j.at("datetime").get_to(metrics.datetime);
    if (j.contains("total_return")) j.at("total_return").get_to(metrics.total_return);
    if (j.contains("annual_return")) j.at("annual_return").get_to(metrics.annual_return);
    if (j.contains("daily_return")) j.at("daily_return").get_to(metrics.daily_return);
    if (j.contains("volatility")) j.at("volatility").get_to(metrics.volatility);
    if (j.contains("max_drawdown")) j.at("max_drawdown").get_to(metrics.max_drawdown);
    if (j.contains("sharpe_ratio")) j.at("sharpe_ratio").get_to(metrics.sharpe_ratio);
    if (j.contains("sortino_ratio")) j.at("sortino_ratio").get_to(metrics.sortino_ratio);
    if (j.contains("win_rate")) j.at("win_rate").get_to(metrics.win_rate);
    if (j.contains("profit_loss_ratio")) j.at("profit_loss_ratio").get_to(metrics.profit_loss_ratio);
    if (j.contains("total_trades")) j.at("total_trades").get_to(metrics.total_trades);
    if (j.contains("win_trades")) j.at("win_trades").get_to(metrics.win_trades);
    return metrics;
}

// 工具函数实现
namespace utils {

std::string direction_to_string(Direction dir) {
    switch (dir) {
        case Direction::BUY: return "BUY";
        case Direction::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

Direction string_to_direction(const std::string& str) {
    if (str == "BUY") return Direction::BUY;
    if (str == "SELL") return Direction::SELL;
    return Direction::UNKNOWN;
}

std::string offset_to_string(Offset offset) {
    switch (offset) {
        case Offset::OPEN: return "OPEN";
        case Offset::CLOSE: return "CLOSE";
        case Offset::CLOSE_TODAY: return "CLOSE_TODAY";
        case Offset::CLOSE_YESTERDAY: return "CLOSE_YESTERDAY";
        case Offset::FORCE_CLOSE: return "FORCE_CLOSE";
        default: return "UNKNOWN";
    }
}

Offset string_to_offset(const std::string& str) {
    if (str == "OPEN") return Offset::OPEN;
    if (str == "CLOSE") return Offset::CLOSE;
    if (str == "CLOSE_TODAY") return Offset::CLOSE_TODAY;
    if (str == "CLOSE_YESTERDAY") return Offset::CLOSE_YESTERDAY;
    if (str == "FORCE_CLOSE") return Offset::FORCE_CLOSE;
    return Offset::UNKNOWN;
}

std::string order_status_to_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::PARTIAL_FILLED: return "PARTIAL_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
        case OrderStatus::EXPIRED: return "EXPIRED";
        default: return "UNKNOWN";
    }
}

OrderStatus string_to_order_status(const std::string& str) {
    if (str == "PENDING") return OrderStatus::PENDING;
    if (str == "PARTIAL_FILLED") return OrderStatus::PARTIAL_FILLED;
    if (str == "FILLED") return OrderStatus::FILLED;
    if (str == "CANCELLED") return OrderStatus::CANCELLED;
    if (str == "REJECTED") return OrderStatus::REJECTED;
    if (str == "EXPIRED") return OrderStatus::EXPIRED;
    return OrderStatus::UNKNOWN;
}

std::string price_type_to_string(PriceType type) {
    switch (type) {
        case PriceType::LIMIT: return "LIMIT";
        case PriceType::MARKET: return "MARKET";
        case PriceType::STOP: return "STOP";
        case PriceType::STOP_LIMIT: return "STOP_LIMIT";
        case PriceType::FAK: return "FAK";
        case PriceType::FOK: return "FOK";
        default: return "UNKNOWN";
    }
}

PriceType string_to_price_type(const std::string& str) {
    if (str == "LIMIT") return PriceType::LIMIT;
    if (str == "MARKET") return PriceType::MARKET;
    if (str == "STOP") return PriceType::STOP;
    if (str == "STOP_LIMIT") return PriceType::STOP_LIMIT;
    if (str == "FAK") return PriceType::FAK;
    if (str == "FOK") return PriceType::FOK;
    return PriceType::UNKNOWN;
}

std::string time_condition_to_string(TimeCondition condition) {
    switch (condition) {
        case TimeCondition::IOC: return "IOC";
        case TimeCondition::GTC: return "GTC";
        case TimeCondition::GTD: return "GTD";
        case TimeCondition::DAY: return "DAY";
        case TimeCondition::FAK: return "FAK";
        case TimeCondition::FOK: return "FOK";
        default: return "UNKNOWN";
    }
}

TimeCondition string_to_time_condition(const std::string& str) {
    if (str == "IOC") return TimeCondition::IOC;
    if (str == "GTC") return TimeCondition::GTC;
    if (str == "GTD") return TimeCondition::GTD;
    if (str == "DAY") return TimeCondition::DAY;
    if (str == "FAK") return TimeCondition::FAK;
    if (str == "FOK") return TimeCondition::FOK;
    return TimeCondition::UNKNOWN;
}

std::string generate_trade_id() {
    return qaultra::util::UUIDGenerator::generate_time_based_id("TRADE");
}

bool validate_order(const Order& order) {
    // 基本验证
    if (order.order_id.empty() || order.instrument_id.empty()) {
        return false;
    }

    if (order.volume <= 0.0) {
        return false;
    }

    if (order.price_type == PriceType::LIMIT && order.price <= 0.0) {
        return false;
    }

    return true;
}

double calculate_commission(const std::string& instrument_id, double volume, double price) {
    // 简化的手续费计算
    // 实际项目中应该根据合约类型和券商费率计算

    double commission = 0.0;

    // 股票手续费：万分之三，最低5元
    if (instrument_id.find(".XSHE") != std::string::npos ||
        instrument_id.find(".XSHG") != std::string::npos) {
        commission = volume * price * 0.0003;
        if (commission < 5.0) commission = 5.0;
    }
    // 期货手续费：按手数计算
    else {
        commission = volume * 2.0; // 简化为每手2元
    }

    return commission;
}

} // namespace utils

} // namespace qaultra::protocol::tifi