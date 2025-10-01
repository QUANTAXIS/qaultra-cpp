#include "qaultra/data/datatype.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace qaultra::data {

// StockCnDay 实现
nlohmann::json StockCnDay::to_json() const {
    nlohmann::json j;
    j["date"] = utils::date_to_string(date);
    j["order_book_id"] = order_book_id;
    j["num_trades"] = num_trades;
    j["limit_up"] = limit_up;
    j["limit_down"] = limit_down;
    j["open"] = open;
    j["high"] = high;
    j["low"] = low;
    j["close"] = close;
    j["volume"] = volume;
    j["total_turnover"] = total_turnover;
    return j;
}

StockCnDay StockCnDay::from_json(const nlohmann::json& j) {
    return StockCnDay(
        utils::string_to_date(j.value("date", "")),
        j.value("order_book_id", std::string("")),
        j.value("num_trades", 0.0f),
        j.value("limit_up", 0.0f),
        j.value("limit_down", 0.0f),
        j.value("open", 0.0f),
        j.value("high", 0.0f),
        j.value("low", 0.0f),
        j.value("close", 0.0f),
        j.value("volume", 0.0f),
        j.value("total_turnover", 0.0f)
    );
}

// StockCn1Min 实现
nlohmann::json StockCn1Min::to_json() const {
    nlohmann::json j;
    j["datetime"] = utils::timestamp_to_string(datetime);
    j["order_book_id"] = order_book_id;
    j["open"] = open;
    j["high"] = high;
    j["low"] = low;
    j["close"] = close;
    j["volume"] = volume;
    j["total_turnover"] = total_turnover;
    return j;
}

StockCn1Min StockCn1Min::from_json(const nlohmann::json& j) {
    return StockCn1Min(
        utils::string_to_timestamp(j.value("datetime", "")),
        j.value("order_book_id", std::string("")),
        j.value("open", 0.0f),
        j.value("high", 0.0f),
        j.value("low", 0.0f),
        j.value("close", 0.0f),
        j.value("volume", 0.0f),
        j.value("total_turnover", 0.0f)
    );
}

// FutureCn1Min 实现
nlohmann::json FutureCn1Min::to_json() const {
    nlohmann::json j;
    j["datetime"] = utils::timestamp_to_string(datetime);
    j["trading_date"] = utils::date_to_string(trading_date);
    j["order_book_id"] = order_book_id;
    j["open_interest"] = open_interest;
    j["open"] = open;
    j["high"] = high;
    j["low"] = low;
    j["close"] = close;
    j["volume"] = volume;
    j["total_turnover"] = total_turnover;
    return j;
}

FutureCn1Min FutureCn1Min::from_json(const nlohmann::json& j) {
    return FutureCn1Min(
        utils::string_to_timestamp(j.value("datetime", "")),
        utils::string_to_date(j.value("trading_date", "")),
        j.value("order_book_id", std::string("")),
        j.value("open_interest", 0.0f),
        j.value("open", 0.0f),
        j.value("high", 0.0f),
        j.value("low", 0.0f),
        j.value("close", 0.0f),
        j.value("volume", 0.0f),
        j.value("total_turnover", 0.0f)
    );
}

// FutureCnDay 实现
nlohmann::json FutureCnDay::to_json() const {
    nlohmann::json j;
    j["date"] = utils::date_to_string(date);
    j["order_book_id"] = order_book_id;
    j["limit_up"] = limit_up;
    j["limit_down"] = limit_down;
    j["open_interest"] = open_interest;
    j["prev_settlement"] = prev_settlement;
    j["settlement"] = settlement;
    j["open"] = open;
    j["high"] = high;
    j["low"] = low;
    j["close"] = close;
    j["volume"] = volume;
    j["total_turnover"] = total_turnover;
    return j;
}

FutureCnDay FutureCnDay::from_json(const nlohmann::json& j) {
    return FutureCnDay(
        utils::string_to_date(j.value("date", "")),
        j.value("order_book_id", std::string("")),
        j.value("limit_up", 0.0f),
        j.value("limit_down", 0.0f),
        j.value("open_interest", 0.0f),
        j.value("prev_settlement", 0.0f),
        j.value("settlement", 0.0f),
        j.value("open", 0.0f),
        j.value("high", 0.0f),
        j.value("low", 0.0f),
        j.value("close", 0.0f),
        j.value("volume", 0.0f),
        j.value("total_turnover", 0.0f)
    );
}

// Kline 实现
nlohmann::json Kline::to_json() const {
    nlohmann::json j;
    j["order_book_id"] = order_book_id;
    j["open"] = open;
    j["close"] = close;
    j["high"] = high;
    j["low"] = low;
    j["volume"] = volume;
    j["limit_up"] = limit_up;
    j["limit_down"] = limit_down;
    j["total_turnover"] = total_turnover;
    j["split_coefficient_to"] = split_coefficient_to;
    j["dividend_cash_before_tax"] = dividend_cash_before_tax;
    return j;
}

Kline Kline::from_json(const nlohmann::json& j) {
    return Kline(
        j.value("order_book_id", std::string("")),
        j.value("open", 0.0),
        j.value("close", 0.0),
        j.value("high", 0.0),
        j.value("low", 0.0),
        j.value("volume", 0.0),
        j.value("limit_up", 0.0),
        j.value("limit_down", 0.0),
        j.value("total_turnover", 0.0),
        j.value("split_coefficient_to", 0.0),
        j.value("dividend_cash_before_tax", 0.0)
    );
}

// 工具函数实现
namespace utils {

std::string timestamp_to_string(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::chrono::system_clock::time_point string_to_timestamp(const std::string& str) {
    std::tm tm = {};
    std::istringstream ss(str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        // 尝试只有日期的格式
        std::istringstream ss2(str);
        ss2 >> std::get_time(&tm, "%Y-%m-%d");
        if (ss2.fail()) {
            return std::chrono::system_clock::time_point{};
        }
    }

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string date_to_string(const Date& date) {
    return date.to_string();
}

Date string_to_date(const std::string& str) {
    std::istringstream ss(str);
    int year, month, day;
    char dash1, dash2;

    ss >> year >> dash1 >> month >> dash2 >> day;

    if (ss.fail() || dash1 != '-' || dash2 != '-') {
        return Date(1970, 1, 1);
    }

    return Date(year, month, day);
}

int trading_days_between(const Date& start, const Date& end) {
    // 简化实现，实际项目中应该考虑节假日
    // 使用简单的日期差计算，假设平均每周5个交易日
    int start_days = start.year * 365 + start.month * 30 + start.day;
    int end_days = end.year * 365 + end.month * 30 + end.day;
    int total_days = end_days - start_days;

    // 粗略估算：约 5/7 的天数是交易日
    return static_cast<int>(total_days * 5.0 / 7.0);
}

bool is_trading_day(const Date& date) {
    // 简化实现：使用 1970-01-01 作为基准（星期四）
    // 计算距离基准的天数，然后判断星期几
    int days_since_epoch = (date.year - 1970) * 365 + (date.month - 1) * 30 + (date.day - 1);
    int weekday = (days_since_epoch + 4) % 7;  // 1970-01-01 是星期四 (4)

    // 0=Sunday, 1=Monday, ..., 6=Saturday
    return weekday != 0 && weekday != 6;
}

Date next_trading_day(const Date& date) {
    Date current = date;
    do {
        // 简单的日期加1（不考虑月末和年末边界）
        current.day++;
        if (current.day > 31) {
            current.day = 1;
            current.month++;
            if (current.month > 12) {
                current.month = 1;
                current.year++;
            }
        }
    } while (!is_trading_day(current));

    return current;
}

Date prev_trading_day(const Date& date) {
    Date current = date;
    do {
        // 简单的日期减1（不考虑月初和年初边界）
        current.day--;
        if (current.day < 1) {
            current.month--;
            if (current.month < 1) {
                current.month = 12;
                current.year--;
            }
            current.day = 31;  // 简化：假设所有月份都有31天
        }
    } while (!is_trading_day(current));

    return current;
}

} // namespace utils

} // namespace qaultra::data