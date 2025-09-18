#include "../../include/qaultra/data/kline.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace qaultra::data {

// Kline implementation

Price Kline::typical_price() const {
    return (high + low + close) / 3.0;
}

Price Kline::weighted_close() const {
    return (high + low + close + close) / 4.0;
}

bool Kline::is_bullish() const {
    return close > open;
}

bool Kline::is_bearish() const {
    return close < open;
}

Price Kline::body_size() const {
    return std::abs(close - open);
}

Price Kline::upper_shadow() const {
    return high - std::max(open, close);
}

Price Kline::lower_shadow() const {
    return std::min(open, close) - low;
}

Price Kline::range() const {
    return high - low;
}

json Kline::to_json() const {
    auto time_t_value = std::chrono::system_clock::to_time_t(datetime);
    return json{
        {"code", code},
        {"open", open},
        {"high", high},
        {"low", low},
        {"close", close},
        {"volume", volume},
        {"amount", amount},
        {"datetime", time_t_value}
    };
}

Kline Kline::from_json(const json& j) {
    Kline kline;
    kline.code = j.at("code");
    kline.open = j.at("open");
    kline.high = j.at("high");
    kline.low = j.at("low");
    kline.close = j.at("close");
    kline.volume = j.at("volume");
    kline.amount = j.at("amount");

    auto time_t_value = j.at("datetime").get<std::time_t>();
    kline.datetime = std::chrono::system_clock::from_time_t(time_t_value);

    return kline;
}

bool Kline::operator==(const Kline& other) const {
    return code == other.code && datetime == other.datetime;
}

bool Kline::operator!=(const Kline& other) const {
    return !(*this == other);
}

bool Kline::operator<(const Kline& other) const {
    return datetime < other.datetime;
}

bool Kline::operator>(const Kline& other) const {
    return datetime > other.datetime;
}

std::string Kline::to_string() const {
    std::ostringstream oss;
    auto time_t_value = std::chrono::system_clock::to_time_t(datetime);
    oss << "Kline{code=" << code
        << ", datetime=" << std::put_time(std::localtime(&time_t_value), "%Y-%m-%d %H:%M:%S")
        << ", open=" << open << ", high=" << high << ", low=" << low
        << ", close=" << close << ", volume=" << volume << "}";
    return oss.str();
}

// KlineCollection implementation

KlineCollection::KlineCollection(std::vector<Kline> data) : data_(std::move(data)) {
    sort();
}

void KlineCollection::add(const Kline& kline) {
    data_.push_back(kline);
}

void KlineCollection::add(Kline&& kline) {
    data_.push_back(std::move(kline));
}

void KlineCollection::add_batch(const std::vector<Kline>& klines) {
    data_.insert(data_.end(), klines.begin(), klines.end());
}

void KlineCollection::add_batch(std::vector<Kline>&& klines) {
    data_.insert(data_.end(), std::make_move_iterator(klines.begin()),
                 std::make_move_iterator(klines.end()));
}

size_t KlineCollection::size() const {
    return data_.size();
}

bool KlineCollection::empty() const {
    return data_.empty();
}

const Kline& KlineCollection::operator[](size_t index) const {
    return data_[index];
}

Kline& KlineCollection::operator[](size_t index) {
    return data_[index];
}

auto KlineCollection::begin() -> decltype(data_.begin()) {
    return data_.begin();
}

auto KlineCollection::end() -> decltype(data_.end()) {
    return data_.end();
}

auto KlineCollection::begin() const -> decltype(data_.begin()) {
    return data_.begin();
}

auto KlineCollection::end() const -> decltype(data_.end()) {
    return data_.end();
}

const Kline& KlineCollection::latest() const {
    if (empty()) {
        throw std::runtime_error("KlineCollection is empty");
    }
    return data_.back();
}

Kline& KlineCollection::latest() {
    if (empty()) {
        throw std::runtime_error("KlineCollection is empty");
    }
    return data_.back();
}

KlineCollection KlineCollection::get_range(const Timestamp& start, const Timestamp& end) const {
    std::vector<Kline> result;
    for (const auto& kline : data_) {
        if (kline.datetime >= start && kline.datetime <= end) {
            result.push_back(kline);
        }
    }
    return KlineCollection(std::move(result));
}

KlineCollection KlineCollection::get_last(size_t count) const {
    if (count >= data_.size()) {
        return KlineCollection(data_);
    }

    std::vector<Kline> result(data_.end() - count, data_.end());
    return KlineCollection(std::move(result));
}

Price KlineCollection::max_price() const {
    if (empty()) return 0.0;

    return std::max_element(data_.begin(), data_.end(),
        [](const Kline& a, const Kline& b) {
            return a.high < b.high;
        })->high;
}

Price KlineCollection::min_price() const {
    if (empty()) return 0.0;

    return std::min_element(data_.begin(), data_.end(),
        [](const Kline& a, const Kline& b) {
            return a.low < b.low;
        })->low;
}

Price KlineCollection::avg_price() const {
    if (empty()) return 0.0;

    Price sum = std::accumulate(data_.begin(), data_.end(), 0.0,
        [](Price sum, const Kline& kline) {
            return sum + kline.close;
        });
    return sum / data_.size();
}

Volume KlineCollection::total_volume() const {
    return std::accumulate(data_.begin(), data_.end(), 0.0,
        [](Volume sum, const Kline& kline) {
            return sum + kline.volume;
        });
}

std::vector<Price> KlineCollection::get_closes() const {
    std::vector<Price> closes;
    closes.reserve(data_.size());
    for (const auto& kline : data_) {
        closes.push_back(kline.close);
    }
    return closes;
}

std::vector<Price> KlineCollection::get_highs() const {
    std::vector<Price> highs;
    highs.reserve(data_.size());
    for (const auto& kline : data_) {
        highs.push_back(kline.high);
    }
    return highs;
}

std::vector<Price> KlineCollection::get_lows() const {
    std::vector<Price> lows;
    lows.reserve(data_.size());
    for (const auto& kline : data_) {
        lows.push_back(kline.low);
    }
    return lows;
}

std::vector<Volume> KlineCollection::get_volumes() const {
    std::vector<Volume> volumes;
    volumes.reserve(data_.size());
    for (const auto& kline : data_) {
        volumes.push_back(kline.volume);
    }
    return volumes;
}

void KlineCollection::sort() {
    std::sort(data_.begin(), data_.end());
}

void KlineCollection::clear() {
    data_.clear();
}

json KlineCollection::to_json() const {
    json result = json::array();
    for (const auto& kline : data_) {
        result.push_back(kline.to_json());
    }
    return result;
}

KlineCollection KlineCollection::from_json(const json& j) {
    std::vector<Kline> klines;
    for (const auto& item : j) {
        klines.push_back(Kline::from_json(item));
    }
    return KlineCollection(std::move(klines));
}

} // namespace qaultra::data