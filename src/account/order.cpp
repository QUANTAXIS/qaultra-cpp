#include "qaultra/account/order.hpp"
#include "qaultra/util/uuid_generator.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>

namespace qaultra::account {

// Order类实现 - 完全匹配Rust QIFI实现

Order::Order(const std::string& account,
             const std::string& code,
             int towards_val,
             const std::string& exchange,
             const std::string& order_time_str,
             double volume,
             double price,
             const std::string& order_id_str)
    : account_cookie(account)
    , instrument_id(code)
    , secu_code(code)
    , exchange_id(exchange)
    , volume_orign(volume)
    , price_order(price)
    , volume_left(volume)
    , order_time(order_time_str)
    , towards(towards_val)
{
    // 生成订单ID
    order_id = order_id_str.empty() ? generate_order_id() : order_id_str;

    // 根据towards设置方向和开平
    if (towards_val > 0) {
        direction = "BUY";
    } else {
        direction = "SELL";
    }

    // 默认为OPEN，具体的开平逻辑由调用方设置
    offset = "OPEN";

    // 判断市场类型
    market_type = get_market_type_from_code(code);

    // 设置时间戳
    update_timestamp();

    // 设置初始状态
    status = "NEW";
    price_type = "LIMIT";
}

Order Order::from_qifi(const protocol::QIFIOrder& qifi_order) {
    Order order;
    order.order_id = qifi_order.order_id;
    order.account_cookie = qifi_order.account_cookie;
    order.user_cookie = qifi_order.user_cookie;
    order.portfolio_cookie = qifi_order.portfolio_cookie;
    order.instrument_id = qifi_order.instrument_id;
    order.secu_code = qifi_order.secu_code;
    order.exchange_id = qifi_order.exchange_id;
    order.direction = qifi_order.direction;
    order.offset = qifi_order.offset;
    order.volume_orign = qifi_order.volume_orign;
    order.price_order = qifi_order.price_order;
    order.price_type = qifi_order.price_type;
    order.status = qifi_order.status;
    order.volume_left = qifi_order.volume_left;
    order.volume_fill = qifi_order.volume_fill;
    order.price_fill = qifi_order.price_fill;
    order.fee = qifi_order.fee;
    order.tax = qifi_order.tax;
    order.order_time = qifi_order.order_time;
    order.cancel_time = qifi_order.cancel_time;
    order.trade_time = qifi_order.trade_time;
    order.last_update_time = qifi_order.last_update_time;
    order.reason = qifi_order.reason;
    order.error_message = qifi_order.error_message;
    order.towards = qifi_order.towards;
    order.exchange_order_id = qifi_order.exchange_order_id;
    order.market_type = qifi_order.market_type;

    return order;
}

protocol::QIFIOrder Order::to_qifi() const {
    protocol::QIFIOrder qifi_order;
    qifi_order.order_id = order_id;
    qifi_order.account_cookie = account_cookie;
    qifi_order.user_cookie = user_cookie;
    qifi_order.portfolio_cookie = portfolio_cookie;
    qifi_order.instrument_id = instrument_id;
    qifi_order.secu_code = secu_code;
    qifi_order.exchange_id = exchange_id;
    qifi_order.direction = direction;
    qifi_order.offset = offset;
    qifi_order.volume_orign = volume_orign;
    qifi_order.price_order = price_order;
    qifi_order.price_type = price_type;
    qifi_order.status = status;
    qifi_order.volume_left = volume_left;
    qifi_order.volume_fill = volume_fill;
    qifi_order.price_fill = price_fill;
    qifi_order.fee = fee;
    qifi_order.tax = tax;
    qifi_order.order_time = order_time;
    qifi_order.cancel_time = cancel_time;
    qifi_order.trade_time = trade_time;
    qifi_order.last_update_time = last_update_time;
    qifi_order.reason = reason;
    qifi_order.error_message = error_message;
    qifi_order.towards = towards;
    qifi_order.exchange_order_id = exchange_order_id;
    qifi_order.market_type = market_type;

    return qifi_order;
}

void Order::cancel() {
    if (can_cancel()) {
        status = "CANCELLED";
        cancel_time = get_current_time();
        update_timestamp();
    }
}

void Order::finish() {
    if (volume_left <= 0.001) { // 使用小的容差处理浮点数精度
        status = "FILLED";
        volume_left = 0.0;
    } else {
        status = "PARTIAL_FILLED";
    }
    update_timestamp();
}

void Order::update(double filled_volume) {
    if (filled_volume <= 0) return;

    double remaining_volume = volume_left;
    double actual_filled = std::min(filled_volume, remaining_volume);

    volume_fill += actual_filled;
    volume_left -= actual_filled;

    // 更新成交价格（加权平均）
    if (volume_fill > 0) {
        // 这里简化处理，实际应该根据具体的成交记录计算加权平均价
        if (price_fill == 0.0) {
            price_fill = price_order;
        }
    }

    // 计算手续费和税费
    calculate_fee_and_tax();

    // 更新状态
    if (volume_left <= 0.001) {
        status = "FILLED";
        volume_left = 0.0;
        trade_time = get_current_time();
    } else {
        status = "PARTIAL_FILLED";
    }

    update_timestamp();
}

void Order::reject(const std::string& reason_str) {
    status = "REJECTED";
    reason = reason_str;
    error_message = reason_str;
    update_timestamp();
}

bool Order::is_finished() const {
    return status == "FILLED" || status == "CANCELLED" || status == "REJECTED";
}

bool Order::is_active() const {
    return status == "NEW" || status == "ACCEPTED" || status == "PARTIAL_FILLED";
}

bool Order::can_cancel() const {
    return status == "NEW" || status == "ACCEPTED" || status == "PARTIAL_FILLED";
}

double Order::get_unfilled_volume() const {
    return volume_left;
}

double Order::get_filled_ratio() const {
    if (volume_orign <= 0) return 0.0;
    return volume_fill / volume_orign;
}

double Order::get_total_amount() const {
    return volume_orign * price_order;
}

double Order::get_filled_amount() const {
    return volume_fill * (price_fill > 0 ? price_fill : price_order);
}

OrderStatus Order::get_status_enum() const {
    return string_to_status(status);
}

Direction Order::get_direction_enum() const {
    return string_to_direction(direction);
}

std::string Order::get_market_type() const {
    return market_type;
}

nlohmann::json Order::to_json() const {
    nlohmann::json j;
    j["order_id"] = order_id;
    j["account_cookie"] = account_cookie;
    j["user_cookie"] = user_cookie;
    j["portfolio_cookie"] = portfolio_cookie;
    j["instrument_id"] = instrument_id;
    j["secu_code"] = secu_code;
    j["exchange_id"] = exchange_id;
    j["direction"] = direction;
    j["offset"] = offset;
    j["volume_orign"] = volume_orign;
    j["price_order"] = price_order;
    j["price_type"] = price_type;
    j["status"] = status;
    j["volume_left"] = volume_left;
    j["volume_fill"] = volume_fill;
    j["price_fill"] = price_fill;
    j["fee"] = fee;
    j["tax"] = tax;
    j["order_time"] = order_time;
    j["cancel_time"] = cancel_time;
    j["trade_time"] = trade_time;
    j["last_update_time"] = last_update_time;
    j["reason"] = reason;
    j["error_message"] = error_message;
    j["towards"] = towards;
    j["exchange_order_id"] = exchange_order_id;
    j["market_type"] = market_type;

    return j;
}

Order Order::from_json(const nlohmann::json& j) {
    Order order;
    order.order_id = j.value("order_id", "");
    order.account_cookie = j.value("account_cookie", "");
    order.user_cookie = j.value("user_cookie", "");
    order.portfolio_cookie = j.value("portfolio_cookie", "");
    order.instrument_id = j.value("instrument_id", "");
    order.secu_code = j.value("secu_code", "");
    order.exchange_id = j.value("exchange_id", "");
    order.direction = j.value("direction", "BUY");
    order.offset = j.value("offset", "OPEN");
    order.volume_orign = j.value("volume_orign", 0.0);
    order.price_order = j.value("price_order", 0.0);
    order.price_type = j.value("price_type", "LIMIT");
    order.status = j.value("status", "NEW");
    order.volume_left = j.value("volume_left", 0.0);
    order.volume_fill = j.value("volume_fill", 0.0);
    order.price_fill = j.value("price_fill", 0.0);
    order.fee = j.value("fee", 0.0);
    order.tax = j.value("tax", 0.0);
    order.order_time = j.value("order_time", "");
    order.cancel_time = j.value("cancel_time", "");
    order.trade_time = j.value("trade_time", "");
    order.last_update_time = j.value("last_update_time", "");
    order.reason = j.value("reason", "");
    order.error_message = j.value("error_message", "");
    order.towards = j.value("towards", 1);
    order.exchange_order_id = j.value("exchange_order_id", "");
    order.market_type = j.value("market_type", "");

    return order;
}

std::string Order::generate_order_id() {
    return util::UUIDGenerator::generate();
}

std::string Order::direction_to_string(Direction dir) {
    switch (dir) {
        case Direction::BUY: return "BUY";
        case Direction::SELL: return "SELL";
        default: return "BUY";
    }
}

Direction Order::string_to_direction(const std::string& dir) {
    if (dir == "SELL") return Direction::SELL;
    return Direction::BUY;
}

std::string Order::status_to_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::PARTIAL_FILLED: return "PARTIAL_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::REJECTED: return "REJECTED";
        case OrderStatus::PENDING_CANCEL: return "PENDING_CANCEL";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::PENDING_NEW: return "PENDING_NEW";
        case OrderStatus::CALCULATED: return "CALCULATED";
        case OrderStatus::ACCEPTED: return "ACCEPTED";
        case OrderStatus::SUSPENDED: return "SUSPENDED";
        default: return "UNKNOWN";
    }
}

OrderStatus Order::string_to_status(const std::string& status_str) {
    if (status_str == "NEW") return OrderStatus::NEW;
    if (status_str == "PARTIAL_FILLED") return OrderStatus::PARTIAL_FILLED;
    if (status_str == "FILLED") return OrderStatus::FILLED;
    if (status_str == "REJECTED") return OrderStatus::REJECTED;
    if (status_str == "PENDING_CANCEL") return OrderStatus::PENDING_CANCEL;
    if (status_str == "CANCELLED") return OrderStatus::CANCELLED;
    if (status_str == "PENDING_NEW") return OrderStatus::PENDING_NEW;
    if (status_str == "CALCULATED") return OrderStatus::CALCULATED;
    if (status_str == "ACCEPTED") return OrderStatus::ACCEPTED;
    if (status_str == "SUSPENDED") return OrderStatus::SUSPENDED;
    return OrderStatus::UNKNOWN;
}

int Order::get_towards_from_direction(const std::string& direction, const std::string& offset) {
    // 匹配Rust实现的逻辑
    if (direction == "BUY") {
        return offset == "CLOSE" ? -1 : 1;
    } else {
        return offset == "CLOSE" ? 1 : -1;
    }
}

void Order::update_timestamp() {
    last_update_time = get_current_time();
}

void Order::calculate_fee_and_tax() {
    // 根据市场类型计算手续费和税费
    // 这里需要结合MarketPreset的配置
    double filled_amount = get_filled_amount();

    if (market_type == "stock_cn") {
        // 股票市场：买入手续费，卖出手续费+印花税
        fee = filled_amount * 0.0025; // 0.25% 手续费
        fee = std::max(fee, 5.0);      // 最低5元

        if (direction == "SELL") {
            tax = filled_amount * 0.001; // 0.1% 印花税
        }
    } else if (market_type == "future_cn") {
        // 期货市场：开仓和平仓都有手续费
        fee = volume_fill * 0.0001;    // 按手数计算
    }
}

std::string Order::get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Order::get_market_type_from_code(const std::string& code) {
    // 匹配Rust中的adjust_market函数逻辑
    std::regex re("[a-zA-Z]+");
    std::smatch matches;

    if (std::regex_search(code, matches, re)) {
        std::string market = matches[0].str();
        if (market == "XSHG" || market == "XSHE") {
            return "stock_cn";
        }
        return "future_cn";
    }

    return "stock_cn"; // 默认为股票市场
}

// OrderStats实现

void OrderStats::update(const Order& order) {
    total_orders++;
    total_volume += order.volume_orign;
    total_amount += order.get_total_amount();

    if (order.is_active()) {
        active_orders++;
    }

    if (order.status == "FILLED") {
        filled_orders++;
        filled_volume += order.volume_fill;
        filled_amount += order.get_filled_amount();
    } else if (order.status == "CANCELLED") {
        cancelled_orders++;
    } else if (order.status == "REJECTED") {
        rejected_orders++;
    }

    total_fee += order.fee;
    total_tax += order.tax;
}

void OrderStats::reset() {
    total_orders = 0;
    active_orders = 0;
    filled_orders = 0;
    cancelled_orders = 0;
    rejected_orders = 0;
    total_volume = 0.0;
    filled_volume = 0.0;
    total_amount = 0.0;
    filled_amount = 0.0;
    total_fee = 0.0;
    total_tax = 0.0;
}

double OrderStats::get_fill_rate() const {
    if (total_volume <= 0) return 0.0;
    return filled_volume / total_volume;
}

double OrderStats::get_fill_amount_rate() const {
    if (total_amount <= 0) return 0.0;
    return filled_amount / total_amount;
}

nlohmann::json OrderStats::to_json() const {
    nlohmann::json j;
    j["total_orders"] = total_orders;
    j["active_orders"] = active_orders;
    j["filled_orders"] = filled_orders;
    j["cancelled_orders"] = cancelled_orders;
    j["rejected_orders"] = rejected_orders;
    j["total_volume"] = total_volume;
    j["filled_volume"] = filled_volume;
    j["total_amount"] = total_amount;
    j["filled_amount"] = filled_amount;
    j["total_fee"] = total_fee;
    j["total_tax"] = total_tax;
    j["fill_rate"] = get_fill_rate();
    j["fill_amount_rate"] = get_fill_amount_rate();

    return j;
}

// Order Factory 实现
namespace order_factory {

std::unique_ptr<Order> create_stock_buy_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, 1, "XSHG", order_time, volume, price, "");
    order->direction = "BUY";
    order->offset = "OPEN";
    order->market_type = "stock_cn";
    return order;
}

std::unique_ptr<Order> create_stock_sell_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, -1, "XSHG", order_time, volume, price, "");
    order->direction = "SELL";
    order->offset = "CLOSE";
    order->market_type = "stock_cn";
    return order;
}

std::unique_ptr<Order> create_future_buy_open_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, 1, "DCE", order_time, volume, price, "");
    order->direction = "BUY";
    order->offset = "OPEN";
    order->market_type = "future_cn";
    return order;
}

std::unique_ptr<Order> create_future_sell_open_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, -1, "DCE", order_time, volume, price, "");
    order->direction = "SELL";
    order->offset = "OPEN";
    order->market_type = "future_cn";
    return order;
}

std::unique_ptr<Order> create_future_buy_close_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, -1, "DCE", order_time, volume, price, "");
    order->direction = "BUY";
    order->offset = "CLOSE";
    order->market_type = "future_cn";
    return order;
}

std::unique_ptr<Order> create_future_sell_close_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, 1, "DCE", order_time, volume, price, "");
    order->direction = "SELL";
    order->offset = "CLOSE";
    order->market_type = "future_cn";
    return order;
}

std::unique_ptr<Order> create_future_buy_closetoday_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, -1, "DCE", order_time, volume, price, "");
    order->direction = "BUY";
    order->offset = "CLOSETODAY";
    order->market_type = "future_cn";
    return order;
}

std::unique_ptr<Order> create_future_sell_closetoday_order(
    const std::string& account_cookie,
    const std::string& code,
    double volume,
    double price,
    const std::string& order_time)
{
    auto order = std::make_unique<Order>(account_cookie, code, 1, "DCE", order_time, volume, price, "");
    order->direction = "SELL";
    order->offset = "CLOSETODAY";
    order->market_type = "future_cn";
    return order;
}

} // namespace order_factory

} // namespace qaultra::account