#include "qaultra/market/simmarket.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <uuid/uuid.h>

namespace qaultra::market::simmarket {

// 工具函数实现
std::optional<SimMarketAsset> parse_asset(const std::string& asset) {
    if (asset == "IX2301") return SimMarketAsset::IX;
    if (asset == "IY2301") return SimMarketAsset::IY;
    if (asset == "IZ2301") return SimMarketAsset::IZ;
    return std::nullopt;
}

std::optional<matchengine::OrderDirection> parse_order_direction(const std::string& direction) {
    if (direction == "BUY") return matchengine::OrderDirection::BUY;
    if (direction == "SELL") return matchengine::OrderDirection::SELL;
    return std::nullopt;
}

std::string asset_to_string(SimMarketAsset asset) {
    switch (asset) {
        case SimMarketAsset::IX: return "IX2301";
        case SimMarketAsset::IY: return "IY2301";
        case SimMarketAsset::IZ: return "IZ2301";
        default: return "UNKNOWN";
    }
}

// TickSim 实现
std::tuple<std::string, double, double, double, double, double, double> TickSim::to_human() const {
    // 简化的时间戳转换，实际项目中应该使用更完整的日期解析
    std::time_t time = static_cast<std::time_t>(datetime / 1000000000);  // 纳秒转秒
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    return std::make_tuple(ss.str(), bid1, ask1, bid_volume1, ask_volume1, last_price, last_volume);
}

nlohmann::json TickSim::to_json() const {
    nlohmann::json j;
    j["datetime"] = datetime;
    j["bid1"] = bid1;
    j["ask1"] = ask1;
    j["bid_volume1"] = bid_volume1;
    j["ask_volume1"] = ask_volume1;
    j["last_price"] = last_price;
    j["last_volume"] = last_volume;
    return j;
}

TickSim TickSim::from_json(const nlohmann::json& j) {
    return TickSim(
        j.value("datetime", 0L),
        j.value("bid1", 0.0),
        j.value("ask1", 0.0),
        j.value("bid_volume1", 0.0),
        j.value("ask_volume1", 0.0),
        j.value("last_price", 0.0),
        j.value("last_volume", 0.0)
    );
}

// QASIMMarket 实现
QASIMMarket::QASIMMarket()
    : time_("")
    , portfolio_name_("")
    , trading_day_("") {
    init_default_orderbooks();
}

void QASIMMarket::init_default_orderbooks() {
    // 初始化默认的三个合约的订单簿 - 匹配Rust实现
    std::vector<std::string> default_codes = {"IX2301", "IY2301", "IZ2301"};

    for (const auto& code : default_codes) {
        auto asset = parse_asset(code);
        if (asset) {
            order_book_[code] = std::make_unique<matchengine::Orderbook<SimMarketAsset>>(*asset, 100.0);
            matching_e_id_[code] = std::unordered_map<uint64_t, account::Order>();
            lastpricepanel_[code] = 100.0;  // 默认初始价格
            hisprice_[code] = std::vector<TickSim>();
            lasttick_[code] = TickSim(get_current_timestamp(), 100.0, 100.0, 0.0, 0.0, 100.0, 0.0);
        }
    }
}

void QASIMMarket::register_account(const std::string& account_cookie,
                                 std::unique_ptr<account::QA_Account> account) {
    reg_account_[account_cookie] = std::move(account);
    std::cout << "账户注册成功: " << account_cookie << std::endl;
}

account::QA_Account* QASIMMarket::get_account(const std::string& account_cookie) {
    auto it = reg_account_.find(account_cookie);
    return (it != reg_account_.end()) ? it->second.get() : nullptr;
}

const account::QA_Account* QASIMMarket::get_account(const std::string& account_cookie) const {
    auto it = reg_account_.find(account_cookie);
    return (it != reg_account_.end()) ? it->second.get() : nullptr;
}

std::string QASIMMarket::order_send(const std::string& code,
                                   const std::string& direction,
                                   double price,
                                   double volume,
                                   const std::string& order_type,
                                   const std::string& account_cookie) {
    // 验证参数
    auto asset = parse_asset(code);
    auto order_direction = parse_order_direction(direction);

    if (!asset || !order_direction) {
        std::cerr << "无效的资产代码或订单方向: " << code << ", " << direction << std::endl;
        return "";
    }

    // 获取账户
    auto* account = get_account(account_cookie);
    if (!account) {
        std::cerr << "账户未找到: " << account_cookie << std::endl;
        return "";
    }

    // 获取订单簿
    auto orderbook_it = order_book_.find(code);
    if (orderbook_it == order_book_.end()) {
        std::cerr << "订单簿未找到: " << code << std::endl;
        return "";
    }

    auto* orderbook = orderbook_it->second.get();

    // 创建订单请求
    std::string order_id = generate_order_id();
    int64_t timestamp = get_current_timestamp();

    matchengine::OrderRequest<SimMarketAsset> request;

    if (order_type == "MARKET") {
        request = matchengine::OrderRequest<SimMarketAsset>::new_market_order(
            *asset, *order_direction, volume, timestamp);
    } else if (order_type == "LIMIT") {
        request = matchengine::OrderRequest<SimMarketAsset>::new_limit_order(
            *asset, *order_direction, price, volume, timestamp);
    } else if (order_type == "BEST") {
        request = matchengine::OrderRequest<SimMarketAsset>::new_best_order(
            *asset, *order_direction, volume, timestamp);
    } else {
        std::cerr << "不支持的订单类型: " << order_type << std::endl;
        return "";
    }

    // 创建本地订单记录
    account::Order local_order;
    local_order.order_id = order_id;
    local_order.code = code;
    local_order.direction = direction;
    local_order.price_order = price;
    local_order.volume_orign = volume;
    local_order.volume_left = volume;
    local_order.order_status = 11;  // 等待状态
    local_order.datetime = std::to_string(timestamp);

    // 处理订单
    auto results = orderbook->process_order(request);

    // 处理结果
    handle_order_results(results, account_cookie, code);

    // 存储订单映射
    matching_e_id_[code][std::stoull(order_id)] = local_order;

    std::cout << "订单发送成功: " << order_id << " [" << code << " " << direction
              << " " << price << " " << volume << "]" << std::endl;

    return order_id;
}

bool QASIMMarket::order_cancel(const std::string& order_id, const std::string& account_cookie) {
    // 验证账户
    auto* account = get_account(account_cookie);
    if (!account) {
        std::cerr << "账户未找到: " << account_cookie << std::endl;
        return false;
    }

    // 查找订单
    uint64_t order_id_num = std::stoull(order_id);
    std::string found_code;
    matchengine::OrderDirection found_direction = matchengine::OrderDirection::BUY;

    for (const auto& [code, orders] : matching_e_id_) {
        auto it = orders.find(order_id_num);
        if (it != orders.end()) {
            found_code = code;
            found_direction = (it->second.direction == "BUY") ?
                matchengine::OrderDirection::BUY : matchengine::OrderDirection::SELL;
            break;
        }
    }

    if (found_code.empty()) {
        std::cerr << "订单未找到: " << order_id << std::endl;
        return false;
    }

    // 获取订单簿并取消订单
    auto orderbook_it = order_book_.find(found_code);
    if (orderbook_it != order_book_.end()) {
        auto request = matchengine::OrderRequest<SimMarketAsset>::cancel_order(
            order_id_num, found_direction);

        auto results = orderbook_it->second->process_order(request);
        handle_order_results(results, account_cookie, found_code);

        // 移除订单映射
        matching_e_id_[found_code].erase(order_id_num);

        std::cout << "订单取消成功: " << order_id << std::endl;
        return true;
    }

    return false;
}

matchengine::Orderbook<SimMarketAsset>* QASIMMarket::get_orderbook(const std::string& code) {
    auto it = order_book_.find(code);
    return (it != order_book_.end()) ? it->second.get() : nullptr;
}

const matchengine::Orderbook<SimMarketAsset>* QASIMMarket::get_orderbook(const std::string& code) const {
    auto it = order_book_.find(code);
    return (it != order_book_.end()) ? it->second.get() : nullptr;
}

double QASIMMarket::get_last_price(const std::string& code) const {
    auto it = lastpricepanel_.find(code);
    if (it != lastpricepanel_.end()) {
        return it->second;
    }

    // 从订单簿获取最新价
    auto orderbook = get_orderbook(code);
    if (orderbook) {
        return orderbook->get_last_price();
    }

    return 0.0;
}

std::optional<TickSim> QASIMMarket::get_last_tick(const std::string& code) const {
    auto it = lasttick_.find(code);
    if (it != lasttick_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<TickSim> QASIMMarket::get_his_price(const std::string& code) const {
    auto it = hisprice_.find(code);
    if (it != hisprice_.end()) {
        return it->second;
    }
    return {};
}

void QASIMMarket::update_market(const std::string& code, const TickSim& tick) {
    // 更新最新tick
    lasttick_[code] = tick;

    // 添加到历史价格
    hisprice_[code].push_back(tick);

    // 更新价格面板
    update_price_panel(code, tick.last_price);

    // 更新订单簿最新价
    auto orderbook = get_orderbook(code);
    if (orderbook) {
        // 订单簿的最新价在撮合时自动更新，这里可以用于验证
    }
}

std::pair<std::optional<std::map<std::string, std::vector<double>>>,
          std::optional<std::map<std::string, std::vector<double>>>>
QASIMMarket::get_market_depth(const std::string& code) {
    auto orderbook = get_orderbook(code);
    if (orderbook) {
        return orderbook->get_full_depth();
    }
    return std::make_pair(std::nullopt, std::nullopt);
}

void QASIMMarket::display_all_orderbooks() {
    std::cout << "=== 所有订单簿状态 ===" << std::endl;
    for (const auto& [code, orderbook] : order_book_) {
        std::cout << "\n--- " << code << " ---" << std::endl;
        orderbook->display_full_depth();
    }
}

std::vector<std::string> QASIMMarket::get_registered_accounts() const {
    std::vector<std::string> accounts;
    accounts.reserve(reg_account_.size());
    for (const auto& [cookie, account] : reg_account_) {
        accounts.push_back(cookie);
    }
    return accounts;
}

std::vector<std::string> QASIMMarket::get_tradeable_codes() const {
    std::vector<std::string> codes;
    codes.reserve(order_book_.size());
    for (const auto& [code, orderbook] : order_book_) {
        codes.push_back(code);
    }
    return codes;
}

nlohmann::json QASIMMarket::to_json() const {
    nlohmann::json j;
    j["portfolio_name"] = portfolio_name_;
    j["trading_day"] = trading_day_;
    j["time"] = time_;

    // 序列化价格面板
    j["lastpricepanel"] = lastpricepanel_;

    // 序列化最新tick
    nlohmann::json lasttick_json;
    for (const auto& [code, tick] : lasttick_) {
        lasttick_json[code] = tick.to_json();
    }
    j["lasttick"] = lasttick_json;

    // 账户信息（简化）
    j["registered_accounts"] = get_registered_accounts();
    j["tradeable_codes"] = get_tradeable_codes();

    return j;
}

void QASIMMarket::handle_order_results(
    const std::vector<std::variant<matchengine::Success, matchengine::Failed>>& results,
    const std::string& account_cookie,
    const std::string& code) {

    auto* account = get_account(account_cookie);
    if (!account) return;

    for (const auto& result : results) {
        std::visit([&](const auto& res) {
            using T = std::decay_t<decltype(res)>;
            if constexpr (std::is_same_v<T, matchengine::Success>) {
                // 处理成功结果
                switch (res.type) {
                    case matchengine::Success::Accepted:
                        std::cout << "订单已接受: " << res.id << std::endl;
                        break;
                    case matchengine::Success::Filled:
                        std::cout << "订单完全成交: " << res.order_id
                                 << " 价格: " << res.price << " 数量: " << res.volume << std::endl;
                        update_price_panel(code, res.price);
                        break;
                    case matchengine::Success::PartiallyFilled:
                        std::cout << "订单部分成交: " << res.order_id
                                 << " 价格: " << res.price << " 数量: " << res.volume << std::endl;
                        update_price_panel(code, res.price);
                        break;
                    case matchengine::Success::Amended:
                        std::cout << "订单已修改: " << res.id << std::endl;
                        break;
                    case matchengine::Success::Cancelled:
                        std::cout << "订单已取消: " << res.id << std::endl;
                        break;
                }
            } else if constexpr (std::is_same_v<T, matchengine::Failed>) {
                // 处理失败结果
                switch (res.type) {
                    case matchengine::Failed::ValidationFailed:
                        std::cerr << "订单验证失败: " << res.message << std::endl;
                        break;
                    case matchengine::Failed::DuplicateOrderID:
                        std::cerr << "重复订单ID: " << res.order_id << std::endl;
                        break;
                    case matchengine::Failed::NoMatch:
                        std::cerr << "无匹配: " << res.order_id << std::endl;
                        break;
                    case matchengine::Failed::OrderNotFound:
                        std::cerr << "订单未找到: " << res.order_id << std::endl;
                        break;
                }
            }
        }, result);
    }
}

void QASIMMarket::update_price_panel(const std::string& code, double price) {
    lastpricepanel_[code] = price;

    // 更新最新tick
    auto it = lasttick_.find(code);
    if (it != lasttick_.end()) {
        it->second.last_price = price;
        it->second.datetime = get_current_timestamp();
    }
}

std::string QASIMMarket::generate_order_id() const {
    // 使用UUID生成唯一订单ID
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);

    // 转换为数字字符串（简化版本）
    std::hash<std::string> hasher;
    size_t hash = hasher(std::string(uuid_str));
    return std::to_string(hash);
}

int64_t QASIMMarket::get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace qaultra::market::simmarket