#pragma once

#include "matchengine/orderbook.hpp"
#include "../account/qa_account.hpp"
#include "../account/order.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace qaultra::market::simmarket {

/**
 * @brief 模拟市场资产枚举 - 完全匹配Rust SimMarketAsset
 */
enum class SimMarketAsset {
    IX,
    IY,
    IZ
};

/**
 * @brief 解析资产代码 - 匹配Rust parse_asset函数
 */
std::optional<SimMarketAsset> parse_asset(const std::string& asset);

/**
 * @brief 解析订单方向 - 匹配Rust parse_order_direction函数
 */
std::optional<matchengine::OrderDirection> parse_order_direction(const std::string& direction);

/**
 * @brief 资产枚举转换为字符串
 */
std::string asset_to_string(SimMarketAsset asset);

/**
 * @brief 模拟行情数据 - 完全匹配Rust TickSim
 */
struct TickSim {
    int64_t datetime;           // 时间戳
    double bid1;                // 买一价
    double ask1;                // 卖一价
    double bid_volume1;         // 买一量
    double ask_volume1;         // 卖一量
    double last_price;          // 最新价
    double last_volume;         // 最新量

    /**
     * @brief 构造函数
     */
    TickSim(int64_t datetime = 0, double bid1 = 0.0, double ask1 = 0.0,
            double bid_volume1 = 0.0, double ask_volume1 = 0.0,
            double last_price = 0.0, double last_volume = 0.0)
        : datetime(datetime), bid1(bid1), ask1(ask1),
          bid_volume1(bid_volume1), ask_volume1(ask_volume1),
          last_price(last_price), last_volume(last_volume) {}

    /**
     * @brief 转换为人类可读格式 - 匹配Rust to_human方法
     */
    std::tuple<std::string, double, double, double, double, double, double> to_human() const;

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static TickSim from_json(const nlohmann::json& j);
};

/**
 * @brief 模拟市场类 - 完全匹配Rust QASIMMarket
 */
class QASIMMarket {
private:
    std::unordered_map<std::string, std::unique_ptr<account::QA_Account>> reg_account_;  // 注册账户
    std::unordered_map<std::string, std::unique_ptr<matchengine::Orderbook<SimMarketAsset>>> order_book_;  // 订单簿
    std::unordered_map<std::string, std::unordered_map<uint64_t, account::Order>> matching_e_id_;  // 撮合引擎订单映射
    std::string time_;                              // 当前时间
    std::string portfolio_name_;                    // 组合名称
    std::string trading_day_;                       // 交易日
    std::unordered_map<std::string, double> lastpricepanel_;  // 最新价格面板
    std::unordered_map<std::string, std::vector<TickSim>> hisprice_;  // 历史价格
    std::unordered_map<std::string, TickSim> lasttick_;  // 最新tick

public:
    /**
     * @brief 构造函数 - 匹配Rust new方法
     */
    QASIMMarket();

    /**
     * @brief 析构函数
     */
    ~QASIMMarket() = default;

    /**
     * @brief 禁止拷贝，允许移动
     */
    QASIMMarket(const QASIMMarket&) = delete;
    QASIMMarket& operator=(const QASIMMarket&) = delete;
    QASIMMarket(QASIMMarket&&) = default;
    QASIMMarket& operator=(QASIMMarket&&) = default;

    /**
     * @brief 注册账户 - 匹配Rust register_account方法
     */
    void register_account(const std::string& account_cookie,
                         std::unique_ptr<account::QA_Account> account);

    /**
     * @brief 获取账户 - 匹配Rust get_account方法
     */
    account::QA_Account* get_account(const std::string& account_cookie);
    const account::QA_Account* get_account(const std::string& account_cookie) const;

    /**
     * @brief 订单发送 - 匹配Rust order_send方法
     */
    std::string order_send(const std::string& code,
                          const std::string& direction,
                          double price,
                          double volume,
                          const std::string& order_type,
                          const std::string& account_cookie);

    /**
     * @brief 订单撤销 - 匹配Rust order_cancel方法
     */
    bool order_cancel(const std::string& order_id, const std::string& account_cookie);

    /**
     * @brief 获取订单簿 - 匹配Rust get_orderbook方法
     */
    matchengine::Orderbook<SimMarketAsset>* get_orderbook(const std::string& code);
    const matchengine::Orderbook<SimMarketAsset>* get_orderbook(const std::string& code) const;

    /**
     * @brief 获取最新价格 - 匹配Rust get_last_price方法
     */
    double get_last_price(const std::string& code) const;

    /**
     * @brief 获取最新tick - 匹配Rust get_last_tick方法
     */
    std::optional<TickSim> get_last_tick(const std::string& code) const;

    /**
     * @brief 获取历史价格 - 匹配Rust get_his_price方法
     */
    std::vector<TickSim> get_his_price(const std::string& code) const;

    /**
     * @brief 市场更新 - 匹配Rust update_market方法
     */
    void update_market(const std::string& code, const TickSim& tick);

    /**
     * @brief 获取市场深度 - 匹配Rust get_market_depth方法
     */
    std::pair<std::optional<std::map<std::string, std::vector<double>>>,
              std::optional<std::map<std::string, std::vector<double>>>>
    get_market_depth(const std::string& code);

    /**
     * @brief 显示所有订单簿状态
     */
    void display_all_orderbooks();

    /**
     * @brief 获取所有注册的账户cookie
     */
    std::vector<std::string> get_registered_accounts() const;

    /**
     * @brief 获取所有可交易的代码
     */
    std::vector<std::string> get_tradeable_codes() const;

    /**
     * @brief 设置交易日 - 匹配Rust set_trading_day方法
     */
    void set_trading_day(const std::string& trading_day) { trading_day_ = trading_day; }

    /**
     * @brief 获取交易日
     */
    const std::string& get_trading_day() const { return trading_day_; }

    /**
     * @brief 设置组合名称
     */
    void set_portfolio_name(const std::string& portfolio_name) { portfolio_name_ = portfolio_name; }

    /**
     * @brief 获取组合名称
     */
    const std::string& get_portfolio_name() const { return portfolio_name_; }

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;

private:
    /**
     * @brief 处理订单结果 - 内部方法
     */
    void handle_order_results(const std::vector<std::variant<matchengine::Success, matchengine::Failed>>& results,
                             const std::string& account_cookie,
                             const std::string& code);

    /**
     * @brief 更新最新价格面板
     */
    void update_price_panel(const std::string& code, double price);

    /**
     * @brief 生成订单ID
     */
    std::string generate_order_id() const;

    /**
     * @brief 获取当前时间戳
     */
    static int64_t get_current_timestamp();

    /**
     * @brief 初始化默认订单簿
     */
    void init_default_orderbooks();
};

} // namespace qaultra::market::simmarket