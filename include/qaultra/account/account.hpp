#pragma once

#include "order.hpp"
#include "position.hpp"
#include "marketpreset.hpp"
#include "../protocol/qifi.hpp"

// 使用现有的 QIFI 命名空间别名
namespace protocol = qaultra::protocol::qifi;
#include <unordered_map>
#include <map>
#include <string>
#include <memory>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace qaultra::account {

// 类型别名 - 匹配Rust实现
using Price = double;
using Volume = double;
using Amount = double;
using AssetId = std::string;

/**
 * @brief 账户切片 - 用于记录账户某个时刻的状态
 */
struct AccountSlice {
    std::string datetime;                               // 时间戳
    double cash;                                        // 现金
    protocol::Account accounts;                         // 账户信息
    std::unordered_map<std::string, QA_Position> positions; // 持仓信息

    nlohmann::json to_json() const;
    static AccountSlice from_json(const nlohmann::json& j);
};

/**
 * @brief MOM切片 - Multi-Order-Management切片
 */
struct MOMSlice {
    std::string datetime;       // 时间戳
    std::string user_id;        // 用户ID
    double pre_balance;         // 上一交易日权益
    double close_profit;        // 平仓盈亏
    double commission;          // 手续费
    double position_profit;     // 持仓盈亏
    double float_profit;        // 浮动盈亏
    double balance;             // 当前权益
    double margin;              // 保证金
    double available;           // 可用资金
    double risk_ratio;          // 风险度

    nlohmann::json to_json() const;
    static MOMSlice from_json(const nlohmann::json& j);
};

/**
 * @brief 账户信息 - 用于消息传递
 */
struct AccountInfo {
    std::string datetime;           // 时间戳
    double balance;                 // 权益
    std::string account_cookie;     // 账户标识

    nlohmann::json to_json() const;
    static AccountInfo from_json(const nlohmann::json& j);
};

#include "algo/algo.hpp"

/**
 * @brief QA账户类 - 完全匹配Rust QA_Account实现
 * @details 核心交易账户，支持股票和期货交易，包含完整的风控和资金管理
 */
class Account {
private:
    // 基础配置 - 匹配Rust字段
    double init_cash_;                                          // 初始资金
    std::unordered_map<std::string, QA_Position> init_hold_;       // 初始持仓
    bool allow_t0_;                                             // 是否允许T+0交易
    bool allow_sellopen_;                                       // 是否允许卖开仓
    bool allow_margin_;                                         // 是否允许融资融券
    bool auto_reload_;                                          // 自动重载
    MarketPreset market_preset_;                                // 市场预设配置

    // 时间和事件
    std::string time_;                                          // 当前时间
    std::unordered_map<std::string, std::string> events_;      // 事件记录
    int event_id_;                                              // 事件ID

    // 账户信息
    protocol::Account accounts_;                                // QIFI账户信息
    double money_;                                              // 当前现金
    std::unordered_map<std::string, QA_Position> hold_;           // 持仓
    std::unordered_map<std::string, protocol::Frozen> frozen_; // 冻结资金

    // 标识信息
    std::string account_cookie_;                                // 账户标识
    std::string portfolio_cookie_;                              // 组合标识
    std::string user_cookie_;                                   // 用户标识
    std::string environment_;                                   // 环境标识

    // 交易记录
    std::map<std::string, protocol::Trade> daily_trades_;      // 当日成交
    std::map<std::string, protocol::Order> daily_orders_;      // 当日委托

    // 费率配置
    double commission_ratio_;                                   // 手续费率
    double tax_ratio_;                                          // 印花税率

    // 算法管理器
    std::unique_ptr<algo::AlgoOrderManager> algo_manager_;      // 算法订单管理器

public:
    /**
     * @brief 构造函数 - 匹配Rust new方法
     */
    Account(const std::string& account_cookie,
            const std::string& portfolio_cookie,
            const std::string& user_cookie,
            double init_cash,
            bool auto_reload = false,
            const std::string& environment = "backtest");

    /**
     * @brief 从QIFI格式创建账户 - 匹配Rust new_from_qifi方法
     */
    static Account from_qifi(const protocol::QIFI& qifi);

    /**
     * @brief 析构函数
     */
    ~Account() = default;

    // 禁止拷贝，允许移动
    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;
    Account(Account&&) = default;
    Account& operator=(Account&&) = default;

    // 配置方法 - 匹配Rust实现
    void set_sellopen(bool sellopen);
    void set_t0(bool t0);
    void set_portfolio_cookie(const std::string& portfolio);
    void reload();

    // 资金查询方法 - 匹配Rust实现
    double get_cash() const { return money_; }
    double get_frozen_margin() const;
    double get_risk_ratio() const;
    double get_balance() const;
    double get_margin() const;
    double get_position_profit() const;
    double get_float_profit() const;

    // 账户切片和信息 - 匹配Rust实现
    MOMSlice get_mom_slice() const;
    protocol::QIFI get_qifi_slice() const;
    protocol::Account get_account_message() const;
    AccountInfo get_account_info() const;

    // 持仓查询方法 - 匹配Rust实现
    std::unordered_map<std::string, double> get_account_pos_value() const;
    std::unordered_map<std::string, double> get_account_pos_weight() const;
    std::unordered_map<std::string, double> get_account_pos_longshort() const;

    QA_Position* get_position(const std::string& code);
    const QA_Position* get_position(const std::string& code) const;
    double get_volume_long(const std::string& code) const;
    double get_volume_short(const std::string& code) const;
    double get_volume_avail(const std::string& code) const;

    // 价格更新方法 - 匹配Rust实现
    void on_price_change(const std::string& code, double new_price, const std::string& datetime);
    void on_bar(const protocol::MarketData& bar);

    // 资产事件方法 - 匹配Rust实现
    void transfer_event(const std::string& code, double amount);
    void dividend_event(const std::string& code, double money_ratio);

    // 核心交易方法 - 完全匹配Rust实现
    std::optional<Order> buy(const std::string& code, double amount, const std::string& time, double price);
    std::optional<Order> sell(const std::string& code, double amount, const std::string& time, double price);

    // 智能交易方法 - 匹配Rust smart_buy/smart_sell
    std::optional<Order> smart_buy(const std::string& code, double amount, const std::string& time, double price);
    std::optional<Order> smart_sell(const std::string& code, double amount, const std::string& time, double price);

    // 期货交易方法 - 匹配Rust实现
    std::optional<Order> buy_open(const std::string& code, double amount, const std::string& time, double price);
    std::optional<Order> sell_open(const std::string& code, double amount, const std::string& time, double price);
    std::optional<Order> buy_close(const std::string& code, double amount, const std::string& time, double price);
    std::optional<Order> sell_close(const std::string& code, double amount, const std::string& time, double price);
    std::optional<Order> buy_closetoday(const std::string& code, double amount, const std::string& time, double price);
    std::optional<Order> sell_closetoday(const std::string& code, double amount, const std::string& time, double price);

    // 通用下单方法 - 匹配Rust send_order
    std::optional<Order> send_order(const std::string& code,
                                    double amount,
                                    const std::string& time,
                                    int towards,
                                    double price,
                                    const std::string& exchange = "",
                                    const std::string& order_type = "LIMIT");

    // 手动下单 - 匹配Rust insert_order
    std::optional<Order> insert_order(const std::string& code,
                                      double amount,
                                      const std::string& time,
                                      double price,
                                      const std::string& direction,
                                      const std::string& offset);

    // 订单处理方法 - 匹配Rust实现
    bool receive_deal(const std::string& trade_id,
                      const std::string& order_id,
                      const std::string& code,
                      double trade_price,
                      double trade_volume,
                      const std::string& trade_time,
                      const std::string& direction,
                      const std::string& offset);

    // 算法交易接口 - 匹配Rust实现
    std::string create_algo_order(const std::string& code,
                                 double total_amount,
                                 double base_price,
                                 int direction,
                                 algo::SplitAlgorithm algorithm,
                                 std::optional<algo::SplitParams> params = std::nullopt);

    bool execute_next_algo_chunk(const std::string& algo_id);
    bool cancel_algo_order(const std::string& algo_id);
    void update_algo_orders(const std::string& datetime);

    // 算法订单状态查询
    std::vector<std::string> get_active_algo_orders() const;
    algo::SplitOrderPlan* get_algo_plan(const std::string& algo_id);
    nlohmann::json get_algo_status() const;

    // 工具方法 - 匹配Rust实现
    std::string get_trading_day() const;
    std::string get_qifi_trading_day() const;
    std::string get_current_time() const;
    void update_timestamp();
    void settle();  // 日终结算

    // 访问器方法
    double get_init_cash() const { return init_cash_; }
    bool get_allow_t0() const { return allow_t0_; }
    bool get_allow_sellopen() const { return allow_sellopen_; }
    bool get_allow_margin() const { return allow_margin_; }
    bool get_auto_reload() const { return auto_reload_; }
    const std::string& get_time() const { return time_; }
    const std::string& get_environment() const { return environment_; }
    int get_event_id() const { return event_id_; }
    double get_commission_ratio() const { return commission_ratio_; }
    double get_tax_ratio() const { return tax_ratio_; }
    const std::string& get_account_cookie() const { return account_cookie_; }
    const std::string& get_portfolio_cookie() const { return portfolio_cookie_; }
    const std::string& get_user_cookie() const { return user_cookie_; }

    // 修改器方法
    void set_commission_ratio(double ratio) { commission_ratio_ = ratio; }
    void set_tax_ratio(double ratio) { tax_ratio_ = ratio; }

    // 序列化方法
    nlohmann::json to_json() const;
    static Account from_json(const nlohmann::json& j);

    // 调试方法 - 匹配Rust实现
    void message() const;
    std::string to_string() const;

private:
    // 内部辅助方法 - 匹配Rust私有方法
    void init_position(const std::string& code);
    std::optional<Order> order_check(const std::string& code,
                                     double amount,
                                     double price,
                                     int towards,
                                     const std::string& order_id,
                                     const std::string& datetime);

    int get_towards(const std::string& direction, const std::string& offset) const;
    void freeze_position(const std::string& code, const std::string& direction,
                        const std::string& offset, double volume);
    void unfreeze_position(const std::string& code, const std::string& direction,
                          const std::string& offset, double volume);

    bool check_order_rules(const std::string& code, double amount, int towards) const;
    void update_account_info();
    std::string generate_order_id() const;
    int64_t get_timestamp_nanos(const std::string& datetime) const;
};

/**
 * @brief 账户统计信息
 */
struct AccountStats {
    size_t total_positions = 0;         // 总持仓数
    size_t active_positions = 0;        // 活跃持仓数
    double total_market_value = 0.0;    // 总市值
    double total_profit = 0.0;          // 总盈亏
    double total_margin = 0.0;          // 总保证金
    double max_drawdown = 0.0;          // 最大回撤
    double sharpe_ratio = 0.0;          // 夏普比率

    void update(const Account& account);
    void reset();
    nlohmann::json to_json() const;
};

} // namespace qaultra::account
