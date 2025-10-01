#pragma once

#include "../account/qa_account.hpp"
#include "../data/datatype.hpp"
#include "../data/marketcenter.hpp"
#include "../protocol/qifi.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <queue>
#include <functional>

namespace qaultra::market {

/**
 * @brief 市场订单结构 - 匹配 Rust MarketOrder
 */
struct MarketOrder {
    std::string account_name;
    std::string code;
    double amount = 0.0;
    double price = 0.0;
    std::string direction;  // BUY, SELL
    std::string offset;     // OPEN, CLOSE, CLOSETODAY
    std::string label;

    MarketOrder() = default;
    MarketOrder(const std::string& acc, const std::string& c, double amt, double p,
                const std::string& dir, const std::string& off, const std::string& lbl = "")
        : account_name(acc), code(c), amount(amt), price(p),
          direction(dir), offset(off), label(lbl) {}
};

/**
 * @brief 持仓目标结构
 */
struct TargetPosition {
    std::string account_name;
    std::string code;
    std::unordered_map<std::string, double> targets;  // code -> target_volume
    std::string label;
};

/**
 * @brief 市场系统 - 完全匹配 Rust QAMarket
 *
 * 核心功能:
 * - 账户注册和管理
 * - 市场数据中心
 * - 时间管理 (today, curtime)
 * - 订单队列和目标持仓队列
 * - QIFI 快照缓存
 */
class QAMarketSystem {
private:
    // 基础配置
    std::string username_;
    std::string portfolio_name_;

    // 账户管理
    std::unordered_map<std::string, std::shared_ptr<account::QA_Account>> reg_accounts_;

    // 市场数据中心
    std::shared_ptr<data::QAMarketCenter> market_data_;

    // 时间管理
    std::string today_;        // 当前日期 "2025-10-01"
    std::string curtime_;      // 当前时间 "2025-10-01 09:30:00"

    // 订单和目标队列
    std::queue<std::tuple<std::string, std::string, std::unordered_map<std::string, double>, std::string>>
        schedule_queue_;           // (account, code, positions, label)

    std::queue<std::tuple<std::string, MarketOrder, std::string>>
        schedule_order_queue_;     // (account, order, label)

    std::queue<std::tuple<std::string, std::string, std::unordered_map<std::string, double>, std::string>>
        schedule_target_queue_;    // (account, code, targets, label)

    // QIFI 快照缓存 - account_name -> [QIFI snapshots]
    std::unordered_map<std::string, std::vector<protocol::qifi::QIFI>> cache_;

public:
    /**
     * @brief 构造函数 - 默认创建
     */
    QAMarketSystem();

    /**
     * @brief 构造函数 - 从路径创建
     */
    QAMarketSystem(const std::string& data_path, const std::string& portfolio_name = "");

    /**
     * @brief 构造函数 - 使用已有的 MarketCenter
     */
    explicit QAMarketSystem(std::shared_ptr<data::QAMarketCenter> market_center);

    /**
     * @brief 析构函数
     */
    ~QAMarketSystem() = default;

    // 禁用拷贝
    QAMarketSystem(const QAMarketSystem&) = delete;
    QAMarketSystem& operator=(const QAMarketSystem&) = delete;

    // 启用移动
    QAMarketSystem(QAMarketSystem&&) = default;
    QAMarketSystem& operator=(QAMarketSystem&&) = default;

    // ============ 基础管理 ============

    /**
     * @brief 重新初始化 - 匹配 Rust reinit()
     */
    void reinit();

    /**
     * @brief 保存所有 QIFI 快照到数据库 - 匹配 Rust save()
     */
    void save();

    /**
     * @brief 获取用户名
     */
    const std::string& get_username() const { return username_; }

    /**
     * @brief 获取组合名称
     */
    const std::string& get_portfolio_name() const { return portfolio_name_; }

    /**
     * @brief 设置组合名称
     */
    void set_portfolio_name(const std::string& name) { portfolio_name_ = name; }

    // ============ 时间管理 ============

    /**
     * @brief 设置当前日期 - 匹配 Rust set_date()
     */
    void set_date(const std::string& date);

    /**
     * @brief 设置当前时间 - 匹配 Rust set_datetime()
     */
    void set_datetime(const std::string& datetime);

    /**
     * @brief 获取当前日期
     */
    const std::string& get_date() const { return today_; }

    /**
     * @brief 获取当前时间
     */
    const std::string& get_datetime() const { return curtime_; }

    // ============ 账户管理 ============

    /**
     * @brief 注册账户 - 匹配 Rust register_account()
     */
    void register_account(const std::string& account_name, double init_cash = 1000000.0);

    /**
     * @brief 从 QIFI 注册账户
     */
    void register_account_from_qifi(const protocol::qifi::QIFI& qifi);

    /**
     * @brief 获取账户
     */
    std::shared_ptr<account::QA_Account> get_account(const std::string& account_name);
    std::shared_ptr<const account::QA_Account> get_account(const std::string& account_name) const;

    /**
     * @brief 获取所有账户名称
     */
    std::vector<std::string> get_account_names() const;

    /**
     * @brief 获取账户数量
     */
    size_t get_account_count() const { return reg_accounts_.size(); }

    // ============ 市场数据 ============

    /**
     * @brief 获取市场数据中心
     */
    std::shared_ptr<data::QAMarketCenter> get_market_data() { return market_data_; }
    std::shared_ptr<const data::QAMarketCenter> get_market_data() const { return market_data_; }

    /**
     * @brief 获取股票日线数据
     */
    std::vector<data::StockCnDay> get_stock_day(const std::string& code,
                                                  const std::string& start_date,
                                                  const std::string& end_date) const;

    /**
     * @brief 获取股票分钟数据
     */
    std::vector<data::StockCn1Min> get_stock_min(const std::string& code,
                                                   const std::string& start_datetime,
                                                   const std::string& end_datetime) const;

    // ============ 交易调度 ============

    /**
     * @brief 添加订单到队列
     */
    void schedule_order(const std::string& account_name, const MarketOrder& order,
                       const std::string& label = "");

    /**
     * @brief 添加持仓调整到队列
     */
    void schedule_position(const std::string& account_name, const std::string& code,
                          const std::unordered_map<std::string, double>& positions,
                          const std::string& label = "");

    /**
     * @brief 添加目标持仓到队列
     */
    void schedule_target(const std::string& account_name, const std::string& code,
                        const std::unordered_map<std::string, double>& targets,
                        const std::string& label = "");

    /**
     * @brief 处理订单队列 - 执行所有待处理订单
     */
    void process_order_queue();

    /**
     * @brief 处理目标持仓队列
     */
    void process_target_queue();

    /**
     * @brief 清空所有队列
     */
    void clear_queues();

    // ============ QIFI 快照管理 ============

    /**
     * @brief 保存当前所有账户的 QIFI 快照
     */
    void snapshot_all_accounts();

    /**
     * @brief 获取账户的 QIFI 快照历史
     */
    const std::vector<protocol::qifi::QIFI>& get_account_snapshots(const std::string& account_name) const;

    /**
     * @brief 清空快照缓存
     */
    void clear_cache() { cache_.clear(); }

    // ============ 回测执行 ============

    /**
     * @brief 运行回测 - 按日期范围
     */
    void run_backtest(const std::string& start_date,
                     const std::string& end_date,
                     std::function<void(QAMarketSystem&)> strategy_func);

    /**
     * @brief 单步执行 - 处理一个时间点
     */
    void step(const std::string& datetime);

    /**
     * @brief 批量更新所有账户的市场价格
     */
    void update_all_prices(const std::unordered_map<std::string, double>& price_map);
};

} // namespace qaultra::market
