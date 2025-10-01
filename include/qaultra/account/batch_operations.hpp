#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>
#include "qa_account.hpp"
#include "position.hpp"
#include "../data/datatype.hpp"

namespace qaultra::account {

/**
 * @brief 原子化金融计算操作
 * 确保在并行环境下的数值精度和一致性
 *
 * 对应 Rust: src/qaaccount/parallel_ops.rs::AtomicFinancialOps
 */
class AtomicFinancialOps {
public:
    /**
     * @brief 原子化浮点数加法 - 用于并行求和
     * 使用 compare-exchange 循环确保线程安全
     */
    static double atomic_add_f64(std::atomic<uint64_t>& atomic_val, double value);

    /**
     * @brief 并行安全的浮动盈亏计算
     * 多线程并行计算所有持仓的浮动盈亏
     */
    static double parallel_float_profit_calculation(
        const std::unordered_map<std::string, QA_Position>& positions);

    /**
     * @brief 并行计算账户余额
     * balance = cash + frozen_cash + float_profit
     */
    static double parallel_balance_calculation(
        const std::unordered_map<std::string, QA_Position>& positions,
        double cash,
        double frozen_cash);

    /**
     * @brief 并行计算保证金使用
     * @return std::tuple<margin_long, margin_short, total_margin>
     */
    static std::tuple<double, double, double> parallel_margin_calculation(
        const std::unordered_map<std::string, QA_Position>& positions);

    // 辅助函数：将double转换为uint64_t用于原子操作 (公开以供 batch_utils 使用)
    static inline uint64_t double_to_bits(double value) {
        uint64_t bits;
        std::memcpy(&bits, &value, sizeof(double));
        return bits;
    }

    static inline double bits_to_double(uint64_t bits) {
        double value;
        std::memcpy(&value, &bits, sizeof(uint64_t));
        return value;
    }
};

/**
 * @brief 并行安全的仓位管理器
 * 支持多线程并发访问和更新持仓数据
 *
 * 对应 Rust: src/qaaccount/parallel_ops.rs::ConcurrentPositionManager
 */
class ConcurrentPositionManager {
private:
    // 持仓数据 - 使用共享锁保护
    mutable std::shared_mutex positions_mutex_;
    std::unordered_map<std::string, std::shared_ptr<QA_Position>> positions_;

    // 余额缓存 - 原子操作
    std::atomic<uint64_t> balance_cache_{0};
    std::atomic<uint64_t> last_update_time_{0};

public:
    ConcurrentPositionManager() = default;
    ~ConcurrentPositionManager() = default;

    // 禁止拷贝，允许移动
    ConcurrentPositionManager(const ConcurrentPositionManager&) = delete;
    ConcurrentPositionManager& operator=(const ConcurrentPositionManager&) = delete;
    ConcurrentPositionManager(ConcurrentPositionManager&&) = default;
    ConcurrentPositionManager& operator=(ConcurrentPositionManager&&) = default;

    /**
     * @brief 并行价格更新 - 原子化操作
     * @param price_updates 价格更新列表: [(code, price, datetime), ...]
     */
    void parallel_price_update(
        const std::vector<std::tuple<std::string, double, std::string>>& price_updates);

    /**
     * @brief 批量获取持仓快照
     * @return 所有持仓的只读副本
     */
    std::unordered_map<std::string, QA_Position> get_positions_snapshot() const;

    /**
     * @brief 添加或更新持仓
     */
    void update_position(const std::string& code, const QA_Position& position);

    /**
     * @brief 获取指定持仓
     */
    std::shared_ptr<QA_Position> get_position(const std::string& code) const;

    /**
     * @brief 获取所有持仓代码
     */
    std::vector<std::string> get_position_codes() const;

    /**
     * @brief 获取缓存的余额
     */
    double get_cached_balance() const;

    /**
     * @brief 更新缓存的余额
     */
    void update_cached_balance(double new_balance);

    /**
     * @brief 清空所有持仓
     */
    void clear();

    /**
     * @brief 获取持仓数量
     */
    size_t size() const;
};

/**
 * @brief 批量订单处理器
 * 高性能批量处理大量订单
 */
class BatchOrderProcessor {
private:
    size_t batch_size_ = 1000;           // 批处理大小
    size_t max_workers_ = 4;             // 最大工作线程数
    bool async_mode_ = true;             // 是否异步处理

public:
    /**
     * @brief 构造函数
     * @param batch_size 批处理大小
     * @param max_workers 最大工作线程数
     */
    explicit BatchOrderProcessor(size_t batch_size = 1000, size_t max_workers = 4)
        : batch_size_(batch_size)
        , max_workers_(max_workers > 0 ? max_workers : std::thread::hardware_concurrency())
    {}

    /**
     * @brief 批量下单
     * @param accounts 账户列表
     * @param orders 订单列表
     * @return 成功下单数量
     */
    size_t batch_place_orders(
        std::vector<std::shared_ptr<QA_Account>>& accounts,
        const std::vector<Order>& orders);

    /**
     * @brief 批量撤单
     * @param accounts 账户列表
     * @param order_ids 订单ID列表
     * @return 成功撤单数量
     */
    size_t batch_cancel_orders(
        std::vector<std::shared_ptr<QA_Account>>& accounts,
        const std::vector<std::string>& order_ids);

    /**
     * @brief 批量账户结算
     * 并行处理多个账户的结算操作
     * @param accounts 账户列表
     */
    void batch_settle_accounts(std::vector<std::shared_ptr<QA_Account>>& accounts);

    /**
     * @brief 批量盈亏计算
     * @param accounts 账户列表
     * @return 账户ID -> 盈亏映射
     */
    std::unordered_map<std::string, double> batch_calculate_pnl(
        const std::vector<std::shared_ptr<QA_Account>>& accounts) const;

    /**
     * @brief 设置批处理大小
     */
    void set_batch_size(size_t size) { batch_size_ = size; }

    /**
     * @brief 设置最大工作线程数
     */
    void set_max_workers(size_t workers) { max_workers_ = workers; }

    /**
     * @brief 设置异步模式
     */
    void set_async_mode(bool async) { async_mode_ = async; }

private:
    /**
     * @brief 将任务分割成批次
     */
    template<typename T>
    std::vector<std::vector<T>> split_into_batches(const std::vector<T>& items) const {
        std::vector<std::vector<T>> batches;
        size_t total = items.size();

        for (size_t i = 0; i < total; i += batch_size_) {
            size_t end = std::min(i + batch_size_, total);
            batches.emplace_back(items.begin() + i, items.begin() + end);
        }

        return batches;
    }
};

/**
 * @brief 账户性能统计器
 * 并行计算多个账户的性能指标
 */
class AccountPerformanceCalculator {
public:
    struct PerformanceMetrics {
        double total_return = 0.0;          // 总收益率
        double sharpe_ratio = 0.0;          // 夏普比率
        double max_drawdown = 0.0;          // 最大回撤
        double win_rate = 0.0;              // 胜率
        size_t total_trades = 0;            // 总交易次数
        size_t winning_trades = 0;          // 盈利交易次数
        double average_profit = 0.0;        // 平均盈利
        double average_loss = 0.0;          // 平均亏损
        double profit_factor = 0.0;         // 盈亏比
    };

    /**
     * @brief 并行计算多个账户的性能指标
     * @param accounts 账户列表
     * @return 账户ID -> 性能指标映射
     */
    static std::unordered_map<std::string, PerformanceMetrics>
    parallel_calculate_performance(
        const std::vector<std::shared_ptr<QA_Account>>& accounts);

    /**
     * @brief 计算单个账户的性能指标
     */
    static PerformanceMetrics calculate_single_account_performance(
        const QA_Account& account);

private:
    /**
     * @brief 计算夏普比率
     */
    static double calculate_sharpe_ratio(const std::vector<double>& returns,
                                         double risk_free_rate = 0.03);

    /**
     * @brief 计算最大回撤
     */
    static double calculate_max_drawdown(const std::vector<double>& equity_curve);
};

/**
 * @brief 批量操作工具函数
 */
namespace batch_utils {
    /**
     * @brief 并行应用函数到多个账户
     * @param accounts 账户列表
     * @param func 要应用的函数
     */
    template<typename Func>
    void parallel_apply(std::vector<std::shared_ptr<QA_Account>>& accounts,
                       Func&& func) {
        const size_t num_threads = std::min(
            accounts.size(),
            static_cast<size_t>(std::thread::hardware_concurrency())
        );

        std::vector<std::future<void>> futures;
        futures.reserve(num_threads);

        size_t chunk_size = (accounts.size() + num_threads - 1) / num_threads;

        for (size_t i = 0; i < num_threads; ++i) {
            size_t start = i * chunk_size;
            size_t end = std::min(start + chunk_size, accounts.size());

            if (start >= accounts.size()) break;

            futures.push_back(std::async(std::launch::async, [&accounts, &func, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    func(*accounts[j]);
                }
            }));
        }

        // 等待所有任务完成
        for (auto& future : futures) {
            future.get();
        }
    }

    /**
     * @brief 并行聚合多个账户的数值结果
     */
    template<typename Func>
    double parallel_aggregate(
        const std::vector<std::shared_ptr<QA_Account>>& accounts,
        Func&& func,
        double initial_value = 0.0) {

        std::atomic<uint64_t> result{AtomicFinancialOps::double_to_bits(initial_value)};

        parallel_apply(const_cast<std::vector<std::shared_ptr<QA_Account>>&>(accounts),
            [&result, &func](const QA_Account& account) {
                double value = func(account);
                AtomicFinancialOps::atomic_add_f64(result, value);
            });

        return AtomicFinancialOps::bits_to_double(result.load());
    }
}

} // namespace qaultra::account
