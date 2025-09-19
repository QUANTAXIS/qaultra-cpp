#include "../../include/qaultra/protocol/mifi.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace qaultra::protocol::mifi {

// Kline 实现
nlohmann::json Kline::to_json() const {
    nlohmann::json j;
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["datetime"] = datetime;
    j["market_type"] = utils::market_type_to_string(market_type);

    j["open"] = open;
    j["high"] = high;
    j["low"] = low;
    j["close"] = close;
    j["volume"] = volume;
    j["amount"] = amount;

    j["pre_close"] = pre_close;
    j["settle_price"] = settle_price;
    j["pre_settle"] = pre_settle;
    j["limit_up"] = limit_up;
    j["limit_down"] = limit_down;
    j["open_interest"] = open_interest;

    j["trade_count"] = trade_count;
    j["avg_price"] = avg_price;
    j["turnover_rate"] = turnover_rate;

    return j;
}

Kline Kline::from_json(const nlohmann::json& j) {
    Kline kline;

    j.at("instrument_id").get_to(kline.instrument_id);
    j.at("exchange_id").get_to(kline.exchange_id);
    j.at("datetime").get_to(kline.datetime);

    if (j.contains("market_type")) {
        kline.market_type = utils::string_to_market_type(j.at("market_type"));
    }

    j.at("open").get_to(kline.open);
    j.at("high").get_to(kline.high);
    j.at("low").get_to(kline.low);
    j.at("close").get_to(kline.close);
    j.at("volume").get_to(kline.volume);
    j.at("amount").get_to(kline.amount);

    if (j.contains("pre_close")) j.at("pre_close").get_to(kline.pre_close);
    if (j.contains("settle_price")) j.at("settle_price").get_to(kline.settle_price);
    if (j.contains("pre_settle")) j.at("pre_settle").get_to(kline.pre_settle);
    if (j.contains("limit_up")) j.at("limit_up").get_to(kline.limit_up);
    if (j.contains("limit_down")) j.at("limit_down").get_to(kline.limit_down);
    if (j.contains("open_interest")) j.at("open_interest").get_to(kline.open_interest);
    if (j.contains("trade_count")) j.at("trade_count").get_to(kline.trade_count);
    if (j.contains("avg_price")) j.at("avg_price").get_to(kline.avg_price);
    if (j.contains("turnover_rate")) j.at("turnover_rate").get_to(kline.turnover_rate);

    return kline;
}

// Tick 实现
nlohmann::json Tick::to_json() const {
    nlohmann::json j;
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["datetime"] = datetime;
    j["market_type"] = utils::market_type_to_string(market_type);
    j["status"] = utils::trading_status_to_string(status);

    j["last_price"] = last_price;
    j["pre_close"] = pre_close;
    j["open"] = open;
    j["high"] = high;
    j["low"] = low;

    j["volume"] = volume;
    j["amount"] = amount;
    j["trade_count"] = trade_count;

    j["bid_prices"] = bid_prices;
    j["bid_volumes"] = bid_volumes;
    j["ask_prices"] = ask_prices;
    j["ask_volumes"] = ask_volumes;

    j["settle_price"] = settle_price;
    j["pre_settle"] = pre_settle;
    j["open_interest"] = open_interest;
    j["limit_up"] = limit_up;
    j["limit_down"] = limit_down;

    return j;
}

Tick Tick::from_json(const nlohmann::json& j) {
    Tick tick;

    j.at("instrument_id").get_to(tick.instrument_id);
    j.at("exchange_id").get_to(tick.exchange_id);
    j.at("datetime").get_to(tick.datetime);

    if (j.contains("market_type")) {
        tick.market_type = utils::string_to_market_type(j.at("market_type"));
    }
    if (j.contains("status")) {
        tick.status = utils::string_to_trading_status(j.at("status"));
    }

    j.at("last_price").get_to(tick.last_price);
    if (j.contains("pre_close")) j.at("pre_close").get_to(tick.pre_close);
    if (j.contains("open")) j.at("open").get_to(tick.open);
    if (j.contains("high")) j.at("high").get_to(tick.high);
    if (j.contains("low")) j.at("low").get_to(tick.low);

    if (j.contains("volume")) j.at("volume").get_to(tick.volume);
    if (j.contains("amount")) j.at("amount").get_to(tick.amount);
    if (j.contains("trade_count")) j.at("trade_count").get_to(tick.trade_count);

    if (j.contains("bid_prices")) j.at("bid_prices").get_to(tick.bid_prices);
    if (j.contains("bid_volumes")) j.at("bid_volumes").get_to(tick.bid_volumes);
    if (j.contains("ask_prices")) j.at("ask_prices").get_to(tick.ask_prices);
    if (j.contains("ask_volumes")) j.at("ask_volumes").get_to(tick.ask_volumes);

    if (j.contains("settle_price")) j.at("settle_price").get_to(tick.settle_price);
    if (j.contains("pre_settle")) j.at("pre_settle").get_to(tick.pre_settle);
    if (j.contains("open_interest")) j.at("open_interest").get_to(tick.open_interest);
    if (j.contains("limit_up")) j.at("limit_up").get_to(tick.limit_up);
    if (j.contains("limit_down")) j.at("limit_down").get_to(tick.limit_down);

    return tick;
}

// Transaction 实现
nlohmann::json Transaction::to_json() const {
    nlohmann::json j;
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["datetime"] = datetime;
    j["trade_id"] = trade_id;
    j["price"] = price;
    j["volume"] = volume;
    j["direction"] = direction;
    return j;
}

Transaction Transaction::from_json(const nlohmann::json& j) {
    Transaction trans;
    j.at("instrument_id").get_to(trans.instrument_id);
    j.at("exchange_id").get_to(trans.exchange_id);
    j.at("datetime").get_to(trans.datetime);
    j.at("trade_id").get_to(trans.trade_id);
    j.at("price").get_to(trans.price);
    j.at("volume").get_to(trans.volume);
    j.at("direction").get_to(trans.direction);
    return trans;
}

// OrderQueue 实现
nlohmann::json OrderQueue::to_json() const {
    nlohmann::json j;
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["datetime"] = datetime;

    nlohmann::json buy_array = nlohmann::json::array();
    for (const auto& level : buy_queue) {
        nlohmann::json level_obj;
        level_obj["price"] = level.price;
        level_obj["volume"] = level.volume;
        level_obj["order_count"] = level.order_count;
        buy_array.push_back(level_obj);
    }
    j["buy_queue"] = buy_array;

    nlohmann::json sell_array = nlohmann::json::array();
    for (const auto& level : sell_queue) {
        nlohmann::json level_obj;
        level_obj["price"] = level.price;
        level_obj["volume"] = level.volume;
        level_obj["order_count"] = level.order_count;
        sell_array.push_back(level_obj);
    }
    j["sell_queue"] = sell_array;

    return j;
}

OrderQueue OrderQueue::from_json(const nlohmann::json& j) {
    OrderQueue queue;
    j.at("instrument_id").get_to(queue.instrument_id);
    j.at("exchange_id").get_to(queue.exchange_id);
    j.at("datetime").get_to(queue.datetime);

    if (j.contains("buy_queue")) {
        for (const auto& level_obj : j.at("buy_queue")) {
            PriceLevel level;
            level_obj.at("price").get_to(level.price);
            level_obj.at("volume").get_to(level.volume);
            if (level_obj.contains("order_count")) {
                level_obj.at("order_count").get_to(level.order_count);
            }
            queue.buy_queue.push_back(level);
        }
    }

    if (j.contains("sell_queue")) {
        for (const auto& level_obj : j.at("sell_queue")) {
            PriceLevel level;
            level_obj.at("price").get_to(level.price);
            level_obj.at("volume").get_to(level.volume);
            if (level_obj.contains("order_count")) {
                level_obj.at("order_count").get_to(level.order_count);
            }
            queue.sell_queue.push_back(level);
        }
    }

    return queue;
}

// MarketStatus 实现
nlohmann::json MarketStatus::to_json() const {
    nlohmann::json j;
    j["exchange_id"] = exchange_id;
    j["datetime"] = datetime;
    j["status"] = utils::trading_status_to_string(status);
    j["message"] = message;
    j["session_begin"] = session_begin;
    j["session_end"] = session_end;
    j["auction_begin"] = auction_begin;
    j["auction_end"] = auction_end;
    return j;
}

MarketStatus MarketStatus::from_json(const nlohmann::json& j) {
    MarketStatus status;
    j.at("exchange_id").get_to(status.exchange_id);
    j.at("datetime").get_to(status.datetime);
    status.status = utils::string_to_trading_status(j.at("status"));
    j.at("message").get_to(status.message);

    if (j.contains("session_begin")) j.at("session_begin").get_to(status.session_begin);
    if (j.contains("session_end")) j.at("session_end").get_to(status.session_end);
    if (j.contains("auction_begin")) j.at("auction_begin").get_to(status.auction_begin);
    if (j.contains("auction_end")) j.at("auction_end").get_to(status.auction_end);

    return status;
}

// InstrumentInfo 实现
nlohmann::json InstrumentInfo::to_json() const {
    nlohmann::json j;
    j["instrument_id"] = instrument_id;
    j["exchange_id"] = exchange_id;
    j["product_id"] = product_id;
    j["instrument_name"] = instrument_name;
    j["market_type"] = utils::market_type_to_string(market_type);

    j["price_tick"] = price_tick;
    j["lot_size"] = lot_size;
    j["multiplier"] = multiplier;
    j["margin_rate"] = margin_rate;
    j["commission_rate"] = commission_rate;
    j["min_commission"] = min_commission;
    j["limit_up_rate"] = limit_up_rate;
    j["limit_down_rate"] = limit_down_rate;

    j["list_date"] = list_date;
    j["expire_date"] = expire_date;
    j["delivery_date"] = delivery_date;
    j["is_trading"] = is_trading;
    j["is_suspended"] = is_suspended;

    return j;
}

InstrumentInfo InstrumentInfo::from_json(const nlohmann::json& j) {
    InstrumentInfo info;
    j.at("instrument_id").get_to(info.instrument_id);
    j.at("exchange_id").get_to(info.exchange_id);

    if (j.contains("product_id")) j.at("product_id").get_to(info.product_id);
    if (j.contains("instrument_name")) j.at("instrument_name").get_to(info.instrument_name);
    if (j.contains("market_type")) {
        info.market_type = utils::string_to_market_type(j.at("market_type"));
    }

    if (j.contains("price_tick")) j.at("price_tick").get_to(info.price_tick);
    if (j.contains("lot_size")) j.at("lot_size").get_to(info.lot_size);
    if (j.contains("multiplier")) j.at("multiplier").get_to(info.multiplier);
    if (j.contains("margin_rate")) j.at("margin_rate").get_to(info.margin_rate);
    if (j.contains("commission_rate")) j.at("commission_rate").get_to(info.commission_rate);
    if (j.contains("min_commission")) j.at("min_commission").get_to(info.min_commission);
    if (j.contains("limit_up_rate")) j.at("limit_up_rate").get_to(info.limit_up_rate);
    if (j.contains("limit_down_rate")) j.at("limit_down_rate").get_to(info.limit_down_rate);

    if (j.contains("list_date")) j.at("list_date").get_to(info.list_date);
    if (j.contains("expire_date")) j.at("expire_date").get_to(info.expire_date);
    if (j.contains("delivery_date")) j.at("delivery_date").get_to(info.delivery_date);
    if (j.contains("is_trading")) j.at("is_trading").get_to(info.is_trading);
    if (j.contains("is_suspended")) j.at("is_suspended").get_to(info.is_suspended);

    return info;
}

// 工具函数实现
namespace utils {

std::string market_type_to_string(MarketType type) {
    switch (type) {
        case MarketType::STOCK: return "STOCK";
        case MarketType::FUTURE: return "FUTURE";
        case MarketType::OPTION: return "OPTION";
        case MarketType::FUND: return "FUND";
        case MarketType::BOND: return "BOND";
        case MarketType::INDEX: return "INDEX";
        case MarketType::FOREX: return "FOREX";
        case MarketType::CRYPTO: return "CRYPTO";
        case MarketType::COMMODITY: return "COMMODITY";
        default: return "UNKNOWN";
    }
}

MarketType string_to_market_type(const std::string& str) {
    if (str == "STOCK") return MarketType::STOCK;
    if (str == "FUTURE") return MarketType::FUTURE;
    if (str == "OPTION") return MarketType::OPTION;
    if (str == "FUND") return MarketType::FUND;
    if (str == "BOND") return MarketType::BOND;
    if (str == "INDEX") return MarketType::INDEX;
    if (str == "FOREX") return MarketType::FOREX;
    if (str == "CRYPTO") return MarketType::CRYPTO;
    if (str == "COMMODITY") return MarketType::COMMODITY;
    return MarketType::UNKNOWN;
}

std::string trading_status_to_string(TradingStatus status) {
    switch (status) {
        case TradingStatus::TRADING: return "TRADING";
        case TradingStatus::HALT: return "HALT";
        case TradingStatus::SUSPENSION: return "SUSPENSION";
        case TradingStatus::PRE_OPEN: return "PRE_OPEN";
        case TradingStatus::CLOSED: return "CLOSED";
        case TradingStatus::AUCTION: return "AUCTION";
        default: return "UNKNOWN";
    }
}

TradingStatus string_to_trading_status(const std::string& str) {
    if (str == "TRADING") return TradingStatus::TRADING;
    if (str == "HALT") return TradingStatus::HALT;
    if (str == "SUSPENSION") return TradingStatus::SUSPENSION;
    if (str == "PRE_OPEN") return TradingStatus::PRE_OPEN;
    if (str == "CLOSED") return TradingStatus::CLOSED;
    if (str == "AUCTION") return TradingStatus::AUCTION;
    return TradingStatus::UNKNOWN;
}

bool validate_instrument_id(const std::string& instrument_id) {
    if (instrument_id.empty()) return false;

    // 股票：6位数字 + .XSHE 或 .XSHG
    if (instrument_id.length() == 11 && instrument_id[6] == '.') {
        std::string code = instrument_id.substr(0, 6);
        std::string exchange = instrument_id.substr(7);
        if ((exchange == "XSHE" || exchange == "XSHG") &&
            std::all_of(code.begin(), code.end(), ::isdigit)) {
            return true;
        }
    }

    // 期货：基本检查格式
    if (instrument_id.find('.') != std::string::npos) {
        return true; // 简化验证
    }

    return false;
}

MarketType get_market_type_from_instrument(const std::string& instrument_id) {
    if (instrument_id.find(".XSHE") != std::string::npos ||
        instrument_id.find(".XSHG") != std::string::npos) {
        return MarketType::STOCK;
    }

    if (instrument_id.find(".DCE") != std::string::npos ||
        instrument_id.find(".CZCE") != std::string::npos ||
        instrument_id.find(".SHFE") != std::string::npos ||
        instrument_id.find(".INE") != std::string::npos) {
        return MarketType::FUTURE;
    }

    return MarketType::UNKNOWN;
}

std::string standardize_datetime(const std::string& datetime) {
    // 简化实现：假设输入已经是ISO 8601格式
    // 实际项目中可以添加更复杂的日期时间解析和标准化
    if (datetime.find('T') == std::string::npos && datetime.find(' ') != std::string::npos) {
        // 将 "YYYY-MM-DD HH:MM:SS" 转换为 "YYYY-MM-DDTHH:MM:SS"
        std::string result = datetime;
        std::replace(result.begin(), result.end(), ' ', 'T');
        return result;
    }
    return datetime;
}

} // namespace utils

} // namespace qaultra::protocol::mifi