#include "qaultra/account/position.hpp"
#include "qaultra/util/uuid_generator.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>
#include <iostream>
#include <cmath>

namespace qaultra::account {

// Position类实现 - 完全匹配Rust QA_Position实现

QA_Position::QA_Position(const std::string& code,
                   const std::string& account_cookie,
                   const std::string& user_id,
                   const std::string& portfolio_cookie)
    : code(code)
    , instrument_id(code)
    , user_id(user_id)
    , portfolio_cookie(portfolio_cookie)
    , account_cookie(account_cookie)
{
    position_id = util::UUIDGenerator::generate();
    username = user_id;
    market_type = adjust_market(code);
    exchange_id = get_exchange_from_code(code);
    name = code; // 简化处理
    update_timestamp();

    // 初始化预设配置
    preset = get_preset_for_market(market_type);
}

QA_Position QA_Position::new_position(const std::string& code,
                               const std::string& account_cookie,
                               const std::string& user_id,
                               const std::string& portfolio_cookie)
{
    return QA_Position(code, account_cookie, user_id, portfolio_cookie);
}

QA_Position QA_Position::from_qifi(const std::string& account_cookie,
                            const std::string& user_cookie,
                            const std::string& account_id,
                            const std::string& portfolio_cookie,
                            const protocol::QIFIPosition& qifi_pos)
{
    QA_Position pos;

    // 基础信息
    pos.code = qifi_pos.code;
    pos.instrument_id = qifi_pos.instrument_id;
    pos.user_id = user_cookie;
    pos.portfolio_cookie = portfolio_cookie;
    pos.username = user_cookie;
    pos.position_id = qifi_pos.position_id;
    pos.account_cookie = account_cookie;
    pos.frozen = qifi_pos.frozen;
    pos.name = qifi_pos.name;
    pos.exchange_id = qifi_pos.exchange_id;
    pos.market_type = qifi_pos.market_type;
    pos.lastupdatetime = qifi_pos.lastupdatetime;

    // 持仓量
    pos.volume_long_today = qifi_pos.volume_long_today;
    pos.volume_long_his = qifi_pos.volume_long_his;
    pos.volume_short_today = qifi_pos.volume_short_today;
    pos.volume_short_his = qifi_pos.volume_short_his;

    // 冻结量
    pos.volume_long_frozen_today = qifi_pos.volume_long_frozen_today;
    pos.volume_long_frozen_his = qifi_pos.volume_long_frozen_his;
    pos.volume_short_frozen_today = qifi_pos.volume_short_frozen_today;
    pos.volume_short_frozen_his = qifi_pos.volume_short_frozen_his;

    // 保证金
    pos.margin_long = qifi_pos.margin_long;
    pos.margin_short = qifi_pos.margin_short;

    // 持仓成本
    pos.position_price_long = qifi_pos.position_price_long;
    pos.position_cost_long = qifi_pos.position_cost_long;
    pos.position_price_short = qifi_pos.position_price_short;
    pos.position_cost_short = qifi_pos.position_cost_short;

    // 开仓成本
    pos.open_price_long = qifi_pos.open_price_long;
    pos.open_cost_long = qifi_pos.open_cost_long;
    pos.open_price_short = qifi_pos.open_price_short;
    pos.open_cost_short = qifi_pos.open_cost_short;

    // 最新价格
    pos.lastest_price = qifi_pos.lastest_price;
    pos.lastest_datetime = qifi_pos.lastest_datetime;

    return pos;
}

// 核心计算方法 - 完全匹配Rust实现

double QA_Position::volume_long() const {
    return volume_long_today + volume_long_his;
}

double QA_Position::volume_short() const {
    return volume_short_today + volume_short_his;
}

double QA_Position::volume_long_frozen() const {
    return volume_long_frozen_today + volume_long_frozen_his;
}

double QA_Position::volume_short_frozen() const {
    return volume_short_frozen_today + volume_short_frozen_his;
}

double QA_Position::volume_long_avaliable() const {
    return volume_long() - volume_long_frozen();
}

double QA_Position::volume_short_avaliable() const {
    return volume_short() - volume_short_frozen();
}

double QA_Position::volume_net() const {
    return volume_long() - volume_short();
}

double QA_Position::volume_total() const {
    return volume_long() + volume_short();
}

double QA_Position::market_value() const {
    if (lastest_price <= 0.0) return 0.0;

    if (market_type == "stock_cn") {
        // 股票市值 = 持股数量 * 最新价格
        return volume_long() * lastest_price;
    } else if (market_type == "future_cn") {
        // 期货市值 = (多头持仓 - 空头持仓) * 合约乘数 * 最新价格
        return volume_net() * preset.unit_table * lastest_price;
    }

    return 0.0;
}

double QA_Position::market_value_long() const {
    if (lastest_price <= 0.0 || volume_long() <= 0.0) return 0.0;
    return volume_long() * preset.unit_table * lastest_price;
}

double QA_Position::market_value_short() const {
    if (lastest_price <= 0.0 || volume_short() <= 0.0) return 0.0;
    return volume_short() * preset.unit_table * lastest_price;
}

// 盈亏计算 - 匹配Rust实现

double QA_Position::position_profit() const {
    return position_profit_long() + position_profit_short();
}

double QA_Position::position_profit_long() const {
    if (volume_long() <= 0.0 || position_price_long <= 0.0) return 0.0;

    // 持仓盈亏 = (当前价格 - 持仓均价) * 持仓量 * 合约乘数
    return (lastest_price - position_price_long) * volume_long() * preset.unit_table;
}

double QA_Position::position_profit_short() const {
    if (volume_short() <= 0.0 || position_price_short <= 0.0) return 0.0;

    // 空头盈亏 = (持仓均价 - 当前价格) * 持仓量 * 合约乘数
    return (position_price_short - lastest_price) * volume_short() * preset.unit_table;
}

double QA_Position::float_profit() const {
    return float_profit_long() + float_profit_short();
}

double QA_Position::float_profit_long() const {
    if (volume_long() <= 0.0 || open_price_long <= 0.0) return 0.0;

    // 浮动盈亏 = (当前价格 - 开仓均价) * 持仓量 * 合约乘数
    return (lastest_price - open_price_long) * volume_long() * preset.unit_table;
}

double QA_Position::float_profit_short() const {
    if (volume_short() <= 0.0 || open_price_short <= 0.0) return 0.0;

    // 空头浮动盈亏 = (开仓均价 - 当前价格) * 持仓量 * 合约乘数
    return (open_price_short - lastest_price) * volume_short() * preset.unit_table;
}

// 均价计算

double QA_Position::avg_price_long() const {
    if (volume_long() <= 0.0) return 0.0;
    return position_cost_long / (volume_long() * preset.unit_table);
}

double QA_Position::avg_price_short() const {
    if (volume_short() <= 0.0) return 0.0;
    return position_cost_short / (volume_short() * preset.unit_table);
}

// 保证金计算

double QA_Position::margin() const {
    return margin_long + margin_short;
}

double QA_Position::margin_required() const {
    if (market_type == "future_cn") {
        double long_margin = volume_long() * preset.unit_table * lastest_price * preset.margin_ratio;
        double short_margin = volume_short() * preset.unit_table * lastest_price * preset.margin_ratio;
        return long_margin + short_margin;
    }
    return 0.0; // 股票不需要保证金
}

// 交易操作方法 - 匹配Rust receive_deal方法

void QA_Position::receive_deal(const std::string& trade_id,
                           const std::string& direction,
                           const std::string& offset,
                           double volume,
                           double price,
                           const std::string& datetime)
{
    if (volume <= 0.0) return;

    // 更新时间
    lastupdatetime = datetime;

    if (direction == "BUY") {
        if (offset == "OPEN") {
            // 买开
            volume_long_today += volume;

            // 更新开仓成本和均价
            double old_cost = open_cost_long;
            double old_volume = volume_long() - volume;
            open_cost_long += volume * price * preset.unit_table;

            if (volume_long() > 0) {
                open_price_long = open_cost_long / (volume_long() * preset.unit_table);
            }

            // 更新持仓成本
            position_cost_long = open_cost_long;
            position_price_long = open_price_long;

        } else if (offset == "CLOSE") {
            // 买平（平空头仓）
            if (volume_short_his >= volume) {
                volume_short_his -= volume;
            } else {
                double his_volume = volume_short_his;
                volume_short_his = 0.0;
                volume_short_today -= (volume - his_volume);
            }

            // 重新计算空头均价
            if (volume_short() > 0) {
                position_cost_short = open_cost_short * (volume_short() / (volume_short() + volume));
                position_price_short = position_cost_short / (volume_short() * preset.unit_table);
            } else {
                position_cost_short = 0.0;
                position_price_short = 0.0;
                open_cost_short = 0.0;
                open_price_short = 0.0;
            }

        } else if (offset == "CLOSETODAY") {
            // 买平今
            volume_short_today -= std::min(volume, volume_short_today);

            // 重新计算空头均价
            if (volume_short() > 0) {
                position_price_short = position_cost_short / (volume_short() * preset.unit_table);
            } else {
                position_cost_short = 0.0;
                position_price_short = 0.0;
            }
        }

    } else if (direction == "SELL") {
        if (offset == "OPEN") {
            // 卖开
            volume_short_today += volume;

            // 更新开仓成本和均价
            open_cost_short += volume * price * preset.unit_table;

            if (volume_short() > 0) {
                open_price_short = open_cost_short / (volume_short() * preset.unit_table);
            }

            // 更新持仓成本
            position_cost_short = open_cost_short;
            position_price_short = open_price_short;

        } else if (offset == "CLOSE") {
            // 卖平（平多头仓）
            if (volume_long_his >= volume) {
                volume_long_his -= volume;
            } else {
                double his_volume = volume_long_his;
                volume_long_his = 0.0;
                volume_long_today -= (volume - his_volume);
            }

            // 重新计算多头均价
            if (volume_long() > 0) {
                position_cost_long = open_cost_long * (volume_long() / (volume_long() + volume));
                position_price_long = position_cost_long / (volume_long() * preset.unit_table);
            } else {
                position_cost_long = 0.0;
                position_price_long = 0.0;
                open_cost_long = 0.0;
                open_price_long = 0.0;
            }

        } else if (offset == "CLOSETODAY") {
            // 卖平今
            volume_long_today -= std::min(volume, volume_long_today);

            // 重新计算多头均价
            if (volume_long() > 0) {
                position_price_long = position_cost_long / (volume_long() * preset.unit_table);
            } else {
                position_cost_long = 0.0;
                position_price_long = 0.0;
            }
        }
    }

    // 重新计算保证金
    recalculate_margins();

    // 验证数据一致性
    validate_data();
}

void QA_Position::on_price_change(double new_price, const std::string& datetime) {
    lastest_price = new_price;
    lastest_datetime = datetime;
    update_timestamp();

    // 重新计算保证金
    recalculate_margins();
}

void QA_Position::freeze_position(const std::string& direction,
                              const std::string& offset,
                              double volume)
{
    if (direction == "BUY" && offset == "CLOSE") {
        // 买平冻结空头仓位
        volume_short_frozen_today += std::min(volume, volume_short_today);
        volume_short_frozen_his += std::max(0.0, volume - volume_short_today);
    } else if (direction == "SELL" && offset == "CLOSE") {
        // 卖平冻结多头仓位
        volume_long_frozen_today += std::min(volume, volume_long_today);
        volume_long_frozen_his += std::max(0.0, volume - volume_long_today);
    }
}

void QA_Position::unfreeze_position(const std::string& direction,
                                const std::string& offset,
                                double volume)
{
    if (direction == "BUY" && offset == "CLOSE") {
        // 解冻空头仓位
        volume_short_frozen_today -= std::min(volume, volume_short_frozen_today);
        volume_short_frozen_his -= std::max(0.0, volume - volume_short_frozen_today);
    } else if (direction == "SELL" && offset == "CLOSE") {
        // 解冻多头仓位
        volume_long_frozen_today -= std::min(volume, volume_long_frozen_today);
        volume_long_frozen_his -= std::max(0.0, volume - volume_long_frozen_today);
    }
}

// 查询方法

bool QA_Position::is_empty() const {
    return volume_long() <= 0.001 && volume_short() <= 0.001;
}

bool QA_Position::is_long() const {
    return volume_long() > 0.001;
}

bool QA_Position::is_short() const {
    return volume_short() > 0.001;
}

bool QA_Position::has_position() const {
    return !is_empty();
}

bool QA_Position::can_close_today(const std::string& direction, double volume) const {
    if (direction == "BUY") {
        return volume_short_today >= volume;
    } else {
        return volume_long_today >= volume;
    }
}

// 序列化方法

nlohmann::json QA_Position::to_json() const {
    nlohmann::json j;

    // 基础信息
    j["code"] = code;
    j["instrument_id"] = instrument_id;
    j["user_id"] = user_id;
    j["portfolio_cookie"] = portfolio_cookie;
    j["username"] = username;
    j["position_id"] = position_id;
    j["account_cookie"] = account_cookie;
    j["frozen"] = frozen;
    j["name"] = name;
    j["spms_id"] = spms_id;
    j["oms_id"] = oms_id;
    j["market_type"] = market_type;
    j["exchange_id"] = exchange_id;
    j["lastupdatetime"] = lastupdatetime;

    // 持仓量
    j["volume_long_today"] = volume_long_today;
    j["volume_long_his"] = volume_long_his;
    j["volume_short_today"] = volume_short_today;
    j["volume_short_his"] = volume_short_his;

    // 冻结量
    j["volume_long_frozen_today"] = volume_long_frozen_today;
    j["volume_long_frozen_his"] = volume_long_frozen_his;
    j["volume_short_frozen_today"] = volume_short_frozen_today;
    j["volume_short_frozen_his"] = volume_short_frozen_his;

    // 保证金
    j["margin_long"] = margin_long;
    j["margin_short"] = margin_short;

    // 持仓成本
    j["position_price_long"] = position_price_long;
    j["position_cost_long"] = position_cost_long;
    j["position_price_short"] = position_price_short;
    j["position_cost_short"] = position_cost_short;

    // 开仓成本
    j["open_price_long"] = open_price_long;
    j["open_cost_long"] = open_cost_long;
    j["open_price_short"] = open_price_short;
    j["open_cost_short"] = open_cost_short;

    // 最新价格
    j["lastest_price"] = lastest_price;
    j["lastest_datetime"] = lastest_datetime;

    // 预设配置
    j["preset"] = preset.to_json();

    return j;
}

QA_Position QA_Position::from_json(const nlohmann::json& j) {
    QA_Position pos;

    // 从JSON恢复所有字段
    pos.code = j.value("code", "");
    pos.instrument_id = j.value("instrument_id", "");
    pos.user_id = j.value("user_id", "");
    pos.portfolio_cookie = j.value("portfolio_cookie", "");
    pos.username = j.value("username", "");
    pos.position_id = j.value("position_id", "");
    pos.account_cookie = j.value("account_cookie", "");
    pos.frozen = j.value("frozen", 0.0);
    pos.name = j.value("name", "");
    pos.spms_id = j.value("spms_id", "");
    pos.oms_id = j.value("oms_id", "");
    pos.market_type = j.value("market_type", "");
    pos.exchange_id = j.value("exchange_id", "");
    pos.lastupdatetime = j.value("lastupdatetime", "");

    pos.volume_long_today = j.value("volume_long_today", 0.0);
    pos.volume_long_his = j.value("volume_long_his", 0.0);
    pos.volume_short_today = j.value("volume_short_today", 0.0);
    pos.volume_short_his = j.value("volume_short_his", 0.0);

    pos.volume_long_frozen_today = j.value("volume_long_frozen_today", 0.0);
    pos.volume_long_frozen_his = j.value("volume_long_frozen_his", 0.0);
    pos.volume_short_frozen_today = j.value("volume_short_frozen_today", 0.0);
    pos.volume_short_frozen_his = j.value("volume_short_frozen_his", 0.0);

    pos.margin_long = j.value("margin_long", 0.0);
    pos.margin_short = j.value("margin_short", 0.0);

    pos.position_price_long = j.value("position_price_long", 0.0);
    pos.position_cost_long = j.value("position_cost_long", 0.0);
    pos.position_price_short = j.value("position_price_short", 0.0);
    pos.position_cost_short = j.value("position_cost_short", 0.0);

    pos.open_price_long = j.value("open_price_long", 0.0);
    pos.open_cost_long = j.value("open_cost_long", 0.0);
    pos.open_price_short = j.value("open_price_short", 0.0);
    pos.open_cost_short = j.value("open_cost_short", 0.0);

    pos.lastest_price = j.value("lastest_price", 0.0);
    pos.lastest_datetime = j.value("lastest_datetime", "");

    if (j.contains("preset")) {
        pos.preset = QA_Position::CodePreset::from_json(j["preset"]);
    }

    return pos;
}

protocol::QIFIPosition QA_Position::to_qifi() const {
    protocol::QIFIPosition qifi;

    qifi.code = code;
    qifi.instrument_id = instrument_id;
    qifi.position_id = position_id;
    qifi.exchange_id = exchange_id;
    qifi.market_type = market_type;
    qifi.name = name;
    qifi.frozen = frozen;
    qifi.lastupdatetime = lastupdatetime;

    qifi.volume_long_today = volume_long_today;
    qifi.volume_long_his = volume_long_his;
    qifi.volume_short_today = volume_short_today;
    qifi.volume_short_his = volume_short_his;

    qifi.volume_long_frozen_today = volume_long_frozen_today;
    qifi.volume_long_frozen_his = volume_long_frozen_his;
    qifi.volume_short_frozen_today = volume_short_frozen_today;
    qifi.volume_short_frozen_his = volume_short_frozen_his;

    qifi.margin_long = margin_long;
    qifi.margin_short = margin_short;

    qifi.position_price_long = position_price_long;
    qifi.position_cost_long = position_cost_long;
    qifi.position_price_short = position_price_short;
    qifi.position_cost_short = position_cost_short;

    qifi.open_price_long = open_price_long;
    qifi.open_cost_long = open_cost_long;
    qifi.open_price_short = open_price_short;
    qifi.open_cost_short = open_cost_short;

    qifi.lastest_price = lastest_price;
    qifi.lastest_datetime = lastest_datetime;

    return qifi;
}

// 工具方法

std::string QA_Position::get_market_type() const {
    return market_type;
}

void QA_Position::update_timestamp() {
    lastupdatetime = get_current_time();
}

void QA_Position::settle_position() {
    // 日终结算：今日持仓转为历史持仓
    volume_long_his += volume_long_today;
    volume_long_today = 0.0;
    volume_short_his += volume_short_today;
    volume_short_today = 0.0;

    // 清零今日冻结
    volume_long_frozen_today = 0.0;
    volume_short_frozen_today = 0.0;

    update_timestamp();
}

void QA_Position::message() const {
    std::cout << "QA_Position Info for " << code << ":" << std::endl;
    std::cout << "  Long: " << volume_long() << " (Today: " << volume_long_today << ", His: " << volume_long_his << ")" << std::endl;
    std::cout << "  Short: " << volume_short() << " (Today: " << volume_short_today << ", His: " << volume_short_his << ")" << std::endl;
    std::cout << "  Net: " << volume_net() << std::endl;
    std::cout << "  Market Value: " << market_value() << std::endl;
    std::cout << "  Float Profit: " << float_profit() << std::endl;
    std::cout << "  Latest Price: " << lastest_price << std::endl;
}

std::string QA_Position::to_string() const {
    std::stringstream ss;
    ss << "QA_Position[" << code << "] ";
    ss << "Long:" << volume_long() << " ";
    ss << "Short:" << volume_short() << " ";
    ss << "Net:" << volume_net() << " ";
    ss << "Price:" << lastest_price << " ";
    ss << "PnL:" << float_profit();
    return ss.str();
}

// 私有方法实现

void QA_Position::update_position_costs() {
    if (volume_long() > 0) {
        position_price_long = position_cost_long / (volume_long() * preset.unit_table);
    } else {
        position_price_long = 0.0;
        position_cost_long = 0.0;
    }

    if (volume_short() > 0) {
        position_price_short = position_cost_short / (volume_short() * preset.unit_table);
    } else {
        position_price_short = 0.0;
        position_cost_short = 0.0;
    }
}

void QA_Position::update_open_costs() {
    if (volume_long() > 0) {
        open_price_long = open_cost_long / (volume_long() * preset.unit_table);
    } else {
        open_price_long = 0.0;
        open_cost_long = 0.0;
    }

    if (volume_short() > 0) {
        open_price_short = open_cost_short / (volume_short() * preset.unit_table);
    } else {
        open_price_short = 0.0;
        open_cost_short = 0.0;
    }
}

void QA_Position::recalculate_margins() {
    if (market_type == "future_cn") {
        margin_long = volume_long() * preset.unit_table * lastest_price * preset.margin_ratio;
        margin_short = volume_short() * preset.unit_table * lastest_price * preset.margin_ratio;
    } else {
        margin_long = 0.0;
        margin_short = 0.0;
    }
}

void QA_Position::validate_data() const {
    // 验证数据一致性（可添加断言或日志）
    if (volume_long_today < 0 || volume_long_his < 0 ||
        volume_short_today < 0 || volume_short_his < 0) {
        std::cerr << "Warning: Negative position volumes detected for " << code << std::endl;
    }
}

std::string QA_Position::get_current_time() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string QA_Position::adjust_market_type(const std::string& code) const {
    return adjust_market(code);
}

std::string QA_Position::get_exchange_from_code(const std::string& code) const {
    // 简化的交易所判断逻辑
    if (code.find("XSHG") != std::string::npos) return "XSHG";
    if (code.find("XSHE") != std::string::npos) return "XSHE";
    if (code.find(".SH") != std::string::npos) return "XSHG";
    if (code.find(".SZ") != std::string::npos) return "XSHE";

    // 期货交易所判断
    if (code.find("DCE") != std::string::npos) return "DCE";
    if (code.find("CZCE") != std::string::npos) return "CZCE";
    if (code.find("SHFE") != std::string::npos) return "SHFE";

    return "UNKNOWN";
}

QA_Position::CodePreset QA_Position::get_preset_for_market(const std::string& market_type) const {
    CodePreset preset;

    if (market_type == "stock_cn") {
        preset.name = "股票";
        preset.unit_table = 1;
        preset.price_tick = 0.01;
        preset.buy_fee_ratio = 0.0025;
        preset.sell_fee_ratio = 0.0025;
        preset.min_fee = 5.0;
        preset.margin_ratio = 0.0;
    } else if (market_type == "future_cn") {
        preset.name = "期货";
        preset.unit_table = 10;  // 默认合约乘数
        preset.price_tick = 1.0;
        preset.buy_fee_ratio = 0.0001;
        preset.sell_fee_ratio = 0.0001;
        preset.min_fee = 0.0;
        preset.margin_ratio = 0.1;  // 10%保证金
    }

    return preset;
}

// CodePreset实现

nlohmann::json QA_Position::CodePreset::to_json() const {
    nlohmann::json j;
    j["name"] = name;
    j["unit_table"] = unit_table;
    j["price_tick"] = price_tick;
    j["buy_fee_ratio"] = buy_fee_ratio;
    j["sell_fee_ratio"] = sell_fee_ratio;
    j["min_fee"] = min_fee;
    j["margin_ratio"] = margin_ratio;
    return j;
}

QA_Position::CodePreset QA_Position::CodePreset::from_json(const nlohmann::json& j) {
    CodePreset preset;
    preset.name = j.value("name", "");
    preset.unit_table = j.value("unit_table", 1);
    preset.price_tick = j.value("price_tick", 0.01);
    preset.buy_fee_ratio = j.value("buy_fee_ratio", 0.0);
    preset.sell_fee_ratio = j.value("sell_fee_ratio", 0.0);
    preset.min_fee = j.value("min_fee", 0.0);
    preset.margin_ratio = j.value("margin_ratio", 0.0);
    return preset;
}

// PositionStats实现

void PositionStats::update(const QA_Position& pos) {
    total_positions++;

    if (pos.has_position()) {
        active_positions++;
    }

    if (pos.is_long()) {
        long_positions++;
        total_volume_long += pos.volume_long();
    }

    if (pos.is_short()) {
        short_positions++;
        total_volume_short += pos.volume_short();
    }

    total_market_value += pos.market_value();
    total_float_profit += pos.float_profit();
    total_position_profit += pos.position_profit();
    total_margin += pos.margin();
}

void PositionStats::reset() {
    total_positions = 0;
    active_positions = 0;
    long_positions = 0;
    short_positions = 0;
    total_market_value = 0.0;
    total_float_profit = 0.0;
    total_position_profit = 0.0;
    total_margin = 0.0;
    total_volume_long = 0.0;
    total_volume_short = 0.0;
}

nlohmann::json PositionStats::to_json() const {
    nlohmann::json j;
    j["total_positions"] = total_positions;
    j["active_positions"] = active_positions;
    j["long_positions"] = long_positions;
    j["short_positions"] = short_positions;
    j["total_market_value"] = total_market_value;
    j["total_float_profit"] = total_float_profit;
    j["total_position_profit"] = total_position_profit;
    j["total_margin"] = total_margin;
    j["total_volume_long"] = total_volume_long;
    j["total_volume_short"] = total_volume_short;
    return j;
}

// 全局函数：市场类型判断 - 匹配Rust adjust_market函数

std::string adjust_market(const std::string& code) {
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

} // namespace qaultra::account