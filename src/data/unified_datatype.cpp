#include "qaultra/data/unified_datatype.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <regex>
#include <algorithm>

namespace qaultra::data {

// =======================
// UnifiedKline 实现
// =======================

double UnifiedKline::get_change_percent() const {
    if (!cached_change_percent.has_value()) {
        if (pre_close == 0.0) {
            cached_change_percent = 0.0;
        } else {
            cached_change_percent = (close - pre_close) / pre_close * 100.0;
        }
    }
    return cached_change_percent.value();
}

double UnifiedKline::get_amplitude() const {
    if (!cached_amplitude.has_value()) {
        if (pre_close == 0.0) {
            cached_amplitude = 0.0;
        } else {
            cached_amplitude = (high - low) / pre_close * 100.0;
        }
    }
    return cached_amplitude.value();
}

bool UnifiedKline::is_limit_up() const {
    if (limit_up == 0.0) return false;
    return std::abs(close - limit_up) < 1e-9;
}

bool UnifiedKline::is_limit_down() const {
    if (limit_down == 0.0) return false;
    return std::abs(close - limit_down) < 1e-9;
}

std::string UnifiedKline::get_change_sign() const {
    if (pre_close == 0.0) return "=";
    if (close > pre_close) return "+";
    else if (close < pre_close) return "-";
    else return "=";
}

bool UnifiedKline::is_valid() const {
    // 基础数据验证
    if (order_book_id.empty()) return false;
    if (open < 0 || high < 0 || low < 0 || close < 0) return false;
    if (volume < 0 || total_turnover < 0) return false;

    // OHLC逻辑验证
    if (high < std::max({open, close, low}) ||
        low > std::min({open, close, high})) return false;

    return true;
}

std::string UnifiedKline::to_string() const {
    std::ostringstream oss;
    oss << order_book_id << " "
        << utils::time_point_to_string(datetime) << " "
        << "O:" << std::fixed << std::setprecision(2) << open << " "
        << "H:" << high << " "
        << "L:" << low << " "
        << "C:" << close << " "
        << "V:" << std::setprecision(0) << volume;
    return oss.str();
}

bool UnifiedKline::operator==(const UnifiedKline& other) const {
    return order_book_id == other.order_book_id &&
           datetime == other.datetime &&
           std::abs(open - other.open) < 1e-9 &&
           std::abs(high - other.high) < 1e-9 &&
           std::abs(low - other.low) < 1e-9 &&
           std::abs(close - other.close) < 1e-9 &&
           std::abs(volume - other.volume) < 1e-9;
}

bool UnifiedKline::operator<(const UnifiedKline& other) const {
    if (order_book_id != other.order_book_id) {
        return order_book_id < other.order_book_id;
    }
    return datetime < other.datetime;
}

nlohmann::json UnifiedKline::to_json() const {
    nlohmann::json j;
    j["order_book_id"] = order_book_id;
    j["datetime"] = utils::time_point_to_string(datetime);
    j["market_type"] = utils::market_type_to_string(market_type);
    j["frequency"] = utils::frequency_to_string(frequency);
    j["open"] = open;
    j["high"] = high;
    j["low"] = low;
    j["close"] = close;
    j["volume"] = volume;
    j["total_turnover"] = total_turnover;
    j["pre_close"] = pre_close;
    j["limit_up"] = limit_up;
    j["limit_down"] = limit_down;
    j["num_trades"] = num_trades;
    j["suspended"] = suspended;
    return j;
}

UnifiedKline UnifiedKline::from_json(const nlohmann::json& j) {
    UnifiedKline kline;
    kline.order_book_id = j.at("order_book_id").get<std::string>();
    kline.datetime = utils::string_to_time_point(j.at("datetime").get<std::string>());

    if (j.contains("market_type")) {
        kline.market_type = utils::string_to_market_type(j.at("market_type").get<std::string>());
    }
    if (j.contains("frequency")) {
        kline.frequency = utils::string_to_frequency(j.at("frequency").get<std::string>());
    }

    kline.open = j.at("open").get<double>();
    kline.high = j.at("high").get<double>();
    kline.low = j.at("low").get<double>();
    kline.close = j.at("close").get<double>();
    kline.volume = j.at("volume").get<double>();
    kline.total_turnover = j.at("total_turnover").get<double>();

    if (j.contains("pre_close")) {
        kline.pre_close = j.at("pre_close").get<double>();
    }
    if (j.contains("limit_up")) {
        kline.limit_up = j.at("limit_up").get<double>();
    }
    if (j.contains("limit_down")) {
        kline.limit_down = j.at("limit_down").get<double>();
    }
    if (j.contains("num_trades")) {
        kline.num_trades = j.at("num_trades").get<int>();
    }
    if (j.contains("suspended")) {
        kline.suspended = j.at("suspended").get<bool>();
    }

    return kline;
}

UnifiedKline UnifiedKline::from_simple_kline(const std::string& code,
                                           const std::string& datetime_str,
                                           double o, double h, double l, double c,
                                           double vol, double turnover) {
    UnifiedKline kline;
    kline.order_book_id = code;
    kline.datetime = utils::string_to_time_point(datetime_str);
    kline.open = o;
    kline.high = h;
    kline.low = l;
    kline.close = c;
    kline.volume = vol;
    kline.total_turnover = turnover;
    kline.market_type = utils::detect_market_type(code);
    return kline;
}

void UnifiedKline::to_simple_format(std::string& datetime_str,
                                   double& o, double& h, double& l, double& c,
                                   double& vol, double& turnover) const {
    datetime_str = utils::time_point_to_string(datetime);
    o = open;
    h = high;
    l = low;
    c = close;
    vol = volume;
    turnover = total_turnover;
}

// =======================
// StockCnDay 实现
// =======================

time_point StockCnDay::get_datetime() const {
    return utils::ymd_to_time_point(date);
}

nlohmann::json StockCnDay::to_json() const {
    nlohmann::json j;
    j["date"] = utils::time_point_to_string(get_datetime());
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
    StockCnDay data;
    auto tp = utils::string_to_time_point(j.at("date").get<std::string>());
    data.date = utils::time_point_to_ymd(tp);
    data.order_book_id = j.at("order_book_id").get<std::string>();
    data.num_trades = j.at("num_trades").get<float>();
    data.limit_up = j.at("limit_up").get<float>();
    data.limit_down = j.at("limit_down").get<float>();
    data.open = j.at("open").get<float>();
    data.high = j.at("high").get<float>();
    data.low = j.at("low").get<float>();
    data.close = j.at("close").get<float>();
    data.volume = j.at("volume").get<float>();
    data.total_turnover = j.at("total_turnover").get<float>();
    return data;
}

UnifiedKline StockCnDay::to_unified_kline() const {
    UnifiedKline kline;
    kline.order_book_id = order_book_id;
    kline.datetime = get_datetime();
    kline.market_type = MarketType::Stock;
    kline.frequency = Frequency::Daily;
    kline.open = static_cast<double>(open);
    kline.high = static_cast<double>(high);
    kline.low = static_cast<double>(low);
    kline.close = static_cast<double>(close);
    kline.volume = static_cast<double>(volume);
    kline.total_turnover = static_cast<double>(total_turnover);
    kline.limit_up = static_cast<double>(limit_up);
    kline.limit_down = static_cast<double>(limit_down);
    kline.num_trades = static_cast<int>(num_trades);
    return kline;
}

StockCnDay StockCnDay::from_unified_kline(const UnifiedKline& kline) {
    StockCnDay data;
    data.date = utils::time_point_to_ymd(kline.datetime);
    data.order_book_id = kline.order_book_id;
    data.num_trades = static_cast<float>(kline.num_trades);
    data.limit_up = static_cast<float>(kline.limit_up);
    data.limit_down = static_cast<float>(kline.limit_down);
    data.open = static_cast<float>(kline.open);
    data.high = static_cast<float>(kline.high);
    data.low = static_cast<float>(kline.low);
    data.close = static_cast<float>(kline.close);
    data.volume = static_cast<float>(kline.volume);
    data.total_turnover = static_cast<float>(kline.total_turnover);
    return data;
}

// =======================
// StockCn1Min 实现
// =======================

nlohmann::json StockCn1Min::to_json() const {
    nlohmann::json j;
    j["datetime"] = utils::time_point_to_string(datetime);
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
    StockCn1Min data;
    data.datetime = utils::string_to_time_point(j.at("datetime").get<std::string>());
    data.order_book_id = j.at("order_book_id").get<std::string>();
    data.open = j.at("open").get<float>();
    data.high = j.at("high").get<float>();
    data.low = j.at("low").get<float>();
    data.close = j.at("close").get<float>();
    data.volume = j.at("volume").get<float>();
    data.total_turnover = j.at("total_turnover").get<float>();
    return data;
}

UnifiedKline StockCn1Min::to_unified_kline() const {
    UnifiedKline kline;
    kline.order_book_id = order_book_id;
    kline.datetime = datetime;
    kline.market_type = MarketType::Stock;
    kline.frequency = Frequency::OneMin;
    kline.open = static_cast<double>(open);
    kline.high = static_cast<double>(high);
    kline.low = static_cast<double>(low);
    kline.close = static_cast<double>(close);
    kline.volume = static_cast<double>(volume);
    kline.total_turnover = static_cast<double>(total_turnover);
    return kline;
}

StockCn1Min StockCn1Min::from_unified_kline(const UnifiedKline& kline) {
    StockCn1Min data;
    data.datetime = kline.datetime;
    data.order_book_id = kline.order_book_id;
    data.open = static_cast<float>(kline.open);
    data.high = static_cast<float>(kline.high);
    data.low = static_cast<float>(kline.low);
    data.close = static_cast<float>(kline.close);
    data.volume = static_cast<float>(kline.volume);
    data.total_turnover = static_cast<float>(kline.total_turnover);
    return data;
}

// =======================
// FutureCn1Min 实现
// =======================

nlohmann::json FutureCn1Min::to_json() const {
    nlohmann::json j;
    j["datetime"] = utils::time_point_to_string(datetime);
    j["order_book_id"] = order_book_id;
    j["open"] = open;
    j["high"] = high;
    j["low"] = low;
    j["close"] = close;
    j["volume"] = volume;
    j["total_turnover"] = total_turnover;
    j["open_interest"] = open_interest;
    return j;
}

FutureCn1Min FutureCn1Min::from_json(const nlohmann::json& j) {
    FutureCn1Min data;
    data.datetime = utils::string_to_time_point(j.at("datetime").get<std::string>());
    data.order_book_id = j.at("order_book_id").get<std::string>();
    data.open = j.at("open").get<float>();
    data.high = j.at("high").get<float>();
    data.low = j.at("low").get<float>();
    data.close = j.at("close").get<float>();
    data.volume = j.at("volume").get<float>();
    data.total_turnover = j.at("total_turnover").get<float>();
    data.open_interest = j.at("open_interest").get<float>();
    return data;
}

UnifiedKline FutureCn1Min::to_unified_kline() const {
    UnifiedKline kline;
    kline.order_book_id = order_book_id;
    kline.datetime = datetime;
    kline.market_type = MarketType::Future;
    kline.frequency = Frequency::OneMin;
    kline.open = static_cast<double>(open);
    kline.high = static_cast<double>(high);
    kline.low = static_cast<double>(low);
    kline.close = static_cast<double>(close);
    kline.volume = static_cast<double>(volume);
    kline.total_turnover = static_cast<double>(total_turnover);
    return kline;
}

FutureCn1Min FutureCn1Min::from_unified_kline(const UnifiedKline& kline, float oi) {
    FutureCn1Min data;
    data.datetime = kline.datetime;
    data.order_book_id = kline.order_book_id;
    data.open = static_cast<float>(kline.open);
    data.high = static_cast<float>(kline.high);
    data.low = static_cast<float>(kline.low);
    data.close = static_cast<float>(kline.close);
    data.volume = static_cast<float>(kline.volume);
    data.total_turnover = static_cast<float>(kline.total_turnover);
    data.open_interest = oi;
    return data;
}

// =======================
// MarketDataContainer 实现
// =======================

void MarketDataContainer::add_data(const UnifiedKline& data) {
    data_.emplace_back(data);
}

void MarketDataContainer::add_data(const StockCnDay& data) {
    data_.emplace_back(data);
}

void MarketDataContainer::add_data(const StockCn1Min& data) {
    data_.emplace_back(data);
}

void MarketDataContainer::add_data(const FutureCn1Min& data) {
    data_.emplace_back(data);
}

template<typename T>
void MarketDataContainer::add_batch(const std::vector<T>& data_list) {
    data_.reserve(data_.size() + data_list.size());
    for (const auto& item : data_list) {
        add_data(item);
    }
}

// 显式实例化模板
template void MarketDataContainer::add_batch<UnifiedKline>(const std::vector<UnifiedKline>&);
template void MarketDataContainer::add_batch<StockCnDay>(const std::vector<StockCnDay>&);
template void MarketDataContainer::add_batch<StockCn1Min>(const std::vector<StockCn1Min>&);
template void MarketDataContainer::add_batch<FutureCn1Min>(const std::vector<FutureCn1Min>&);

std::vector<MarketDataVariant> MarketDataContainer::get_data_by_time_range(
    const time_point& start, const time_point& end) const {

    std::vector<MarketDataVariant> result;

    for (const auto& data : data_) {
        auto dt = utils::visit_market_data(data, [](const auto& d) {
            return d.get_datetime();
        });

        if (dt >= start && dt <= end) {
            result.push_back(data);
        }
    }

    return result;
}

MarketDataVariant MarketDataContainer::get_latest_data() const {
    if (data_.empty()) {
        throw std::runtime_error("No data available");
    }
    return data_.back();
}

std::optional<MarketDataVariant> MarketDataContainer::find_by_datetime(const time_point& dt) const {
    for (const auto& data : data_) {
        auto data_dt = utils::visit_market_data(data, [](const auto& d) {
            return d.get_datetime();
        });

        if (data_dt == dt) {
            return data;
        }
    }
    return std::nullopt;
}

std::vector<UnifiedKline> MarketDataContainer::to_unified_klines() const {
    std::vector<UnifiedKline> result;
    result.reserve(data_.size());

    for (const auto& data : data_) {
        result.push_back(utils::variant_to_unified_kline(data));
    }

    return result;
}

MarketDataContainer::Statistics MarketDataContainer::get_statistics() const {
    Statistics stats;

    if (data_.empty()) {
        return stats;
    }

    stats.total_count = data_.size();

    double min_price = std::numeric_limits<double>::max();
    double max_price = std::numeric_limits<double>::min();
    time_point start_time = std::chrono::system_clock::time_point::max();
    time_point end_time = std::chrono::system_clock::time_point::min();
    double total_vol = 0.0;
    double total_turn = 0.0;

    for (const auto& data : data_) {
        utils::visit_market_data(data, [&](const auto& d) {
            auto dt = d.get_datetime();
            auto high = d.get_high();
            auto low = d.get_low();
            auto vol = d.get_volume();
            auto turnover = d.get_turnover();

            if (dt < start_time) start_time = dt;
            if (dt > end_time) end_time = dt;
            if (high > max_price) max_price = high;
            if (low < min_price) min_price = low;
            total_vol += vol;
            total_turn += turnover;
        });
    }

    stats.start_time = start_time;
    stats.end_time = end_time;
    stats.max_price = max_price;
    stats.min_price = min_price;
    stats.total_volume = total_vol;
    stats.total_turnover = total_turn;

    return stats;
}

void MarketDataContainer::sort_by_datetime() {
    std::sort(data_.begin(), data_.end(), [](const auto& a, const auto& b) {
        auto dt_a = utils::visit_market_data(a, [](const auto& d) { return d.get_datetime(); });
        auto dt_b = utils::visit_market_data(b, [](const auto& d) { return d.get_datetime(); });
        return dt_a < dt_b;
    });
}

nlohmann::json MarketDataContainer::to_json() const {
    nlohmann::json j;
    j["code"] = code_;
    j["count"] = data_.size();
    j["data"] = nlohmann::json::array();

    for (const auto& data : data_) {
        utils::visit_market_data(data, [&j](const auto& d) {
            j["data"].push_back(d.to_json());
        });
    }

    return j;
}

MarketDataContainer MarketDataContainer::from_json(const nlohmann::json& j) {
    MarketDataContainer container;
    container.code_ = j.at("code").get<std::string>();

    // 这里需要根据数据类型来反序列化，简化处理
    // 实际应用中需要添加类型标识

    return container;
}

// =======================
// Utils 实现
// =======================

namespace utils {

time_point string_to_time_point(const std::string& datetime_str) {
    std::tm tm = {};
    std::istringstream ss(datetime_str);

    // 尝试多种格式
    if (ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S")) {
        // 成功
    } else {
        ss.clear();
        ss.str(datetime_str);
        if (ss >> std::get_time(&tm, "%Y-%m-%d")) {
            // 日期格式
        } else {
            throw std::invalid_argument("Invalid datetime format: " + datetime_str);
        }
    }

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string time_point_to_string(const time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

Date time_point_to_ymd(const time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return Date{tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday};
}

time_point ymd_to_time_point(const Date& ymd) {
    std::tm tm = {};
    tm.tm_year = ymd.year - 1900;
    tm.tm_mon = ymd.month - 1;
    tm.tm_mday = ymd.day;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

MarketType detect_market_type(const std::string& code) {
    if (code.empty()) return MarketType::Unknown;

    // 简化的市场类型检测
    if (code.find(".XSHE") != std::string::npos ||
        code.find(".XSHG") != std::string::npos) {
        return MarketType::Stock;
    } else if (code.find("IF") == 0 || code.find("IC") == 0 || code.find("IH") == 0) {
        return MarketType::Future;
    } else if (code.find("000") == 0 || code.find("399") == 0) {
        return MarketType::Index;
    }

    return MarketType::Stock; // 默认为股票
}

std::string market_type_to_string(MarketType type) {
    switch (type) {
        case MarketType::Stock: return "Stock";
        case MarketType::Future: return "Future";
        case MarketType::Index: return "Index";
        case MarketType::Bond: return "Bond";
        case MarketType::Option: return "Option";
        case MarketType::Fund: return "Fund";
        case MarketType::Currency: return "Currency";
        default: return "Unknown";
    }
}

MarketType string_to_market_type(const std::string& type_str) {
    if (type_str == "Stock") return MarketType::Stock;
    else if (type_str == "Future") return MarketType::Future;
    else if (type_str == "Index") return MarketType::Index;
    else if (type_str == "Bond") return MarketType::Bond;
    else if (type_str == "Option") return MarketType::Option;
    else if (type_str == "Fund") return MarketType::Fund;
    else if (type_str == "Currency") return MarketType::Currency;
    else return MarketType::Unknown;
}

std::string frequency_to_string(Frequency freq) {
    switch (freq) {
        case Frequency::Tick: return "Tick";
        case Frequency::OneMin: return "1Min";
        case Frequency::FiveMin: return "5Min";
        case Frequency::FifteenMin: return "15Min";
        case Frequency::ThirtyMin: return "30Min";
        case Frequency::OneHour: return "1Hour";
        case Frequency::Daily: return "Daily";
        case Frequency::Weekly: return "Weekly";
        case Frequency::Monthly: return "Monthly";
        case Frequency::Quarterly: return "Quarterly";
        case Frequency::Yearly: return "Yearly";
        default: return "Unknown";
    }
}

Frequency string_to_frequency(const std::string& freq_str) {
    if (freq_str == "Tick") return Frequency::Tick;
    else if (freq_str == "1Min") return Frequency::OneMin;
    else if (freq_str == "5Min") return Frequency::FiveMin;
    else if (freq_str == "15Min") return Frequency::FifteenMin;
    else if (freq_str == "30Min") return Frequency::ThirtyMin;
    else if (freq_str == "1Hour") return Frequency::OneHour;
    else if (freq_str == "Daily") return Frequency::Daily;
    else if (freq_str == "Weekly") return Frequency::Weekly;
    else if (freq_str == "Monthly") return Frequency::Monthly;
    else if (freq_str == "Quarterly") return Frequency::Quarterly;
    else if (freq_str == "Yearly") return Frequency::Yearly;
    else return Frequency::Daily; // 默认日线
}

bool validate_ohlcv(double o, double h, double l, double c, double v) {
    if (o < 0 || h < 0 || l < 0 || c < 0 || v < 0) return false;
    if (h < std::max({o, l, c}) || l > std::min({o, h, c})) return false;
    return true;
}

bool validate_kline_data(const IMarketData& data) {
    return validate_ohlcv(data.get_open(), data.get_high(),
                         data.get_low(), data.get_close(),
                         data.get_volume());
}

UnifiedKline variant_to_unified_kline(const MarketDataVariant& data) {
    return std::visit([](const auto& d) -> UnifiedKline {
        if constexpr (std::is_same_v<std::decay_t<decltype(d)>, UnifiedKline>) {
            return d;
        } else {
            return d.to_unified_kline();
        }
    }, data);
}

IMarketData& get_market_data_interface(MarketDataVariant& data) {
    return std::visit([](auto& d) -> IMarketData& {
        return static_cast<IMarketData&>(d);
    }, data);
}

const IMarketData& get_market_data_interface(const MarketDataVariant& data) {
    return std::visit([](const auto& d) -> const IMarketData& {
        return static_cast<const IMarketData&>(d);
    }, data);
}

} // namespace utils

} // namespace qaultra::data