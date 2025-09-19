#pragma once

#include "position.hpp"
#include "order.hpp"
#include "../protocol/qifi.hpp"
#include "../data/unified_datatype.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>

namespace qaultra::account {

/**
 * @brief 市场预设配置 - 统一版本
 */
struct MarketPreset {
    std::string name;
    int unit_table = 1;
    double price_tick = 0.01;
    double volume_tick = 1.0;
    double buy_fee_ratio = 0.0;
    double sell_fee_ratio = 0.0;
    double min_fee = 0.0;
    double tax_ratio = 0.001;      // 印花税
    double margin_ratio = 0.1;     // 保证金比例
    bool is_stock = true;
    bool allow_t0 = false;         // 是否允许T+0
    bool allow_sellopen = false;   // 是否允许卖开

    MarketPreset() = default;
    MarketPreset(const std::string& market_name);

    static MarketPreset get_stock_preset();
    static MarketPreset get_future_preset();
    static MarketPreset get_forex_preset();

    nlohmann::json to_json() const;
    static MarketPreset from_json(const nlohmann::json& j);
};

/**
 * @brief 冻结资金信息
 */
struct Frozen {
    double money = 0.0;
    std::string order_id;
    std::string datetime;
    std::string code;

    nlohmann::json to_json() const;
    static Frozen from_json(const nlohmann::json& j);
};

/**
 * @brief 账户切片，用于历史记录
 */
struct AccountSlice {
    std::string datetime;
    double cash = 0.0;
    std::string account_cookie;
    std::unordered_map<std::string, Position> positions;
    std::vector<Order> pending_orders;

    nlohmann::json to_json() const;
    static AccountSlice from_json(const nlohmann::json& j);
};

/**
 * @brief 统一账户类 - 集成simple和full版本最佳功能
 */
class UnifiedAccount {
public:
    /**
     * @brief 构造函数
     */
    UnifiedAccount(const std::string& account_cookie,
                   const std::string& portfolio_cookie = "",
                   const std::string& user_cookie = "",
                   double init_cash = 1000000.0,
                   bool auto_reload = false);

    /**
     * @brief 析构函数
     */
    ~UnifiedAccount() = default;

    // 账户基本信息
    const std::string& get_account_cookie() const { return account_cookie_; }
    const std::string& get_portfolio_cookie() const { return portfolio_cookie_; }
    const std::string& get_user_cookie() const { return user_cookie_; }

    // 资金相关
    double get_cash() const;
    double get_frozen_cash() const;
    double get_available_cash() const;
    double get_market_value() const;
    double get_total_value() const;
    double get_pnl() const;
    double get_float_pnl() const;

    // 交易操作 - 支持股票和期货
    std::string buy(const std::string& code, double volume, double price = 0.0);
    std::string sell(const std::string& code, double volume, double price = 0.0);

    // 期货专用操作
    std::string buy_open(const std::string& code, double volume, double price = 0.0);
    std::string sell_open(const std::string& code, double volume, double price = 0.0);
    std::string buy_close(const std::string& code, double volume, double price = 0.0);
    std::string sell_close(const std::string& code, double volume, double price = 0.0);
    std::string buy_closetoday(const std::string& code, double volume, double price = 0.0);
    std::string sell_closetoday(const std::string& code, double volume, double price = 0.0);

    // 订单管理
    bool cancel_order(const std::string& order_id);
    bool cancel_all_orders();
    std::vector<Order> get_pending_orders() const;
    std::vector<Order> get_filled_orders() const;
    std::optional<Order> find_order(const std::string& order_id) const;

    // 持仓管理
    std::unordered_map<std::string, Position> get_positions() const;
    std::optional<Position> get_position(const std::string& code) const;
    bool has_position(const std::string& code) const;

    // 成交管理
    void add_trade(const std::string& order_id, double price, double volume,
                   const std::string& datetime = "");
    std::vector<std::string> get_trade_history() const;

    // 账户切片和历史
    AccountSlice get_current_slice() const;
    void save_slice(const AccountSlice& slice);
    std::vector<AccountSlice> get_history_slices() const;

    // 市场数据更新
    void update_market_data(const std::string& code, double price);
    void update_market_data_batch(const std::unordered_map<std::string, double>& prices);

    // 结算相关
    void daily_settle();
    void calculate_pnl();

    // 风险管理
    bool check_risk_before_order(const Order& order) const;
    double get_buying_power() const;
    double get_margin_usage() const;

    // QIFI协议支持
    protocol::qifi::QIFI get_qifi() const;
    void from_qifi(const protocol::qifi::QIFI& qifi_data);

    // 配置管理
    void set_market_preset(const MarketPreset& preset);
    const MarketPreset& get_market_preset() const { return market_preset_; }

    // 状态查询
    bool is_valid() const;
    std::string get_status() const;

    // 序列化
    nlohmann::json to_json() const;
    static UnifiedAccount from_json(const nlohmann::json& j);

    // 统计信息
    struct Statistics {
        int total_orders = 0;
        int filled_orders = 0;
        int cancelled_orders = 0;
        double total_commission = 0.0;
        double total_tax = 0.0;
        double max_cash = 0.0;
        double min_cash = 0.0;
        double max_total_value = 0.0;
        double min_total_value = 0.0;
        std::string start_date;
        std::string end_date;
    };

    Statistics get_statistics() const;

    // 性能监控
    void enable_performance_monitoring(bool enable = true) { performance_monitoring_ = enable; }
    bool is_performance_monitoring() const { return performance_monitoring_; }

    // 事件回调
    using OrderCallback = std::function<void(const Order&)>;
    using TradeCallback = std::function<void(const std::string&, double, double)>;
    using PositionCallback = std::function<void(const std::string&, const Position&)>;

    void set_order_callback(OrderCallback callback) { order_callback_ = callback; }
    void set_trade_callback(TradeCallback callback) { trade_callback_ = callback; }
    void set_position_callback(PositionCallback callback) { position_callback_ = callback; }

private:
    // 基本属性
    std::string account_cookie_;
    std::string portfolio_cookie_;
    std::string user_cookie_;
    double init_cash_;
    bool auto_reload_;

    // 资金状态
    std::atomic<double> cash_;
    std::atomic<double> frozen_cash_;
    std::atomic<double> total_value_;
    std::atomic<double> float_pnl_;

    // 交易数据
    std::unordered_map<std::string, Position> positions_;
    std::unordered_map<std::string, Order> orders_;
    std::vector<std::string> trade_history_;
    std::vector<AccountSlice> history_slices_;

    // 配置和状态
    MarketPreset market_preset_;
    std::unordered_map<std::string, double> market_prices_;

    // 计数器
    std::atomic<int> order_id_counter_;
    std::atomic<int> trade_id_counter_;

    // 线程安全
    mutable std::mutex positions_mutex_;
    mutable std::mutex orders_mutex_;
    mutable std::mutex history_mutex_;

    // 性能监控
    bool performance_monitoring_ = false;
    Statistics statistics_;

    // 事件回调
    OrderCallback order_callback_;
    TradeCallback trade_callback_;
    PositionCallback position_callback_;

    // 内部辅助方法
    std::string generate_order_id();
    std::string generate_trade_id();

    double calculate_commission(double price, double volume, bool is_buy) const;
    double calculate_tax(double price, double volume) const;

    bool validate_order_params(const std::string& code, double volume, double price) const;
    void update_position_from_trade(const std::string& code, double price, double volume, bool is_buy);
    void freeze_cash_for_order(const Order& order);
    void unfreeze_cash_for_order(const Order& order);

    void trigger_order_callback(const Order& order);
    void trigger_trade_callback(const std::string& trade_id, double price, double volume);
    void trigger_position_callback(const std::string& code, const Position& position);

    void update_statistics(const Order& order);
};

/**
 * @brief 账户工厂类
 */
class AccountFactory {
public:
    enum class AccountType {
        Simple,     // 简化版本
        Full,       // 完整版本
        Unified     // 统一版本
    };

    static std::unique_ptr<UnifiedAccount> create_account(
        AccountType type,
        const std::string& account_cookie,
        double init_cash = 1000000.0);

    static std::unique_ptr<UnifiedAccount> create_stock_account(
        const std::string& account_cookie,
        double init_cash = 1000000.0);

    static std::unique_ptr<UnifiedAccount> create_future_account(
        const std::string& account_cookie,
        double init_cash = 1000000.0);

    static std::unique_ptr<UnifiedAccount> create_forex_account(
        const std::string& account_cookie,
        double init_cash = 1000000.0);

    // 从配置创建
    static std::unique_ptr<UnifiedAccount> create_from_config(
        const nlohmann::json& config);

    // 从QIFI创建
    static std::unique_ptr<UnifiedAccount> create_from_qifi(
        const protocol::qifi::QIFI& qifi_data);
};

/**
 * @brief 账户管理器 - 管理多个账户
 */
class AccountManager {
public:
    AccountManager() = default;
    ~AccountManager() = default;

    // 账户管理
    void add_account(std::unique_ptr<UnifiedAccount> account);
    void remove_account(const std::string& account_cookie);
    UnifiedAccount* get_account(const std::string& account_cookie);
    const UnifiedAccount* get_account(const std::string& account_cookie) const;

    std::vector<std::string> get_account_list() const;
    size_t get_account_count() const;

    // 批量操作
    void update_all_market_data(const std::unordered_map<std::string, double>& prices);
    void daily_settle_all();

    // 统计信息
    struct ManagerStatistics {
        int total_accounts = 0;
        double total_cash = 0.0;
        double total_market_value = 0.0;
        double total_pnl = 0.0;
        std::unordered_map<std::string, int> accounts_by_type;
    };

    ManagerStatistics get_statistics() const;

    // 序列化
    nlohmann::json to_json() const;
    static AccountManager from_json(const nlohmann::json& j);

private:
    std::unordered_map<std::string, std::unique_ptr<UnifiedAccount>> accounts_;
    mutable std::mutex accounts_mutex_;
};

} // namespace qaultra::account