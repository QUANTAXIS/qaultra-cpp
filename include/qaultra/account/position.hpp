#pragma once

#include "../protocol/qifi.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace qaultra::account {

// Type definitions
using Price = double;
using Volume = double;
using Amount = double;
using AssetId = std::string;

/**
 * @brief 持仓信息类 - 完全匹配Rust QA_Postions实现
 */
class Position {
public:
    // 基础信息字段 - 完全匹配Rust实现
    std::string code;                           // 代码
    std::string instrument_id;                  // 合约代码
    std::string user_id;                        // 用户ID
    std::string portfolio_cookie;               // 组合标识
    std::string username;                       // 用户名
    std::string position_id;                    // 持仓ID
    std::string account_cookie;                 // 账户标识
    double frozen = 0.0;                        // 冻结数量
    std::string name;                           // 名称
    std::string spms_id;                        // SPMS ID
    std::string oms_id;                         // OMS ID
    std::string market_type;                    // 市场类型
    std::string exchange_id;                    // 交易所代码
    std::string lastupdatetime;                 // 最后更新时间

    // 持仓量字段 - 完全匹配Rust
    double volume_long_today = 0.0;             // 多头今日持仓量
    double volume_long_his = 0.0;               // 多头历史持仓量
    double volume_short_today = 0.0;            // 空头今日持仓量
    double volume_short_his = 0.0;              // 空头历史持仓量

    // 平仓委托冻结(未成交)
    double volume_long_frozen_today = 0.0;      // 多头今日冻结量
    double volume_long_frozen_his = 0.0;        // 多头历史冻结量
    double volume_short_frozen_today = 0.0;     // 空头今日冻结量
    double volume_short_frozen_his = 0.0;       // 空头历史冻结量

    // 保证金
    double margin_long = 0.0;                   // 多头保证金
    double margin_short = 0.0;                  // 空头保证金

    // 持仓字段
    double position_price_long = 0.0;           // 多头持仓价格
    double position_cost_long = 0.0;            // 多头持仓成本
    double position_price_short = 0.0;          // 空头持仓价格
    double position_cost_short = 0.0;           // 空头持仓成本

    // 开仓字段
    double open_price_long = 0.0;               // 多头开仓价格
    double open_cost_long = 0.0;                // 多头开仓成本
    double open_price_short = 0.0;              // 空头开仓价格
    double open_cost_short = 0.0;               // 空头开仓成本

    // 最新价格信息
    double lastest_price = 0.0;                 // 最新价格
    std::string lastest_datetime;               // 最新价格时间

    // 市场预设配置
    struct CodePreset {
        std::string name;
        int unit_table = 1;                     // 合约乘数
        double price_tick = 0.01;               // 最小变动价位
        double buy_fee_ratio = 0.0;             // 买入手续费率
        double sell_fee_ratio = 0.0;            // 卖出手续费率
        double min_fee = 0.0;                   // 最小手续费
        double margin_ratio = 0.0;              // 保证金率

        nlohmann::json to_json() const;
        static CodePreset from_json(const nlohmann::json& j);
    } preset;

public:
    Position() = default;

    /// 构造函数 - 匹配Rust实现
    Position(const std::string& code,
             const std::string& account_cookie,
             const std::string& user_id,
             const std::string& portfolio_cookie);

    /// 新建持仓 - 匹配Rust new方法
    static Position new_position(const std::string& code,
                                const std::string& account_cookie,
                                const std::string& user_id,
                                const std::string& portfolio_cookie);

    /// 从QIFI格式创建 - 匹配Rust from_qifi方法
    static Position from_qifi(const std::string& account_cookie,
                              const std::string& user_cookie,
                              const std::string& account_id,
                              const std::string& portfolio_cookie,
                              const protocol::Position& qifi_pos);

    /// 核心计算方法 - 完全匹配Rust实现
    double volume_long() const;                 // 多头总持仓量
    double volume_short() const;                // 空头总持仓量
    double volume_long_frozen() const;          // 多头总冻结量
    double volume_short_frozen() const;         // 空头总冻结量
    double volume_long_avaliable() const;       // 多头可用量
    double volume_short_avaliable() const;      // 空头可用量

    // 计算方法
    double volume_net() const;                  // 净持仓量 (多-空)
    double volume_total() const;                // 总持仓量 (多+空)
    double market_value() const;                // 市值
    double market_value_long() const;           // 多头市值
    double market_value_short() const;          // 空头市值

    // 盈亏计算 - 匹配Rust实现
    double position_profit() const;             // 持仓盈亏
    double position_profit_long() const;        // 多头持仓盈亏
    double position_profit_short() const;       // 空头持仓盈亏
    double float_profit() const;                // 浮动盈亏
    double float_profit_long() const;           // 多头浮动盈亏
    double float_profit_short() const;          // 空头浮动盈亏

    // 均价计算
    double avg_price_long() const;              // 多头持仓均价
    double avg_price_short() const;             // 空头持仓均价

    // 保证金计算
    double margin() const;                      // 总保证金
    double margin_required() const;             // 所需保证金

    // 交易操作方法 - 匹配Rust receive_deal方法
    void receive_deal(const std::string& trade_id,
                      const std::string& direction,
                      const std::string& offset,
                      double volume,
                      double price,
                      const std::string& datetime);

    // 价格更新 - 匹配Rust on_price_change方法
    void on_price_change(double new_price, const std::string& datetime);

    // 冻结和解冻操作
    void freeze_position(const std::string& direction,
                        const std::string& offset,
                        double volume);

    void unfreeze_position(const std::string& direction,
                          const std::string& offset,
                          double volume);

    // 查询方法
    bool is_empty() const;                      // 是否空仓
    bool is_long() const;                       // 是否有多头持仓
    bool is_short() const;                      // 是否有空头持仓
    bool has_position() const;                  // 是否有持仓
    bool can_close_today(const std::string& direction, double volume) const;

    // 序列化方法
    nlohmann::json to_json() const;
    static Position from_json(const nlohmann::json& j);
    protocol::Position to_qifi() const;

    // 工具方法
    std::string get_market_type() const;
    void update_timestamp();
    void settle_position();                     // 日终结算

    // 调试方法
    void message() const;                       // 打印消息 - 匹配Rust
    std::string to_string() const;

private:
    void update_position_costs();               // 更新持仓成本
    void update_open_costs();                   // 更新开仓成本
    void recalculate_margins();                 // 重新计算保证金
    void validate_data() const;                 // 验证数据一致性

    // 辅助方法
    std::string get_current_time() const;
    std::string adjust_market_type(const std::string& code) const;
};

/// 持仓统计信息
struct PositionStats {
    size_t total_positions = 0;
    size_t active_positions = 0;
    size_t long_positions = 0;
    size_t short_positions = 0;
    double total_market_value = 0.0;
    double total_float_profit = 0.0;
    double total_position_profit = 0.0;
    double total_margin = 0.0;
    double total_volume_long = 0.0;
    double total_volume_short = 0.0;

    void update(const Position& pos);
    void reset();
    nlohmann::json to_json() const;
};

/// 市场类型判断函数 - 匹配Rust adjust_market函数
std::string adjust_market(const std::string& code);

} // namespace qaultra::account