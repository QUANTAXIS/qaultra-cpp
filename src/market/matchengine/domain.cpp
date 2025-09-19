#include "qaultra/market/matchengine/domain.hpp"
#include <iomanip>
#include <sstream>
#include <chrono>

namespace qaultra::market::matchengine {

// Success 序列化实现
nlohmann::json Success::to_json() const {
    nlohmann::json j;
    j["type"] = static_cast<int>(type);
    j["id"] = id;
    j["order_id"] = order_id;
    j["opposite_order_id"] = opposite_order_id;
    j["direction"] = static_cast<int>(direction);
    j["order_type"] = static_cast<int>(order_type);
    j["price"] = price;
    j["volume"] = volume;
    j["ts"] = ts;
    return j;
}

Success Success::from_json(const nlohmann::json& j) {
    Success s;
    s.type = static_cast<Success::Type>(j.value("type", 0));
    s.id = j.value("id", 0UL);
    s.order_id = j.value("order_id", 0UL);
    s.opposite_order_id = j.value("opposite_order_id", 0UL);
    s.direction = static_cast<OrderDirection>(j.value("direction", 0));
    s.order_type = static_cast<OrderType>(j.value("order_type", 0));
    s.price = j.value("price", 0.0);
    s.volume = j.value("volume", 0.0);
    s.ts = j.value("ts", 0L);
    return s;
}

// Failed 序列化实现
nlohmann::json Failed::to_json() const {
    nlohmann::json j;
    j["type"] = static_cast<int>(type);
    j["order_id"] = order_id;
    j["message"] = message;
    return j;
}

Failed Failed::from_json(const nlohmann::json& j) {
    Failed f;
    f.type = static_cast<Failed::Type>(j.value("type", 0));
    f.order_id = j.value("order_id", 0UL);
    f.message = j.value("message", std::string(""));
    return f;
}

// 工具函数实现
namespace utils {

std::string direction_to_string(OrderDirection direction) {
    switch (direction) {
        case OrderDirection::BUY: return "BUY";
        case OrderDirection::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

OrderDirection string_to_direction(const std::string& str) {
    if (str == "BUY") return OrderDirection::BUY;
    if (str == "SELL") return OrderDirection::SELL;
    return OrderDirection::BUY; // 默认值
}

std::string order_type_to_string(OrderType order_type) {
    switch (order_type) {
        case OrderType::Market: return "Market";
        case OrderType::Limit: return "Limit";
        default: return "UNKNOWN";
    }
}

OrderType string_to_order_type(const std::string& str) {
    if (str == "Market") return OrderType::Market;
    if (str == "Limit") return OrderType::Limit;
    return OrderType::Limit; // 默认值
}

std::string trading_state_to_string(TradingState state) {
    switch (state) {
        case TradingState::PreAuctionPeriod: return "PreAuctionPeriod";
        case TradingState::AuctionOrder: return "AuctionOrder";
        case TradingState::AuctionCancel: return "AuctionCancel";
        case TradingState::AuctionMatch: return "AuctionMatch";
        case TradingState::ContinuousTrading: return "ContinuousTrading";
        case TradingState::Closed: return "Closed";
        default: return "UNKNOWN";
    }
}

TradingState string_to_trading_state(const std::string& str) {
    if (str == "PreAuctionPeriod") return TradingState::PreAuctionPeriod;
    if (str == "AuctionOrder") return TradingState::AuctionOrder;
    if (str == "AuctionCancel") return TradingState::AuctionCancel;
    if (str == "AuctionMatch") return TradingState::AuctionMatch;
    if (str == "ContinuousTrading") return TradingState::ContinuousTrading;
    if (str == "Closed") return TradingState::Closed;
    return TradingState::Closed; // 默认值
}

int64_t get_timestamp_nanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace utils

} // namespace qaultra::market::matchengine