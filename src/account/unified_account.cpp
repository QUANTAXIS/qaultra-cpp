#include "qaultra/account/unified_account.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace qaultra::account {

// =======================
// MarketPreset 实现
// =======================

MarketPreset::MarketPreset(const std::string& market_name) {
    name = market_name;
    if (market_name == "STOCK") {
        *this = get_stock_preset();
    } else if (market_name == "FUTURE") {
        *this = get_future_preset();
    } else if (market_name == "FOREX") {
        *this = get_forex_preset();
    }
}

MarketPreset MarketPreset::get_stock_preset() {
    MarketPreset preset;
    preset.name = "STOCK";
    preset.unit_table = 100;           // 一手100股
    preset.price_tick = 0.01;          // 价格最小变动单位
    preset.volume_tick = 100.0;        // 数量最小变动单位
    preset.buy_fee_ratio = 0.0003;     // 买入手续费率
    preset.sell_fee_ratio = 0.0003;    // 卖出手续费率
    preset.min_fee = 5.0;              // 最小手续费
    preset.tax_ratio = 0.001;          // 印花税率（仅卖出）
    preset.margin_ratio = 1.0;         // 全额保证金
    preset.is_stock = true;
    preset.allow_t0 = false;           // 不允许T+0
    preset.allow_sellopen = false;     // 不允许卖开
    return preset;
}

MarketPreset MarketPreset::get_future_preset() {
    MarketPreset preset;
    preset.name = "FUTURE";
    preset.unit_table = 1;             // 一手1张
    preset.price_tick = 0.2;           // 价格最小变动单位
    preset.volume_tick = 1.0;          // 数量最小变动单位
    preset.buy_fee_ratio = 0.00003;    // 开仓手续费率
    preset.sell_fee_ratio = 0.00003;   // 平仓手续费率
    preset.min_fee = 0.0;              // 最小手续费
    preset.tax_ratio = 0.0;            // 无印花税
    preset.margin_ratio = 0.1;         // 10%保证金
    preset.is_stock = false;
    preset.allow_t0 = true;            // 允许T+0
    preset.allow_sellopen = true;      // 允许卖开
    return preset;
}

MarketPreset MarketPreset::get_forex_preset() {
    MarketPreset preset;
    preset.name = "FOREX";
    preset.unit_table = 1;
    preset.price_tick = 0.0001;
    preset.volume_tick = 1000.0;       // 一标准手
    preset.buy_fee_ratio = 0.0001;
    preset.sell_fee_ratio = 0.0001;
    preset.min_fee = 0.0;
    preset.tax_ratio = 0.0;
    preset.margin_ratio = 0.01;        // 1%保证金
    preset.is_stock = false;
    preset.allow_t0 = true;
    preset.allow_sellopen = true;
    return preset;
}

nlohmann::json MarketPreset::to_json() const {
    nlohmann::json j;
    j["name"] = name;
    j["unit_table"] = unit_table;
    j["price_tick"] = price_tick;
    j["volume_tick"] = volume_tick;
    j["buy_fee_ratio"] = buy_fee_ratio;
    j["sell_fee_ratio"] = sell_fee_ratio;
    j["min_fee"] = min_fee;
    j["tax_ratio"] = tax_ratio;
    j["margin_ratio"] = margin_ratio;
    j["is_stock"] = is_stock;
    j["allow_t0"] = allow_t0;
    j["allow_sellopen"] = allow_sellopen;
    return j;
}

MarketPreset MarketPreset::from_json(const nlohmann::json& j) {
    MarketPreset preset;
    preset.name = j.at("name").get<std::string>();
    preset.unit_table = j.at("unit_table").get<int>();
    preset.price_tick = j.at("price_tick").get<double>();
    preset.volume_tick = j.at("volume_tick").get<double>();
    preset.buy_fee_ratio = j.at("buy_fee_ratio").get<double>();
    preset.sell_fee_ratio = j.at("sell_fee_ratio").get<double>();
    preset.min_fee = j.at("min_fee").get<double>();
    preset.tax_ratio = j.at("tax_ratio").get<double>();
    preset.margin_ratio = j.at("margin_ratio").get<double>();
    preset.is_stock = j.at("is_stock").get<bool>();
    preset.allow_t0 = j.at("allow_t0").get<bool>();
    preset.allow_sellopen = j.at("allow_sellopen").get<bool>();
    return preset;
}

// =======================
// Frozen 实现
// =======================

nlohmann::json Frozen::to_json() const {
    nlohmann::json j;
    j["money"] = money;
    j["order_id"] = order_id;
    j["datetime"] = datetime;
    j["code"] = code;
    return j;
}

Frozen Frozen::from_json(const nlohmann::json& j) {
    Frozen frozen;
    frozen.money = j.at("money").get<double>();
    frozen.order_id = j.at("order_id").get<std::string>();
    frozen.datetime = j.at("datetime").get<std::string>();
    frozen.code = j.at("code").get<std::string>();
    return frozen;
}

// =======================
// AccountSlice 实现
// =======================

nlohmann::json AccountSlice::to_json() const {
    nlohmann::json j;
    j["datetime"] = datetime;
    j["cash"] = cash;
    j["account_cookie"] = account_cookie;

    j["positions"] = nlohmann::json::object();
    for (const auto& [code, pos] : positions) {
        j["positions"][code] = pos.to_json();
    }

    j["pending_orders"] = nlohmann::json::array();
    for (const auto& order : pending_orders) {
        j["pending_orders"].push_back(order.to_json());
    }

    return j;
}

AccountSlice AccountSlice::from_json(const nlohmann::json& j) {
    AccountSlice slice;
    slice.datetime = j.at("datetime").get<std::string>();
    slice.cash = j.at("cash").get<double>();
    slice.account_cookie = j.at("account_cookie").get<std::string>();

    if (j.contains("positions")) {
        for (const auto& [code, pos_json] : j.at("positions").items()) {
            slice.positions[code] = Position::from_json(pos_json);
        }
    }

    if (j.contains("pending_orders")) {
        for (const auto& order_json : j.at("pending_orders")) {
            slice.pending_orders.push_back(Order::from_json(order_json));
        }
    }

    return slice;
}

// =======================
// UnifiedAccount 实现
// =======================

UnifiedAccount::UnifiedAccount(const std::string& account_cookie,
                               const std::string& portfolio_cookie,
                               const std::string& user_cookie,
                               double init_cash,
                               bool auto_reload)
    : account_cookie_(account_cookie)
    , portfolio_cookie_(portfolio_cookie)
    , user_cookie_(user_cookie)
    , init_cash_(init_cash)
    , auto_reload_(auto_reload)
    , cash_(init_cash)
    , frozen_cash_(0.0)
    , total_value_(init_cash)
    , float_pnl_(0.0)
    , order_id_counter_(0)
    , trade_id_counter_(0)
{
    market_preset_ = MarketPreset::get_stock_preset();
}

double UnifiedAccount::get_cash() const {
    return cash_.load();
}

double UnifiedAccount::get_frozen_cash() const {
    return frozen_cash_.load();
}

double UnifiedAccount::get_available_cash() const {
    return get_cash() - get_frozen_cash();
}

double UnifiedAccount::get_market_value() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    double market_value = 0.0;

    for (const auto& [code, position] : positions_) {
        auto price_it = market_prices_.find(code);
        double current_price = (price_it != market_prices_.end()) ?
                              price_it->second : position.price;
        market_value += position.volume * current_price;
    }

    return market_value;
}

double UnifiedAccount::get_total_value() const {
    return get_cash() + get_market_value();
}

double UnifiedAccount::get_pnl() const {
    return get_total_value() - init_cash_;
}

double UnifiedAccount::get_float_pnl() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    double pnl = 0.0;

    for (const auto& [code, position] : positions_) {
        auto price_it = market_prices_.find(code);
        if (price_it != market_prices_.end()) {
            double current_price = price_it->second;
            pnl += (current_price - position.price) * position.volume;
        }
    }

    float_pnl_.store(pnl);
    return pnl;
}

std::string UnifiedAccount::buy(const std::string& code, double volume, double price) {
    if (!validate_order_params(code, volume, price)) {
        return "";
    }

    Order order;
    order.order_id = generate_order_id();
    order.instrument_id = code;
    order.direction = "BUY";
    order.offset = market_preset_.is_stock ? "OPEN" : "OPEN";
    order.volume = volume;
    order.price = price;
    order.status = "PENDING";
    order.datetime = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    // 检查资金
    double required_cash = volume * (price > 0 ? price : market_prices_[code]) *
                          market_preset_.margin_ratio;
    double commission = calculate_commission(price > 0 ? price : market_prices_[code],
                                           volume, true);
    required_cash += commission;

    if (get_available_cash() < required_cash) {
        return "";  // 资金不足
    }

    // 冻结资金
    freeze_cash_for_order(order);

    // 添加到订单列表
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        orders_[order.order_id] = order;
    }

    trigger_order_callback(order);
    update_statistics(order);

    return order.order_id;
}

std::string UnifiedAccount::sell(const std::string& code, double volume, double price) {
    if (!validate_order_params(code, volume, price)) {
        return "";
    }

    // 检查持仓
    {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        auto pos_it = positions_.find(code);
        if (pos_it == positions_.end() || pos_it->second.volume < volume) {
            return "";  // 持仓不足
        }
    }

    Order order;
    order.order_id = generate_order_id();
    order.instrument_id = code;
    order.direction = "SELL";
    order.offset = market_preset_.is_stock ? "CLOSE" : "CLOSE";
    order.volume = volume;
    order.price = price;
    order.status = "PENDING";
    order.datetime = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        orders_[order.order_id] = order;
    }

    trigger_order_callback(order);
    update_statistics(order);

    return order.order_id;
}

// 期货专用操作
std::string UnifiedAccount::buy_open(const std::string& code, double volume, double price) {
    if (market_preset_.is_stock) {
        return buy(code, volume, price);  // 股票情况下等同于买入
    }
    // 期货买开逻辑
    return buy(code, volume, price);
}

std::string UnifiedAccount::sell_open(const std::string& code, double volume, double price) {
    if (market_preset_.is_stock || !market_preset_.allow_sellopen) {
        return "";  // 股票不允许卖开
    }
    // 期货卖开逻辑 - 建立空头仓位
    return sell(code, volume, price);
}

std::string UnifiedAccount::buy_close(const std::string& code, double volume, double price) {
    if (market_preset_.is_stock) {
        return "";  // 股票没有买平概念
    }
    // 期货买平 - 平空头仓位
    return buy(code, volume, price);
}

std::string UnifiedAccount::sell_close(const std::string& code, double volume, double price) {
    return sell(code, volume, price);  // 平多头仓位
}

std::string UnifiedAccount::buy_closetoday(const std::string& code, double volume, double price) {
    if (market_preset_.is_stock) {
        return "";
    }
    // 期货买平今 - 平今日空头仓位
    return buy_close(code, volume, price);
}

std::string UnifiedAccount::sell_closetoday(const std::string& code, double volume, double price) {
    if (market_preset_.is_stock) {
        return "";
    }
    // 期货卖平今 - 平今日多头仓位
    return sell_close(code, volume, price);
}

bool UnifiedAccount::cancel_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto order_it = orders_.find(order_id);
    if (order_it == orders_.end() || order_it->second.status != "PENDING") {
        return false;
    }

    // 解冻资金
    unfreeze_cash_for_order(order_it->second);

    // 更新订单状态
    order_it->second.status = "CANCELLED";
    trigger_order_callback(order_it->second);

    return true;
}

bool UnifiedAccount::cancel_all_orders() {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    bool success = true;

    for (auto& [order_id, order] : orders_) {
        if (order.status == "PENDING") {
            unfreeze_cash_for_order(order);
            order.status = "CANCELLED";
            trigger_order_callback(order);
        }
    }

    return success;
}

std::vector<Order> UnifiedAccount::get_pending_orders() const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> pending_orders;

    for (const auto& [order_id, order] : orders_) {
        if (order.status == "PENDING") {
            pending_orders.push_back(order);
        }
    }

    return pending_orders;
}

std::vector<Order> UnifiedAccount::get_filled_orders() const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> filled_orders;

    for (const auto& [order_id, order] : orders_) {
        if (order.status == "FILLED") {
            filled_orders.push_back(order);
        }
    }

    return filled_orders;
}

std::optional<Order> UnifiedAccount::find_order(const std::string& order_id) const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::unordered_map<std::string, Position> UnifiedAccount::get_positions() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    return positions_;
}

std::optional<Position> UnifiedAccount::get_position(const std::string& code) const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    auto it = positions_.find(code);
    if (it != positions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool UnifiedAccount::has_position(const std::string& code) const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    return positions_.find(code) != positions_.end();
}

void UnifiedAccount::add_trade(const std::string& order_id, double price, double volume,
                               const std::string& datetime) {
    // 查找对应订单
    Order* order = nullptr;
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto order_it = orders_.find(order_id);
        if (order_it == orders_.end()) {
            return;  // 订单不存在
        }
        order = &order_it->second;
    }

    // 生成成交记录
    std::string trade_id = generate_trade_id();
    std::string trade_datetime = datetime.empty() ?
        std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) : datetime;

    // 更新持仓
    update_position_from_trade(order->instrument_id, price, volume,
                              order->direction == "BUY");

    // 计算手续费和税费
    double commission = calculate_commission(price, volume, order->direction == "BUY");
    double tax = calculate_tax(price, volume);

    // 更新资金
    if (order->direction == "BUY") {
        cash_.fetch_sub(price * volume + commission);
    } else {
        cash_.fetch_add(price * volume - commission - tax);
    }

    // 解冻资金
    unfreeze_cash_for_order(*order);

    // 更新订单状态
    order->status = "FILLED";
    order->filled_volume += volume;

    // 添加到成交历史
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        trade_history_.push_back(trade_id + ":" + order_id + ":" +
                               std::to_string(price) + ":" + std::to_string(volume));
    }

    // 触发回调
    trigger_trade_callback(trade_id, price, volume);
    trigger_order_callback(*order);

    auto position = get_position(order->instrument_id);
    if (position.has_value()) {
        trigger_position_callback(order->instrument_id, position.value());
    }
}

void UnifiedAccount::update_market_data(const std::string& code, double price) {
    market_prices_[code] = price;
    calculate_pnl();  // 重新计算浮动盈亏
}

void UnifiedAccount::update_market_data_batch(const std::unordered_map<std::string, double>& prices) {
    for (const auto& [code, price] : prices) {
        market_prices_[code] = price;
    }
    calculate_pnl();
}

void UnifiedAccount::daily_settle() {
    // 日终结算逻辑
    calculate_pnl();

    // 保存当日切片
    auto slice = get_current_slice();
    save_slice(slice);

    // 对于期货，可能需要处理持仓的每日无负债结算
    if (!market_preset_.is_stock) {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        for (auto& [code, position] : positions_) {
            auto price_it = market_prices_.find(code);
            if (price_it != market_prices_.end()) {
                double current_price = price_it->second;
                double daily_pnl = (current_price - position.price) * position.volume;
                cash_.fetch_add(daily_pnl);
                position.price = current_price;  // 更新持仓成本
            }
        }
    }
}

void UnifiedAccount::calculate_pnl() {
    get_float_pnl();  // 更新浮动盈亏
    total_value_.store(get_total_value());  // 更新总资产
}

bool UnifiedAccount::check_risk_before_order(const Order& order) const {
    // 基础风险检查
    if (order.volume <= 0) return false;
    if (order.price < 0) return false;

    // 资金检查
    if (order.direction == "BUY") {
        double required_cash = order.volume_orign * order.price_order * market_preset_.margin_ratio;
        if (get_available_cash() < required_cash) {
            return false;
        }
    }

    // 持仓检查
    if (order.direction == "SELL" && order.offset == "CLOSE") {
        auto position = get_position(order.instrument_id);
        if (!position.has_value() || position->volume < order.volume) {
            return false;
        }
    }

    return true;
}

double UnifiedAccount::get_buying_power() const {
    return get_available_cash() / market_preset_.margin_ratio;
}

double UnifiedAccount::get_margin_usage() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    double margin_used = 0.0;

    for (const auto& [code, position] : positions_) {
        double position_value = position.volume * position.price;
        margin_used += position_value * market_preset_.margin_ratio;
    }

    return margin_used;
}

AccountSlice UnifiedAccount::get_current_slice() const {
    AccountSlice slice;
    slice.datetime = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    slice.cash = get_cash();
    slice.account_cookie = account_cookie_;
    slice.positions = get_positions();
    slice.pending_orders = get_pending_orders();
    return slice;
}

void UnifiedAccount::save_slice(const AccountSlice& slice) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    history_slices_.push_back(slice);
}

std::vector<AccountSlice> UnifiedAccount::get_history_slices() const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    return history_slices_;
}

protocol::qifi::QIFI UnifiedAccount::get_qifi() const {
    protocol::qifi::QIFI qifi;
    qifi.account_cookie = account_cookie_;
    qifi.portfolio = portfolio_cookie_;
    qifi.investor_name = user_cookie_;
    qifi.available = get_available_cash();
    qifi.balance = get_cash();
    qifi.market_value = get_market_value();
    qifi.close_profit = get_pnl();
    qifi.float_profit = get_float_pnl();

    // 转换持仓信息
    auto positions = get_positions();
    for (const auto& [code, position] : positions) {
        protocol::qifi::Position qifi_pos;
        qifi_pos.instrument_id = code;
        qifi_pos.volume_long = position.volume > 0 ? position.volume : 0;
        qifi_pos.volume_short = position.volume < 0 ? -position.volume : 0;
        qifi_pos.price_long = position.price;
        qifi_pos.price_short = position.price;
        qifi.positions[code] = qifi_pos;
    }

    return qifi;
}

void UnifiedAccount::from_qifi(const protocol::qifi::QIFI& qifi_data) {
    account_cookie_ = qifi_data.account_cookie;
    portfolio_cookie_ = qifi_data.portfolio;
    user_cookie_ = qifi_data.investor_name;
    cash_.store(qifi_data.balance);

    // 清空当前持仓
    {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        positions_.clear();

        // 从QIFI重建持仓
        for (const auto& [code, qifi_pos] : qifi_data.positions) {
            Position position;
            position.instrument_id = code;
            position.volume = qifi_pos.volume_long - qifi_pos.volume_short;
            position.price = position.volume > 0 ? qifi_pos.price_long : qifi_pos.price_short;
            positions_[code] = position;
        }
    }
}

bool UnifiedAccount::is_valid() const {
    return !account_cookie_.empty() && get_cash() >= 0;
}

std::string UnifiedAccount::get_status() const {
    std::ostringstream oss;
    oss << "Account[" << account_cookie_ << "] ";
    oss << "Cash:" << std::fixed << std::setprecision(2) << get_cash();
    oss << " MarketValue:" << get_market_value();
    oss << " TotalValue:" << get_total_value();
    oss << " PnL:" << get_pnl();
    return oss.str();
}

nlohmann::json UnifiedAccount::to_json() const {
    nlohmann::json j;
    j["account_cookie"] = account_cookie_;
    j["portfolio_cookie"] = portfolio_cookie_;
    j["user_cookie"] = user_cookie_;
    j["init_cash"] = init_cash_;
    j["cash"] = get_cash();
    j["frozen_cash"] = get_frozen_cash();
    j["total_value"] = get_total_value();
    j["float_pnl"] = get_float_pnl();

    j["market_preset"] = market_preset_.to_json();

    j["positions"] = nlohmann::json::object();
    auto positions = get_positions();
    for (const auto& [code, pos] : positions) {
        j["positions"][code] = pos.to_json();
    }

    j["pending_orders"] = nlohmann::json::array();
    auto pending = get_pending_orders();
    for (const auto& order : pending) {
        j["pending_orders"].push_back(order.to_json());
    }

    j["statistics"] = nlohmann::json::object();
    auto stats = get_statistics();
    j["statistics"]["total_orders"] = stats.total_orders;
    j["statistics"]["filled_orders"] = stats.filled_orders;
    j["statistics"]["total_commission"] = stats.total_commission;

    return j;
}

UnifiedAccount UnifiedAccount::from_json(const nlohmann::json& j) {
    std::string account_cookie = j.at("account_cookie").get<std::string>();
    std::string portfolio_cookie = j.value("portfolio_cookie", "");
    std::string user_cookie = j.value("user_cookie", "");
    double init_cash = j.at("init_cash").get<double>();

    UnifiedAccount account(account_cookie, portfolio_cookie, user_cookie, init_cash);

    if (j.contains("cash")) {
        account.cash_.store(j.at("cash").get<double>());
    }
    if (j.contains("market_preset")) {
        account.market_preset_ = MarketPreset::from_json(j.at("market_preset"));
    }

    return account;
}

UnifiedAccount::Statistics UnifiedAccount::get_statistics() const {
    return statistics_;
}

// 私有辅助方法
std::string UnifiedAccount::generate_order_id() {
    int counter = order_id_counter_.fetch_add(1);
    return account_cookie_ + "_O_" + std::to_string(counter);
}

std::string UnifiedAccount::generate_trade_id() {
    int counter = trade_id_counter_.fetch_add(1);
    return account_cookie_ + "_T_" + std::to_string(counter);
}

double UnifiedAccount::calculate_commission(double price, double volume, bool is_buy) const {
    double ratio = is_buy ? market_preset_.buy_fee_ratio : market_preset_.sell_fee_ratio;
    double commission = price * volume * ratio;
    return std::max(commission, market_preset_.min_fee);
}

double UnifiedAccount::calculate_tax(double price, double volume) const {
    // 只有股票卖出才收印花税
    if (market_preset_.is_stock && market_preset_.tax_ratio > 0) {
        return price * volume * market_preset_.tax_ratio;
    }
    return 0.0;
}

bool UnifiedAccount::validate_order_params(const std::string& code, double volume, double price) const {
    if (code.empty()) return false;
    if (volume <= 0) return false;
    if (price < 0) return false;

    // 检查最小交易单位
    if (std::fmod(volume, market_preset_.volume_tick) != 0.0) {
        return false;
    }

    return true;
}

void UnifiedAccount::update_position_from_trade(const std::string& code, double price,
                                               double volume, bool is_buy) {
    std::lock_guard<std::mutex> lock(positions_mutex_);

    auto pos_it = positions_.find(code);
    if (pos_it == positions_.end()) {
        // 新建仓位
        Position position;
        position.instrument_id = code;
        position.volume = is_buy ? volume : -volume;
        position.price = price;
        positions_[code] = position;
    } else {
        // 更新现有仓位
        Position& position = pos_it->second;
        double new_volume = position.volume + (is_buy ? volume : -volume);

        if (std::abs(new_volume) < 1e-9) {
            // 仓位平完，删除
            positions_.erase(pos_it);
        } else {
            // 更新仓位
            double current_vol = position.volume_long - position.volume_short;
            if ((current_vol > 0 && is_buy) || (current_vol < 0 && !is_buy)) {
                // 同向加仓，更新平均成本
                double current_price = (position.volume_long > 0) ? position.position_price_long : position.position_price_short;
                double total_value = current_vol * current_price + (is_buy ? volume : -volume) * price;
                if (is_buy) {
                    position.volume_long = std::max(0.0, new_volume);
                    position.volume_short = std::max(0.0, -new_volume);
                    if (position.volume_long > 0) position.position_price_long = total_value / position.volume_long;
                } else {
                    position.volume_long = std::max(0.0, new_volume);
                    position.volume_short = std::max(0.0, -new_volume);
                    if (position.volume_short > 0) position.position_price_short = total_value / position.volume_short;
                }
            } else {
                // 反向平仓
                if (is_buy) {
                    position.volume_long = std::max(0.0, new_volume);
                    position.volume_short = std::max(0.0, -new_volume);
                } else {
                    position.volume_long = std::max(0.0, new_volume);
                    position.volume_short = std::max(0.0, -new_volume);
                }
                double new_current_vol = position.volume_long - position.volume_short;
                double old_current_vol = pos_it->second.volume_long - pos_it->second.volume_short;
                if (std::abs(new_current_vol) > std::abs(old_current_vol)) {
                    // 超量平仓，变向
                    if (is_buy && position.volume_long > 0) {
                        position.position_price_long = price;
                    } else if (!is_buy && position.volume_short > 0) {
                        position.position_price_short = price;
                    }
                }
            }
        }
    }
}

void UnifiedAccount::freeze_cash_for_order(const Order& order) {
    if (order.direction == "BUY") {
        double freeze_amount = order.volume_orign * order.price_order * market_preset_.margin_ratio;
        frozen_cash_.store(frozen_cash_.load() + freeze_amount);
    }
}

void UnifiedAccount::unfreeze_cash_for_order(const Order& order) {
    if (order.direction == "BUY") {
        double unfreeze_amount = order.volume_orign * order.price_order * market_preset_.margin_ratio;
        frozen_cash_.store(frozen_cash_.load() - unfreeze_amount);
    }
}

void UnifiedAccount::trigger_order_callback(const Order& order) {
    if (order_callback_) {
        order_callback_(order);
    }
}

void UnifiedAccount::trigger_trade_callback(const std::string& trade_id, double price, double volume) {
    if (trade_callback_) {
        trade_callback_(trade_id, price, volume);
    }
}

void UnifiedAccount::trigger_position_callback(const std::string& code, const Position& position) {
    if (position_callback_) {
        position_callback_(code, position);
    }
}

void UnifiedAccount::update_statistics(const Order& order) {
    if (!performance_monitoring_) return;

    statistics_.total_orders++;
    if (order.status == "FILLED") {
        statistics_.filled_orders++;
    } else if (order.status == "CANCELLED") {
        statistics_.cancelled_orders++;
    }
}

// =======================
// AccountFactory 实现
// =======================

std::unique_ptr<UnifiedAccount> AccountFactory::create_account(
    AccountType type, const std::string& account_cookie, double init_cash) {

    auto account = std::make_unique<UnifiedAccount>(account_cookie, "", "", init_cash);

    switch (type) {
        case AccountType::Simple:
            // 简化配置
            account->enable_performance_monitoring(false);
            break;
        case AccountType::Full:
            // 完整配置
            account->enable_performance_monitoring(true);
            break;
        case AccountType::Unified:
            // 统一配置 - 平衡功能和性能
            account->enable_performance_monitoring(true);
            break;
    }

    return account;
}

std::unique_ptr<UnifiedAccount> AccountFactory::create_stock_account(
    const std::string& account_cookie, double init_cash) {

    auto account = std::make_unique<UnifiedAccount>(account_cookie, "", "", init_cash);
    account->set_market_preset(MarketPreset::get_stock_preset());
    return account;
}

std::unique_ptr<UnifiedAccount> AccountFactory::create_future_account(
    const std::string& account_cookie, double init_cash) {

    auto account = std::make_unique<UnifiedAccount>(account_cookie, "", "", init_cash);
    account->set_market_preset(MarketPreset::get_future_preset());
    return account;
}

std::unique_ptr<UnifiedAccount> AccountFactory::create_forex_account(
    const std::string& account_cookie, double init_cash) {

    auto account = std::make_unique<UnifiedAccount>(account_cookie, "", "", init_cash);
    account->set_market_preset(MarketPreset::get_forex_preset());
    return account;
}

std::unique_ptr<UnifiedAccount> AccountFactory::create_from_config(
    const nlohmann::json& config) {

    std::string account_cookie = config.at("account_cookie").get<std::string>();
    double init_cash = config.value("init_cash", 1000000.0);

    auto account = std::make_unique<UnifiedAccount>(account_cookie, "", "", init_cash);

    if (config.contains("market_preset")) {
        auto preset = MarketPreset::from_json(config.at("market_preset"));
        account->set_market_preset(preset);
    }

    return account;
}

std::unique_ptr<UnifiedAccount> AccountFactory::create_from_qifi(
    const protocol::qifi::QIFI& qifi_data) {

    auto account = std::make_unique<UnifiedAccount>(qifi_data.account_cookie);
    account->from_qifi(qifi_data);
    return account;
}

// =======================
// AccountManager 实现
// =======================

void AccountManager::add_account(std::unique_ptr<UnifiedAccount> account) {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    std::string account_cookie = account->get_account_cookie();
    accounts_[account_cookie] = std::move(account);
}

void AccountManager::remove_account(const std::string& account_cookie) {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    accounts_.erase(account_cookie);
}

UnifiedAccount* AccountManager::get_account(const std::string& account_cookie) {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    auto it = accounts_.find(account_cookie);
    return (it != accounts_.end()) ? it->second.get() : nullptr;
}

const UnifiedAccount* AccountManager::get_account(const std::string& account_cookie) const {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    auto it = accounts_.find(account_cookie);
    return (it != accounts_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> AccountManager::get_account_list() const {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    std::vector<std::string> list;
    list.reserve(accounts_.size());
    for (const auto& [cookie, account] : accounts_) {
        list.push_back(cookie);
    }
    return list;
}

size_t AccountManager::get_account_count() const {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    return accounts_.size();
}

void AccountManager::update_all_market_data(const std::unordered_map<std::string, double>& prices) {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    for (auto& [cookie, account] : accounts_) {
        account->update_market_data_batch(prices);
    }
}

void AccountManager::daily_settle_all() {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    for (auto& [cookie, account] : accounts_) {
        account->daily_settle();
    }
}

AccountManager::ManagerStatistics AccountManager::get_statistics() const {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    ManagerStatistics stats;
    stats.total_accounts = accounts_.size();

    for (const auto& [cookie, account] : accounts_) {
        stats.total_cash += account->get_cash();
        stats.total_market_value += account->get_market_value();
        stats.total_pnl += account->get_pnl();
    }

    return stats;
}

nlohmann::json AccountManager::to_json() const {
    nlohmann::json j;
    j["accounts"] = nlohmann::json::array();

    std::lock_guard<std::mutex> lock(accounts_mutex_);
    for (const auto& [cookie, account] : accounts_) {
        j["accounts"].push_back(account->to_json());
    }

    auto stats = get_statistics();
    j["statistics"] = {
        {"total_accounts", stats.total_accounts},
        {"total_cash", stats.total_cash},
        {"total_market_value", stats.total_market_value},
        {"total_pnl", stats.total_pnl}
    };

    return j;
}

AccountManager AccountManager::from_json(const nlohmann::json& j) {
    AccountManager manager;

    if (j.contains("accounts")) {
        for (const auto& account_json : j.at("accounts")) {
            auto temp_account = UnifiedAccount::from_json(account_json);
            auto account = std::make_unique<UnifiedAccount>(
                temp_account.get_account_cookie(),
                temp_account.get_portfolio_cookie(),
                temp_account.get_user_cookie(),
                temp_account.get_cash());
            manager.add_account(std::move(account));
        }
    }

    return manager;
}

void UnifiedAccount::set_market_preset(const MarketPreset& preset) {
    market_preset_ = preset;
}

std::vector<std::string> UnifiedAccount::get_trade_history() const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    return trade_history_;
}

} // namespace qaultra::account