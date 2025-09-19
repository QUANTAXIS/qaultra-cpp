#include "../../include/qaultra/data/datatype_simple.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace qaultra::data {

// 简化的Kline实现
double Kline::get_change_percent() const {
    if (open == 0.0) return 0.0;
    return (close - open) / open * 100.0;
}

bool Kline::is_limit_up() const {
    if (limit_up == 0.0) return false;
    return std::abs(close - limit_up) < 1e-9;
}

bool Kline::is_limit_down() const {
    if (limit_down == 0.0) return false;
    return std::abs(close - limit_down) < 1e-9;
}

std::string Kline::get_change_sign() const {
    if (close > open) return "+";
    else if (close < open) return "-";
    else return "=";
}

double Kline::get_amount() const {
    return total_turnover;
}

double Kline::get_amplitude() const {
    if (low == 0.0) return 0.0;
    return (high - low) / low * 100.0;
}

bool Kline::operator==(const Kline& other) const {
    return order_book_id == other.order_book_id &&
           std::abs(open - other.open) < 1e-9 &&
           std::abs(close - other.close) < 1e-9 &&
           std::abs(high - other.high) < 1e-9 &&
           std::abs(low - other.low) < 1e-9 &&
           std::abs(volume - other.volume) < 1e-9;
}

// JSON序列化
nlohmann::json Kline::to_json() const {
    nlohmann::json j;
    j["order_book_id"] = order_book_id;
    j["datetime"] = datetime;
    j["open"] = open;
    j["close"] = close;
    j["high"] = high;
    j["low"] = low;
    j["volume"] = volume;
    j["total_turnover"] = total_turnover;
    j["limit_up"] = limit_up;
    j["limit_down"] = limit_down;
    j["pre_close"] = pre_close;
    j["suspended"] = suspended;
    return j;
}

void Kline::from_json(const nlohmann::json& j) {
    j.at("order_book_id").get_to(order_book_id);
    j.at("datetime").get_to(datetime);
    j.at("open").get_to(open);
    j.at("close").get_to(close);
    j.at("high").get_to(high);
    j.at("low").get_to(low);
    j.at("volume").get_to(volume);
    j.at("total_turnover").get_to(total_turnover);

    if (j.contains("limit_up")) j.at("limit_up").get_to(limit_up);
    if (j.contains("limit_down")) j.at("limit_down").get_to(limit_down);
    if (j.contains("pre_close")) j.at("pre_close").get_to(pre_close);
    if (j.contains("suspended")) j.at("suspended").get_to(suspended);
}

// 工具函数命名空间实现
namespace utils {

std::string get_current_date_string() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

std::string get_current_datetime_string() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool is_trading_time(const std::string& time_str) {
    // 简化实现：检查是否在交易时间段内
    // 上午：9:30-11:30, 下午：13:00-15:00

    if (time_str.length() < 8) return false;

    std::string time_part = time_str.substr(time_str.length() - 8); // HH:MM:SS
    int hour = std::stoi(time_part.substr(0, 2));
    int minute = std::stoi(time_part.substr(3, 2));

    int total_minutes = hour * 60 + minute;

    // 上午交易时间：9:30-11:30 (570-690分钟)
    if (total_minutes >= 570 && total_minutes <= 690) return true;

    // 下午交易时间：13:00-15:00 (780-900分钟)
    if (total_minutes >= 780 && total_minutes <= 900) return true;

    return false;
}

std::string format_price(double price, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << price;
    return ss.str();
}

std::string format_volume(double volume) {
    if (volume >= 100000000) { // 1亿
        return format_price(volume / 100000000, 2) + "亿";
    } else if (volume >= 10000) { // 1万
        return format_price(volume / 10000, 2) + "万";
    } else {
        return format_price(volume, 0);
    }
}

bool validate_order_book_id(const std::string& order_book_id) {
    // 简化验证：检查基本格式
    if (order_book_id.empty()) return false;

    // 股票：6位数字 + .XSHE 或 .XSHG
    if (order_book_id.length() == 11) {
        if (order_book_id.substr(6) == ".XSHE" || order_book_id.substr(6) == ".XSHG") {
            std::string code = order_book_id.substr(0, 6);
            return std::all_of(code.begin(), code.end(), ::isdigit);
        }
    }

    // 期货：基本检查是否包含交易所后缀
    if (order_book_id.find(".") != std::string::npos) {
        return true; // 简化检查
    }

    return false;
}

MarketType get_market_type(const std::string& order_book_id) {
    if (order_book_id.find(".XSHE") != std::string::npos ||
        order_book_id.find(".XSHG") != std::string::npos) {
        return MarketType::Stock;
    }

    if (order_book_id.find(".DCE") != std::string::npos ||
        order_book_id.find(".CZCE") != std::string::npos ||
        order_book_id.find(".SHFE") != std::string::npos ||
        order_book_id.find(".INE") != std::string::npos) {
        return MarketType::Future;
    }

    return MarketType::Unknown;
}

} // namespace utils
} // namespace qaultra::data