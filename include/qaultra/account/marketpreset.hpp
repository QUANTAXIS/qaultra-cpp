#pragma once

#include <string>
#include <unordered_map>
#include <regex>
#include <nlohmann/json.hpp>

namespace qaultra::account {

/**
 * @brief 代码预设配置 - 完全匹配Rust CodePreset实现
 * @details 包含合约的基本信息、费率、保证金等配置
 */
class CodePreset {
public:
    // 基础信息字段 - 完全匹配Rust实现
    std::string name;                           // 合约名称
    int unit_table = 1;                         // 合约乘数
    double price_tick = 0.01;                   // 最小变动价位
    double buy_frozen_coeff = 1.0;              // 买入冻结系数(保证金率)
    double sell_frozen_coeff = 1.0;             // 卖出冻结系数(保证金率)
    std::string exchange;                       // 交易所代码
    double commission_coeff_peramount = 0.0;    // 按金额收取的手续费率
    double commission_coeff_pervol = 0.0;       // 按手数收取的手续费
    double commission_coeff_today_peramount = 0.0;  // 平今按金额收取的手续费率
    double commission_coeff_today_pervol = 0.0;     // 平今按手数收取的手续费

public:
    CodePreset() = default;

    /**
     * @brief 构造函数
     */
    CodePreset(const std::string& name_param,
               int unit_table_param,
               double price_tick_param,
               double buy_frozen_coeff_param,
               double sell_frozen_coeff_param,
               const std::string& exchange_param,
               double commission_coeff_peramount_param,
               double commission_coeff_pervol_param,
               double commission_coeff_today_peramount_param,
               double commission_coeff_today_pervol_param);

    /**
     * @brief 计算市值 - 完全匹配Rust实现
     * @param price 价格
     * @param volume 数量
     * @return 市值
     */
    inline double calc_marketvalue(double price, double volume) const {
        return volume * price * static_cast<double>(unit_table);
    }

    /**
     * @brief 计算冻结资金 - 完全匹配Rust实现
     * @param price 价格
     * @param volume 数量
     * @return 冻结资金(保证金)
     */
    inline double calc_frozenmoney(double price, double volume) const {
        return calc_marketvalue(price, volume) * buy_frozen_coeff;
    }

    /**
     * @brief 计算手续费 - 完全匹配Rust实现
     * @param price 价格
     * @param volume 数量
     * @return 手续费
     */
    inline double calc_commission(double price, double volume) const {
        return commission_coeff_pervol * volume +
               commission_coeff_peramount * calc_marketvalue(price, volume);
    }

    /**
     * @brief 计算印花税 - 完全匹配Rust实现
     * @param price 价格
     * @param volume 数量
     * @param towards 方向标识
     * @return 印花税
     */
    inline double calc_tax(double price, double volume, int towards) const {
        // 股票卖出征收千分之一印花税
        if (exchange == "STOCK" && (towards == -1 || towards == 3 ||
                                    towards == 4 || towards == -3 || towards == -4)) {
            return 0.001 * calc_marketvalue(price, volume);
        }
        return 0.0;
    }

    /**
     * @brief 计算平今手续费 - 完全匹配Rust实现
     * @param price 价格
     * @param volume 数量
     * @return 平今手续费
     */
    inline double calc_commission_today(double price, double volume) const {
        return commission_coeff_today_pervol * volume +
               commission_coeff_today_peramount * calc_marketvalue(price, volume);
    }

    /**
     * @brief 计算买入冻结系数 - 完全匹配Rust实现
     * @return 买入冻结系数
     */
    inline double calc_coeff() const {
        return buy_frozen_coeff * static_cast<double>(unit_table);
    }

    /**
     * @brief 计算卖开冻结系数 - 完全匹配Rust实现
     * @return 卖开冻结系数
     */
    inline double calc_sellopencoeff() const {
        return sell_frozen_coeff * static_cast<double>(unit_table);
    }

    /**
     * @brief 打印配置信息 - 匹配Rust实现
     */
    void print() const;

    // 序列化方法
    nlohmann::json to_json() const;
    static CodePreset from_json(const nlohmann::json& j);
};

/**
 * @brief 市场预设配置管理器 - 完全匹配Rust MarketPreset实现
 * @details 管理所有合约的预设配置，包括期货、股票等
 */
class MarketPreset {
private:
    std::unordered_map<std::string, CodePreset> preset_;   // 预设配置映射表

public:
    MarketPreset() = default;

    /**
     * @brief 创建默认市场预设 - 完全匹配Rust new()方法
     * @return MarketPreset实例
     */
    static MarketPreset create_default();

    /**
     * @brief 获取指定合约代码的预设配置 - 完全匹配Rust get()方法
     * @param code 合约代码
     * @return 对应的CodePreset配置
     * @details 支持智能匹配，例如AG2301会匹配到AG的配置
     */
    CodePreset get(const std::string& code);

    /**
     * @brief 手动添加预设配置
     * @param code 合约代码
     * @param preset 预设配置
     */
    void add_preset(const std::string& code, const CodePreset& preset);

    /**
     * @brief 检查是否包含指定合约的预设
     * @param code 合约代码
     * @return 是否包含
     */
    bool contains(const std::string& code) const;

    /**
     * @brief 获取所有预设的合约代码列表
     * @return 合约代码列表
     */
    std::vector<std::string> get_all_codes() const;

    /**
     * @brief 按交易所筛选预设
     * @param exchange 交易所代码
     * @return 该交易所的所有预设
     */
    std::vector<CodePreset> get_by_exchange(const std::string& exchange) const;

    // 序列化方法
    nlohmann::json to_json() const;
    static MarketPreset from_json(const nlohmann::json& j);

private:
    /**
     * @brief 初始化所有预设配置 - 匹配Rust中的硬编码配置
     */
    void init_all_presets();

    /**
     * @brief 提取合约代码的字母部分
     * @param code 完整合约代码
     * @return 字母部分
     */
    std::string extract_symbol(const std::string& code) const;
};

} // namespace qaultra::account