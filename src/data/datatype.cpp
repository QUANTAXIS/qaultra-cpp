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

std::string date_to_string(const std::chrono::year_month_day& date) {
    std::stringstream ss;
    ss << static_cast<int>(date.year()) << "-"
       << std::setfill('0') << std::setw(2) << static_cast<unsigned>(date.month()) << "-"
       << std::setfill('0') << std::setw(2) << static_cast<unsigned>(date.day());
    return ss.str();
}

std::chrono::year_month_day string_to_date(const std::string& str) {
    std::istringstream ss(str);
    int year, month, day;
    char dash1, dash2;

    ss >> year >> dash1 >> month >> dash2 >> day;

    if (ss.fail() || dash1 != '-' || dash2 != '-') {
        return std::chrono::year_month_day{
            std::chrono::year{1970}, std::chrono::month{1}, std::chrono::day{1}
        };
    }

    return std::chrono::year_month_day{
        std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}
    };
}

int trading_days_between(const std::chrono::year_month_day& start,
                        const std::chrono::year_month_day& end) {
    // 简化实现，实际项目中应该考虑节假日
    auto start_sys = std::chrono::sys_days{start};
    auto end_sys = std::chrono::sys_days{end};
    auto diff = end_sys - start_sys;

    int days = diff.count();
    int trading_days = 0;

    for (int i = 0; i <= days; ++i) {
        auto current_date = start + std::chrono::days{i};
        if (is_trading_day(current_date)) {
            trading_days++;
        }
    }

    return trading_days;
}

bool is_trading_day(const std::chrono::year_month_day& date) {
    // 简化实现：排除周末，实际项目中应该考虑节假日
    auto sys_days = std::chrono::sys_days{date};
    auto weekday = std::chrono::weekday{sys_days};

    return weekday != std::chrono::Saturday && weekday != std::chrono::Sunday;
}

std::chrono::year_month_day next_trading_day(const std::chrono::year_month_day& date) {
    auto current = date;
    do {
        current = current + std::chrono::days{1};
    } while (!is_trading_day(current));

    return current;
}

std::chrono::year_month_day prev_trading_day(const std::chrono::year_month_day& date) {
    auto current = date;
    do {
        current = current - std::chrono::days{1};
    } while (!is_trading_day(current));

    return current;
}

} // namespace utils

} // namespace qaultra::data