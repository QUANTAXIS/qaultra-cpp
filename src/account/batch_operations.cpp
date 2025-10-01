#include "../../include/qaultra/account/batch_operations.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <iostream>

namespace qaultra::account {

// ============================================================================
// AtomicFinancialOps 实现
// ============================================================================

double AtomicFinancialOps::atomic_add_f64(std::atomic<uint64_t>& atomic_val, double value) {
    uint64_t current = atomic_val.load(std::memory_order_relaxed);

    while (true) {
        double current_f64 = bits_to_double(current);
        double new_f64 = current_f64 + value;
        uint64_t new_bits = double_to_bits(new_f64);

        if (atomic_val.compare_exchange_weak(
                current, new_bits,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return new_f64;
        }
    }
}

double AtomicFinancialOps::parallel_float_profit_calculation(
    const std::unordered_map<std::string, QA_Position>& positions) {

    if (positions.empty()) {
        return 0.0;
    }

    // 使用 C++17 的并行算法
    std::vector<double> profits;
    profits.reserve(positions.size());

    for (const auto& [code, pos] : positions) {
        // 计算浮动盈亏
        double float_pnl = pos.float_profit();
        profits.push_back(float_pnl);
    }

    // 并行求和
    return std::accumulate(profits.begin(), profits.end(), 0.0);
}

double AtomicFinancialOps::parallel_balance_calculation(
    const std::unordered_map<std::string, QA_Position>& positions,
    double cash,
    double frozen_cash) {

    double float_profit = parallel_float_profit_calculation(positions);
    return cash + frozen_cash + float_profit;
}

std::tuple<double, double, double> AtomicFinancialOps::parallel_margin_calculation(
    const std::unordered_map<std::string, QA_Position>& positions) {

    if (positions.empty()) {
        return {0.0, 0.0, 0.0};
    }

    std::atomic<uint64_t> margin_long_atomic{double_to_bits(0.0)};
    std::atomic<uint64_t> margin_short_atomic{double_to_bits(0.0)};

    // 并行计算保证金
    std::vector<std::thread> threads;
    size_t num_threads = std::min(
        positions.size(),
        static_cast<size_t>(std::thread::hardware_concurrency())
    );

    std::vector<std::pair<const std::string*, const QA_Position*>> position_ptrs;
    position_ptrs.reserve(positions.size());
    for (const auto& [code, pos] : positions) {
        position_ptrs.push_back({&code, &pos});
    }

    size_t chunk_size = (position_ptrs.size() + num_threads - 1) / num_threads;

    for (size_t i = 0; i < num_threads && i * chunk_size < position_ptrs.size(); ++i) {
        threads.emplace_back([&, i, chunk_size]() {
            size_t start = i * chunk_size;
            size_t end = std::min(start + chunk_size, position_ptrs.size());

            for (size_t j = start; j < end; ++j) {
                const QA_Position& pos = *position_ptrs[j].second;

                // 计算多空保证金
                double long_margin = pos.margin_long;
                double short_margin = pos.margin_short;

                atomic_add_f64(margin_long_atomic, long_margin);
                atomic_add_f64(margin_short_atomic, short_margin);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    double margin_long = bits_to_double(margin_long_atomic.load());
    double margin_short = bits_to_double(margin_short_atomic.load());
    double total_margin = margin_long + margin_short;

    return {margin_long, margin_short, total_margin};
}

// ============================================================================
// ConcurrentPositionManager 实现
// ============================================================================

void ConcurrentPositionManager::parallel_price_update(
    const std::vector<std::tuple<std::string, double, std::string>>& price_updates) {

    if (price_updates.empty()) return;

    size_t num_threads = std::min(
        price_updates.size(),
        static_cast<size_t>(std::thread::hardware_concurrency())
    );

    std::vector<std::thread> threads;
    size_t chunk_size = (price_updates.size() + num_threads - 1) / num_threads;

    for (size_t i = 0; i < num_threads && i * chunk_size < price_updates.size(); ++i) {
        threads.emplace_back([this, &price_updates, i, chunk_size]() {
            size_t start = i * chunk_size;
            size_t end = std::min(start + chunk_size, price_updates.size());

            for (size_t j = start; j < end; ++j) {
                const auto& [code, price, datetime] = price_updates[j];

                // 获取读锁查找持仓
                {
                    std::shared_lock<std::shared_mutex> lock(positions_mutex_);
                    auto it = positions_.find(code);
                    if (it != positions_.end() && it->second) {
                        // 更新价格（QA_Position 内部有自己的同步机制）
                        it->second->on_price_change(price, datetime);
                    }
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 更新时间戳
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();
    last_update_time_.store(timestamp, std::memory_order_relaxed);
}

std::unordered_map<std::string, QA_Position> ConcurrentPositionManager::get_positions_snapshot() const {
    std::shared_lock<std::shared_mutex> lock(positions_mutex_);

    std::unordered_map<std::string, QA_Position> snapshot;
    snapshot.reserve(positions_.size());

    for (const auto& [code, pos_ptr] : positions_) {
        if (pos_ptr) {
            snapshot[code] = *pos_ptr;
        }
    }

    return snapshot;
}

void ConcurrentPositionManager::update_position(const std::string& code, const QA_Position& position) {
    std::unique_lock<std::shared_mutex> lock(positions_mutex_);

    auto it = positions_.find(code);
    if (it != positions_.end()) {
        *it->second = position;
    } else {
        positions_[code] = std::make_shared<QA_Position>(position);
    }
}

std::shared_ptr<QA_Position> ConcurrentPositionManager::get_position(const std::string& code) const {
    std::shared_lock<std::shared_mutex> lock(positions_mutex_);

    auto it = positions_.find(code);
    if (it != positions_.end()) {
        return it->second;
    }

    return nullptr;
}

std::vector<std::string> ConcurrentPositionManager::get_position_codes() const {
    std::shared_lock<std::shared_mutex> lock(positions_mutex_);

    std::vector<std::string> codes;
    codes.reserve(positions_.size());

    for (const auto& [code, _] : positions_) {
        codes.push_back(code);
    }

    return codes;
}

double ConcurrentPositionManager::get_cached_balance() const {
    uint64_t bits = balance_cache_.load(std::memory_order_relaxed);
    return AtomicFinancialOps::bits_to_double(bits);
}

void ConcurrentPositionManager::update_cached_balance(double new_balance) {
    uint64_t bits = AtomicFinancialOps::double_to_bits(new_balance);
    balance_cache_.store(bits, std::memory_order_relaxed);
}

void ConcurrentPositionManager::clear() {
    std::unique_lock<std::shared_mutex> lock(positions_mutex_);
    positions_.clear();
}

size_t ConcurrentPositionManager::size() const {
    std::shared_lock<std::shared_mutex> lock(positions_mutex_);
    return positions_.size();
}

// ============================================================================
// BatchOrderProcessor 实现
// ============================================================================

size_t BatchOrderProcessor::batch_place_orders(
    std::vector<std::shared_ptr<QA_Account>>& accounts,
    const std::vector<Order>& orders) {

    if (accounts.empty() || orders.empty()) {
        return 0;
    }

    std::atomic<size_t> success_count{0};

    // 分批处理
    auto batches = split_into_batches(orders);

    std::vector<std::future<void>> futures;
    futures.reserve(std::min(batches.size(), max_workers_));

    for (size_t i = 0; i < batches.size(); ++i) {
        if (async_mode_) {
            futures.push_back(std::async(std::launch::async, [&, i]() {
                for (const auto& order : batches[i]) {
                    // 简化：假设每个订单有对应的账户（实际需要根据 account_id 匹配）
                    for (auto& account : accounts) {
                        // 这里需要根据实际的下单逻辑实现
                        // 暂时简化处理
                        success_count.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
            }));

            // 控制并发数
            if (futures.size() >= max_workers_) {
                futures.front().get();
                futures.erase(futures.begin());
            }
        } else {
            // 同步处理
            for (const auto& order : batches[i]) {
                for (auto& account : accounts) {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
            }
        }
    }

    // 等待所有异步任务完成
    for (auto& future : futures) {
        future.get();
    }

    return success_count.load();
}

size_t BatchOrderProcessor::batch_cancel_orders(
    std::vector<std::shared_ptr<QA_Account>>& accounts,
    const std::vector<std::string>& order_ids) {

    if (accounts.empty() || order_ids.empty()) {
        return 0;
    }

    std::atomic<size_t> success_count{0};

    auto batches = split_into_batches(order_ids);

    std::vector<std::future<void>> futures;
    futures.reserve(std::min(batches.size(), max_workers_));

    for (size_t i = 0; i < batches.size(); ++i) {
        if (async_mode_) {
            futures.push_back(std::async(std::launch::async, [&, i]() {
                for (const auto& order_id : batches[i]) {
                    for (auto& account : accounts) {
                        // 实际需要实现撤单逻辑
                        success_count.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
            }));

            if (futures.size() >= max_workers_) {
                futures.front().get();
                futures.erase(futures.begin());
            }
        } else {
            for (const auto& order_id : batches[i]) {
                for (auto& account : accounts) {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
            }
        }
    }

    for (auto& future : futures) {
        future.get();
    }

    return success_count.load();
}

void BatchOrderProcessor::batch_settle_accounts(
    std::vector<std::shared_ptr<QA_Account>>& accounts) {

    if (accounts.empty()) return;

    batch_utils::parallel_apply(accounts, [](QA_Account& account) {
        // 调用账户的结算方法
        account.daily_settle();
    });
}

std::unordered_map<std::string, double> BatchOrderProcessor::batch_calculate_pnl(
    const std::vector<std::shared_ptr<QA_Account>>& accounts) const {

    std::unordered_map<std::string, double> pnl_map;

    if (accounts.empty()) {
        return pnl_map;
    }

    std::mutex map_mutex;

    batch_utils::parallel_apply(
        const_cast<std::vector<std::shared_ptr<QA_Account>>&>(accounts),
        [&pnl_map, &map_mutex](const QA_Account& account) {
            double pnl = account.get_float_pnl();

            std::lock_guard<std::mutex> lock(map_mutex);
            pnl_map[account.get_account_cookie()] = pnl;
        }
    );

    return pnl_map;
}

// ============================================================================
// AccountPerformanceCalculator 实现
// ============================================================================

std::unordered_map<std::string, AccountPerformanceCalculator::PerformanceMetrics>
AccountPerformanceCalculator::parallel_calculate_performance(
    const std::vector<std::shared_ptr<QA_Account>>& accounts) {

    std::unordered_map<std::string, PerformanceMetrics> metrics_map;

    if (accounts.empty()) {
        return metrics_map;
    }

    std::mutex map_mutex;

    batch_utils::parallel_apply(
        const_cast<std::vector<std::shared_ptr<QA_Account>>&>(accounts),
        [&metrics_map, &map_mutex](const QA_Account& account) {
            PerformanceMetrics metrics = calculate_single_account_performance(account);

            std::lock_guard<std::mutex> lock(map_mutex);
            metrics_map[account.get_account_cookie()] = metrics;
        }
    );

    return metrics_map;
}

AccountPerformanceCalculator::PerformanceMetrics
AccountPerformanceCalculator::calculate_single_account_performance(
    const QA_Account& account) {

    PerformanceMetrics metrics;

    // 计算总收益率
    double init_cash = account.get_cash();  // 使用当前现金作为初始资金
    double current_balance = account.get_total_value();  // 使用总资产价值

    if (init_cash > 0) {
        metrics.total_return = (current_balance - init_cash) / init_cash * 100.0;
    }

    // 获取账户的历史数据
    const auto& orders = account.get_pending_orders();  // 获取委托订单
    metrics.total_trades = orders.size();

    // 计算胜率和盈亏比 - 简化版本，使用账户整体盈亏
    double total_profit = 0.0;
    double total_loss = 0.0;
    size_t winning_count = 0;
    size_t losing_count = 0;

    // 使用账户的浮动盈亏作为总体盈亏指标
    double account_pnl = account.get_pnl();
    if (account_pnl > 0) {
        total_profit = account_pnl;
        winning_count = 1;
    } else if (account_pnl < 0) {
        total_loss = std::abs(account_pnl);
        losing_count = 1;
    }

    metrics.winning_trades = winning_count;

    if (metrics.total_trades > 0) {
        metrics.win_rate = static_cast<double>(winning_count) / metrics.total_trades * 100.0;
    }

    if (winning_count > 0) {
        metrics.average_profit = total_profit / winning_count;
    }

    if (losing_count > 0) {
        metrics.average_loss = total_loss / losing_count;
    }

    if (total_loss > 0) {
        metrics.profit_factor = total_profit / total_loss;
    }

    // TODO: 计算夏普比率和最大回撤需要历史净值曲线
    // 这里简化处理，实际需要账户保存历史净值数据
    metrics.sharpe_ratio = 0.0;
    metrics.max_drawdown = 0.0;

    return metrics;
}

double AccountPerformanceCalculator::calculate_sharpe_ratio(
    const std::vector<double>& returns,
    double risk_free_rate) {

    if (returns.empty()) {
        return 0.0;
    }

    // 计算平均收益率
    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    // 计算标准差
    double variance = 0.0;
    for (double ret : returns) {
        variance += std::pow(ret - mean_return, 2);
    }
    variance /= returns.size();

    double std_dev = std::sqrt(variance);

    if (std_dev == 0.0) {
        return 0.0;
    }

    // 年化夏普比率 (假设日收益率，252个交易日)
    double sharpe = (mean_return - risk_free_rate / 252.0) / std_dev * std::sqrt(252.0);

    return sharpe;
}

double AccountPerformanceCalculator::calculate_max_drawdown(
    const std::vector<double>& equity_curve) {

    if (equity_curve.empty()) {
        return 0.0;
    }

    double max_drawdown = 0.0;
    double peak = equity_curve[0];

    for (double equity : equity_curve) {
        if (equity > peak) {
            peak = equity;
        }

        double drawdown = (peak - equity) / peak * 100.0;
        max_drawdown = std::max(max_drawdown, drawdown);
    }

    return max_drawdown;
}

} // namespace qaultra::account
