#include "../../include/qaultra/account/account.hpp"
#include "../../include/qaultra/account/position.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <regex>
#include <algorithm>

namespace qaultra::account {

// AccountSlice 实现
nlohmann::json AccountSlice::to_json() const {
    nlohmann::json j;
    j["datetime"] = datetime;
    j["cash"] = cash;
    j["accounts"] = accounts.to_json();

    nlohmann::json pos_json;
    for (const auto& [code, pos] : positions) {
        pos_json[code] = pos.to_json();
    }
    j["positions"] = pos_json;
    return j;
}

AccountSlice AccountSlice::from_json(const nlohmann::json& j) {
    AccountSlice slice;
    slice.datetime = j.at("datetime");
    slice.cash = j.at("cash");
    slice.accounts = protocol::Account::from_json(j.at("accounts"));

    if (j.contains("positions")) {
        for (const auto& [code, pos_json] : j.at("positions").items()) {
            slice.positions[code] = Position::from_json(pos_json);
        }
    }
    return slice;
}

// MOMSlice 实现
nlohmann::json MOMSlice::to_json() const {
    return nlohmann::json{
        {"datetime", datetime},
        {"user_id", user_id},
        {"pre_balance", pre_balance},
        {"close_profit", close_profit},
        {"commission", commission},
        {"position_profit", position_profit},
        {"float_profit", float_profit},
        {"balance", balance},
        {"margin", margin},
        {"available", available},
        {"risk_ratio", risk_ratio}
    };
}

MOMSlice MOMSlice::from_json(const nlohmann::json& j) {
    MOMSlice slice;
    slice.datetime = j.at("datetime");
    slice.user_id = j.at("user_id");
    slice.pre_balance = j.at("pre_balance");
    slice.close_profit = j.at("close_profit");
    slice.commission = j.at("commission");
    slice.position_profit = j.at("position_profit");
    slice.float_profit = j.at("float_profit");
    slice.balance = j.at("balance");
    slice.margin = j.at("margin");
    slice.available = j.at("available");
    slice.risk_ratio = j.at("risk_ratio");
    return slice;
}

// AccountInfo 实现
nlohmann::json AccountInfo::to_json() const {
    return nlohmann::json{
        {"datetime", datetime},
        {"balance", balance},
        {"account_cookie", account_cookie}
    };
}

AccountInfo AccountInfo::from_json(const nlohmann::json& j) {
    AccountInfo info;
    info.datetime = j.at("datetime");
    info.balance = j.at("balance");
    info.account_cookie = j.at("account_cookie");
    return info;
}

// Account 类实现
Account::Account(const std::string& account_cookie,
                 const std::string& portfolio_cookie,
                 const std::string& user_cookie,
                 double init_cash,
                 bool auto_reload,
                 const std::string& environment)
    : init_cash_(init_cash)
    , allow_t0_(false)
    , allow_sellopen_(false)
    , allow_margin_(false)
    , auto_reload_(auto_reload)
    , market_preset_(MarketPreset::create_default())
    , event_id_(0)
    , money_(init_cash)
    , account_cookie_(account_cookie)
    , portfolio_cookie_(portfolio_cookie)
    , user_cookie_(user_cookie)
    , environment_(environment)
    , commission_ratio_(0.00025)
    , tax_ratio_(0.001)
{
    // 设置当前时间 - 匹配Rust实现
    update_timestamp();

    // 初始化QIFI账户信息 - 完全匹配Rust实现
    accounts_.user_id = account_cookie;
    accounts_.currency = "CNY";
    accounts_.pre_balance = init_cash;
    accounts_.deposit = 0.0;
    accounts_.withdraw = 0.0;
    accounts_.WithdrawQuota = init_cash;
    accounts_.close_profit = 0.0;
    accounts_.commission = 0.0;
    accounts_.premium = 0.0;
    accounts_.static_balance = init_cash;
    accounts_.position_profit = 0.0;
    accounts_.float_profit = 0.0;
    accounts_.balance = init_cash;
    accounts_.margin = 0.0;
    accounts_.frozen_margin = 0.0;
    accounts_.frozen_commission = 0.0;
    accounts_.frozen_premium = 0.0;
    accounts_.available = init_cash;
    accounts_.risk_ratio = 0.0;

    // 创建算法管理器
    algo_manager_ = std::make_unique<algo::AlgoOrderManager>();

    if (auto_reload) {
        reload();
    }
}

Account Account::from_qifi(const protocol::QIFI& qifi) {
    Account account(
        qifi.account_cookie,
        qifi.portfolio,
        qifi.investor_name,
        qifi.accounts.available,
        false,
        "real"
    );

    // 复制账户信息 - 完全匹配Rust实现
    account.time_ = qifi.updatetime;
    account.accounts_ = qifi.accounts;
    account.money_ = qifi.money;
    account.frozen_ = qifi.frozen;
    account.daily_orders_ = qifi.orders;
    account.daily_trades_ = qifi.trades;

    // 转换持仓信息 - 匹配Rust实现
    for (const auto& [code, qifi_pos] : qifi.positions) {
        account.hold_[code] = Position::from_qifi(
            qifi.account_cookie,
            qifi.investor_name,
            qifi.account_cookie,
            qifi.portfolio,
            qifi_pos
        );
    }

    return account;
}

void Account::set_sellopen(bool sellopen) {
    allow_sellopen_ = sellopen;
}

void Account::set_t0(bool t0) {
    allow_t0_ = t0;
}

void Account::set_portfolio_cookie(const std::string& portfolio) {
    portfolio_cookie_ = portfolio;
}

void Account::reload() {
    // 重载逻辑，暂时为空，匹配Rust实现
}

double Account::get_frozen_margin() const {
    double total = 0.0;
    for (const auto& [_, frozen] : frozen_) {
        total += frozen.money;
    }
    return total;
}

double Account::get_risk_ratio() const {
    double balance = get_balance();
    if (balance <= 0.0) return 0.0;
    return get_margin() / balance;
}

double Account::get_balance() const {
    return money_ + get_position_profit() + get_float_profit();
}

double Account::get_margin() const {
    double total = 0.0;
    for (const auto& [_, pos] : hold_) {
        total += pos.margin();
    }
    return total;
}

double Account::get_position_profit() const {
    double total = 0.0;
    for (const auto& [_, pos] : hold_) {
        total += pos.position_profit();
    }
    return total;
}

double Account::get_float_profit() const {
    double total = 0.0;
    for (const auto& [_, pos] : hold_) {
        total += pos.float_profit();
    }
    return total;
}

MOMSlice Account::get_mom_slice() const {
    MOMSlice slice;
    slice.datetime = time_;
    slice.user_id = account_cookie_;
    slice.pre_balance = accounts_.pre_balance;
    slice.close_profit = accounts_.close_profit;
    slice.commission = accounts_.commission;
    slice.position_profit = get_position_profit();
    slice.float_profit = get_float_profit();
    slice.balance = get_balance();
    slice.margin = get_margin();
    slice.available = money_;
    slice.risk_ratio = get_risk_ratio();
    return slice;
}

protocol::QIFI Account::get_qifi_slice() const {
    protocol::QIFI qifi;
    qifi.account_cookie = account_cookie_;
    qifi.portfolio = portfolio_cookie_;
    qifi.investor_name = user_cookie_;
    qifi.broker_name = "QASIM";
    qifi.money = money_;
    qifi.updatetime = time_;
    qifi.bankname = "QASIM";
    qifi.trading_day = get_qifi_trading_day();
    qifi.status = 200;
    qifi.accounts = get_account_message();
    qifi.orders = daily_orders_;
    qifi.trades = daily_trades_;
    qifi.event = events_;
    qifi.frozen = frozen_;

    // 转换持仓信息
    for (const auto& [code, pos] : hold_) {
        qifi.positions[code] = pos.to_qifi();
    }

    return qifi;
}

protocol::Account Account::get_account_message() const {
    protocol::Account acc = accounts_;
    acc.position_profit = get_position_profit();
    acc.float_profit = get_float_profit();
    acc.balance = get_balance();
    acc.margin = get_margin();
    acc.frozen_margin = get_frozen_margin();
    acc.available = money_;
    acc.risk_ratio = get_risk_ratio();
    return acc;
}

AccountInfo Account::get_account_info() const {
    AccountInfo info;
    info.datetime = time_;
    info.balance = get_balance();
    info.account_cookie = account_cookie_;
    return info;
}

std::unordered_map<std::string, double> Account::get_account_pos_value() const {
    std::unordered_map<std::string, double> weight;

    for (const auto& [code, pos] : hold_) {
        double net_volume = pos.volume_long() - pos.volume_short();
        double value = pos.lastest_price * net_volume * pos.preset.unit_table;
        weight[code] = value;
    }

    weight["cash"] = money_;
    return weight;
}

std::unordered_map<std::string, double> Account::get_account_pos_weight() const {
    std::unordered_map<std::string, double> weight;
    double balance = get_balance();

    if (balance <= 0.0) return weight;

    for (const auto& [code, pos] : hold_) {
        double net_volume = pos.volume_long() - pos.volume_short();
        double value = pos.lastest_price * net_volume * pos.preset.unit_table;
        weight[code] = value / balance;
    }

    weight["cash"] = money_ / balance;
    return weight;
}

std::unordered_map<std::string, double> Account::get_account_pos_longshort() const {
    std::unordered_map<std::string, double> longshort;

    for (const auto& [code, pos] : hold_) {
        double net_volume = pos.volume_long() - pos.volume_short();
        if (net_volume != 0.0) {
            longshort[code] = net_volume;
        }
    }

    return longshort;
}

Position* Account::get_position(const std::string& code) {
    auto it = hold_.find(code);
    return (it != hold_.end()) ? &it->second : nullptr;
}

const Position* Account::get_position(const std::string& code) const {
    auto it = hold_.find(code);
    return (it != hold_.end()) ? &it->second : nullptr;
}

double Account::get_volume_long(const std::string& code) const {
    const Position* pos = get_position(code);
    return pos ? pos->volume_long() : 0.0;
}

double Account::get_volume_short(const std::string& code) const {
    const Position* pos = get_position(code);
    return pos ? pos->volume_short() : 0.0;
}

double Account::get_volume_avail(const std::string& code) const {
    const Position* pos = get_position(code);
    return pos ? pos->volume_long_avaliable() : 0.0;
}

void Account::on_price_change(const std::string& code, double new_price, const std::string& datetime) {
    Position* pos = get_position(code);
    if (pos) {
        pos->on_price_change(new_price, datetime);
    }
}

void Account::on_bar(const protocol::MarketData& bar) {
    on_price_change(bar.code, bar.close, bar.datetime);
}

void Account::transfer_event(const std::string& code, double amount) {
    // 转账事件处理 - 匹配Rust实现
    if (!hold_.count(code)) {
        init_position(code);
    }

    Position* pos = get_position(code);
    if (!pos) return;

    event_id_++;

    // 更新持仓
    if (amount > 0.0) {
        pos->volume_long_his += amount;
    } else if (amount < 0.0) {
        pos->volume_short_his += std::abs(amount);
    }

    events_[time_] = "transfer event code: " + code + " amount: " + std::to_string(amount);

    // 创建交易记录
    std::string order_id = generate_order_id();
    protocol::Trade trade;
    trade.seqno = event_id_;
    trade.user_id = account_cookie_;
    trade.price = 0.0;
    trade.order_id = order_id;
    trade.trade_id = order_id;
    trade.exchange_id = "";
    trade.commission = 0.0;
    trade.direction = (amount > 0.0) ? "BUY" : "SELL";
    trade.offset = "OPEN";
    trade.instrument_id = code;
    trade.exchange_trade_id = "";
    trade.volume = std::abs(amount);
    trade.trade_date_time = get_timestamp_nanos(time_);

    daily_trades_[order_id] = trade;
}

void Account::dividend_event(const std::string& code, double money_ratio) {
    // 分红事件 - 匹配Rust实现
    event_id_++;

    Position* pos = get_position(code);
    if (!pos) return;

    double money = (pos->volume_long_his - pos->volume_short_his) * money_ratio;
    money_ += money;

    events_[time_] = "dividend event code: " + code +
                     " ratio: " + std::to_string(money_ratio) +
                     " money: " + std::to_string(money);
}

std::optional<Order> Account::buy(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, 1, price, "", "LIMIT");
}

std::optional<Order> Account::sell(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, -1, price, "", "LIMIT");
}

std::optional<Order> Account::smart_buy(const std::string& code, double amount, const std::string& time, double price) {
    // 智能买入 - 完全匹配Rust实现
    std::string market_type = adjust_market(code);
    bool can_sellopen = (market_type == "stock_cn") ? allow_sellopen_ : true;

    if (hold_.count(code)) {
        if (!can_sellopen) {
            return buy(code, amount, time, price);
        } else {
            Position* pos = get_position(code);
            double short_pos = pos->volume_short();

            if (short_pos > amount) {
                return buy_close(code, amount, time, price);
            } else if (short_pos == 0.0) {
                return buy_open(code, amount, time, price);
            } else {
                // 先平掉空头，再开多头
                auto close_result = buy_close(code, short_pos, time, price);
                return buy_open(code, amount - short_pos, time, price);
            }
        }
    } else {
        return can_sellopen ? buy_open(code, amount, time, price) : buy(code, amount, time, price);
    }
}

std::optional<Order> Account::smart_sell(const std::string& code, double amount, const std::string& time, double price) {
    // 智能卖出 - 完全匹配Rust实现
    std::string market_type = adjust_market(code);
    bool can_sellopen = (market_type == "stock_cn") ? allow_sellopen_ : true;

    if (hold_.count(code)) {
        if (!can_sellopen) {
            return sell(code, amount, time, price);
        } else {
            Position* pos = get_position(code);
            double long_pos = pos->volume_long();

            if (long_pos > amount) {
                return sell_close(code, amount, time, price);
            } else if (long_pos == 0.0) {
                return sell_open(code, amount, time, price);
            } else {
                // 先平掉多头，再开空头
                auto close_result = sell_close(code, long_pos, time, price);
                return sell_open(code, amount - long_pos, time, price);
            }
        }
    } else {
        return can_sellopen ? sell_open(code, amount, time, price) : std::nullopt;
    }
}

std::optional<Order> Account::buy_open(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, 2, price, "", "LIMIT");
}

std::optional<Order> Account::sell_open(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, -2, price, "", "LIMIT");
}

std::optional<Order> Account::buy_close(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, 3, price, "", "LIMIT");
}

std::optional<Order> Account::sell_close(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, -3, price, "", "LIMIT");
}

std::optional<Order> Account::buy_closetoday(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, 4, price, "", "LIMIT");
}

std::optional<Order> Account::sell_closetoday(const std::string& code, double amount, const std::string& time, double price) {
    return send_order(code, amount, time, -4, price, "", "LIMIT");
}

std::optional<Order> Account::send_order(const std::string& code,
                                        double amount,
                                        const std::string& time,
                                        int towards,
                                        double price,
                                        const std::string& exchange,
                                        const std::string& order_type) {
    // 更新时间
    time_ = time;
    event_id_++;

    // 生成订单ID
    std::string order_id = generate_order_id();

    // 订单检查
    return order_check(code, amount, price, towards, order_id, time);
}

std::optional<Order> Account::insert_order(const std::string& code,
                                          double amount,
                                          const std::string& time,
                                          double price,
                                          const std::string& direction,
                                          const std::string& offset) {
    int towards = get_towards(direction, offset);
    return send_order(code, amount, time, towards, price, "", "LIMIT");
}

// 算法交易方法实现
std::string Account::create_algo_order(const std::string& code,
                                      double total_amount,
                                      double base_price,
                                      int direction,
                                      algo::SplitAlgorithm algorithm,
                                      std::optional<algo::SplitParams> params) {
    if (!algo_manager_) {
        algo_manager_ = std::make_unique<algo::AlgoOrderManager>();
    }

    return algo_manager_->create_split_order(code, total_amount, base_price, direction, algorithm, params);
}

bool Account::execute_next_algo_chunk(const std::string& algo_id) {
    if (!algo_manager_) {
        return false;
    }

    // 创建执行函数，将Account的交易方法绑定
    auto execute_func = [this](const std::string& code, double amount, const std::string& time,
                              double price, int direction) -> std::optional<std::pair<std::string, bool>> {
        std::optional<Order> result;

        switch (direction) {
            case 1:  // 买入
                result = this->buy(code, amount, time, price);
                break;
            case -1: // 卖出
                result = this->sell(code, amount, time, price);
                break;
            case 2:  // 买开
                result = this->buy_open(code, amount, time, price);
                break;
            case -2: // 卖开
                result = this->sell_open(code, amount, time, price);
                break;
            case 3:  // 买平
                result = this->buy_close(code, amount, time, price);
                break;
            case -3: // 卖平
                result = this->sell_close(code, amount, time, price);
                break;
            case 4:  // 买平今
                result = this->buy_closetoday(code, amount, time, price);
                break;
            case -4: // 卖平今
                result = this->sell_closetoday(code, amount, time, price);
                break;
            default:
                return std::nullopt;
        }

        if (result) {
            return std::make_pair(result->order_id, true);
        } else {
            return std::nullopt;
        }
    };

    auto result = algo_manager_->execute_next_chunk(algo_id, execute_func);
    return result && result->first;
}

bool Account::cancel_algo_order(const std::string& algo_id) {
    return algo_manager_ ? algo_manager_->cancel_split_order(algo_id) : false;
}

void Account::update_algo_orders(const std::string& datetime) {
    if (!algo_manager_) return;

    // 更新时间
    time_ = datetime;

    // 创建执行函数
    auto execute_func = [this](const std::string& code, double amount, const std::string& time,
                              double price, int direction) -> std::optional<std::pair<std::string, bool>> {
        std::optional<Order> result;

        switch (direction) {
            case 1:  result = this->buy(code, amount, time, price); break;
            case -1: result = this->sell(code, amount, time, price); break;
            case 2:  result = this->buy_open(code, amount, time, price); break;
            case -2: result = this->sell_open(code, amount, time, price); break;
            case 3:  result = this->buy_close(code, amount, time, price); break;
            case -3: result = this->sell_close(code, amount, time, price); break;
            case 4:  result = this->buy_closetoday(code, amount, time, price); break;
            case -4: result = this->sell_closetoday(code, amount, time, price); break;
            default: return std::nullopt;
        }

        return result ? std::make_optional(std::make_pair(result->order_id, true)) : std::nullopt;
    };

    algo_manager_->update_all_plans(execute_func);
}

std::vector<std::string> Account::get_active_algo_orders() const {
    if (!algo_manager_) {
        return {};
    }

    std::vector<std::string> active_orders;
    auto all_ids = algo_manager_->get_all_plan_ids();

    for (const auto& id : all_ids) {
        const auto* plan = algo_manager_->get_plan(id);
        if (plan && !plan->is_completed && !plan->is_cancelled) {
            active_orders.push_back(id);
        }
    }

    return active_orders;
}

algo::SplitOrderPlan* Account::get_algo_plan(const std::string& algo_id) {
    return algo_manager_ ? algo_manager_->get_plan(algo_id) : nullptr;
}

nlohmann::json Account::get_algo_status() const {
    return algo_manager_ ? algo_manager_->get_manager_status() : nlohmann::json{};
}

std::string Account::get_trading_day() const {
    // 简化的交易日计算，实际应该考虑节假日
    return time_.substr(0, 10);
}

std::string Account::get_qifi_trading_day() const {
    std::string trading_day = get_trading_day();
    // 移除连字符
    trading_day.erase(std::remove(trading_day.begin(), trading_day.end(), '-'), trading_day.end());
    return trading_day;
}

std::string Account::get_current_time() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Account::update_timestamp() {
    time_ = get_current_time();
}

void Account::settle() {
    // 日终结算 - 将今仓转为昨仓
    for (auto& [code, pos] : hold_) {
        pos.settle_position();
    }

    // 清空当日交易记录
    daily_trades_.clear();
    daily_orders_.clear();
}

void Account::message() const {
    std::cout << "Account: " << account_cookie_ << std::endl;
    std::cout << "Cash: " << money_ << std::endl;
    std::cout << "Balance: " << get_balance() << std::endl;
    std::cout << "Positions: " << hold_.size() << std::endl;
}

std::string Account::to_string() const {
    std::ostringstream oss;
    oss << "Account{cookie=" << account_cookie_
        << ", cash=" << money_
        << ", balance=" << get_balance()
        << ", positions=" << hold_.size() << "}";
    return oss.str();
}

nlohmann::json Account::to_json() const {
    nlohmann::json j;
    j["init_cash"] = init_cash_;
    j["allow_t0"] = allow_t0_;
    j["allow_sellopen"] = allow_sellopen_;
    j["allow_margin"] = allow_margin_;
    j["auto_reload"] = auto_reload_;
    j["time"] = time_;
    j["event_id"] = event_id_;
    j["accounts"] = accounts_.to_json();
    j["money"] = money_;
    j["account_cookie"] = account_cookie_;
    j["portfolio_cookie"] = portfolio_cookie_;
    j["user_cookie"] = user_cookie_;
    j["environment"] = environment_;
    j["commission_ratio"] = commission_ratio_;
    j["tax_ratio"] = tax_ratio_;

    // 持仓
    nlohmann::json hold_json;
    for (const auto& [code, pos] : hold_) {
        hold_json[code] = pos.to_json();
    }
    j["hold"] = hold_json;

    return j;
}

Account Account::from_json(const nlohmann::json& j) {
    Account account(
        j.at("account_cookie"),
        j.at("portfolio_cookie"),
        j.at("user_cookie"),
        j.at("init_cash"),
        j.value("auto_reload", false),
        j.value("environment", "backtest")
    );

    account.allow_t0_ = j.value("allow_t0", false);
    account.allow_sellopen_ = j.value("allow_sellopen", false);
    account.allow_margin_ = j.value("allow_margin", false);
    account.time_ = j.value("time", account.get_current_time());
    account.event_id_ = j.value("event_id", 0);
    account.money_ = j.value("money", account.init_cash_);
    account.commission_ratio_ = j.value("commission_ratio", 0.00025);
    account.tax_ratio_ = j.value("tax_ratio", 0.001);

    if (j.contains("accounts")) {
        account.accounts_ = protocol::Account::from_json(j.at("accounts"));
    }

    if (j.contains("hold")) {
        for (const auto& [code, pos_json] : j.at("hold").items()) {
            account.hold_[code] = Position::from_json(pos_json);
        }
    }

    return account;
}

// 私有辅助方法实现
void Account::init_position(const std::string& code) {
    hold_[code] = Position::new_position(code, account_cookie_, user_cookie_, portfolio_cookie_);
}

std::optional<Order> Account::order_check(const std::string& code,
                                          double amount,
                                          double price,
                                          int towards,
                                          const std::string& order_id,
                                          const std::string& datetime) {
    // 订单检查逻辑 - 匹配Rust实现
    if (amount == 0.0) {
        std::cerr << "Warning: amount is 0" << std::endl;
        return std::nullopt;
    }

    // 确保持仓存在
    if (!hold_.count(code)) {
        init_position(code);
    }

    Position* pos = get_position(code);
    if (!pos) return std::nullopt;

    // 688开头股票检查（科创板）
    if (code.find("688") == 0 && amount < 200.0) {
        std::cerr << "Warning: 688 market order amount must > 200" << std::endl;
        return std::nullopt;
    }

    // 平仓检查
    if (towards == 3 || towards == -3) {  // 买平/卖平
        double available = (towards == 3) ? pos->volume_short_avaliable() : pos->volume_long_avaliable();
        if (available < amount) {
            std::cerr << "Warning: 仓位不足" << std::endl;
            return std::nullopt;
        }
    }

    // 平今检查
    if (towards == 4 || towards == -4) {  // 买平今/卖平今
        double available_today = (towards == 4) ?
            (pos->volume_short_today - pos->volume_short_frozen_today) :
            (pos->volume_long_today - pos->volume_long_frozen_today);
        if (available_today < amount) {
            std::cerr << "Warning: 今日仓位不足" << std::endl;
            return std::nullopt;
        }
    }

    // 创建订单
    Order order(
        account_cookie_,
        code,
        towards,
        "",
        datetime,
        amount,
        price,
        order_id
    );

    return order;
}

int Account::get_towards(const std::string& direction, const std::string& offset) const {
    // 方向转换 - 匹配Rust实现
    if (direction == "BUY" && offset == "OPEN") return 2;
    if (direction == "SELL" && offset == "OPEN") return -2;
    if (direction == "BUY" && offset == "CLOSE") return 3;
    if (direction == "SELL" && offset == "CLOSE") return -3;
    if (direction == "BUY" && offset == "CLOSETODAY") return 4;
    if (direction == "SELL" && offset == "CLOSETODAY") return -4;
    if (direction == "BUY") return 1;
    if (direction == "SELL") return -1;
    return 0;
}

std::string Account::generate_order_id() const {
    int64_t timestamp = get_timestamp_nanos(time_) - 28800000000000;  // 减去8小时
    return std::to_string(timestamp) + "-" + std::to_string(event_id_);
}

int64_t Account::get_timestamp_nanos(const std::string& datetime) const {
    // 简化的时间戳生成，实际应该更精确
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

bool Account::check_order_rules(const std::string& code, double amount, int towards) const {
    // 订单规则检查
    return true;  // 简化实现
}

void Account::update_account_info() {
    accounts_.position_profit = get_position_profit();
    accounts_.float_profit = get_float_profit();
    accounts_.balance = get_balance();
    accounts_.margin = get_margin();
    accounts_.frozen_margin = get_frozen_margin();
    accounts_.available = money_;
    accounts_.risk_ratio = get_risk_ratio();
}

// AccountStats 实现
void AccountStats::update(const Account& account) {
    total_positions = account.hold_.size();
    active_positions = 0;
    total_market_value = 0.0;
    total_profit = account.get_position_profit() + account.get_float_profit();
    total_margin = account.get_margin();

    for (const auto& [code, pos] : account.hold_) {
        if (pos.has_position()) {
            active_positions++;
        }
        total_market_value += pos.market_value();
    }
}

void AccountStats::reset() {
    *this = AccountStats{};
}

nlohmann::json AccountStats::to_json() const {
    return nlohmann::json{
        {"total_positions", total_positions},
        {"active_positions", active_positions},
        {"total_market_value", total_market_value},
        {"total_profit", total_profit},
        {"total_margin", total_margin},
        {"max_drawdown", max_drawdown},
        {"sharpe_ratio", sharpe_ratio}
    };
}

} // namespace qaultra::account