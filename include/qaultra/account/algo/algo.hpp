#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <memory>
#include <chrono>
#include <random>
#include <nlohmann/json.hpp>

// 前向声明
namespace qaultra::account {
    class Order;
}

namespace qaultra::account::algo {

/**
 * @brief 订单分割算法枚举 - 完全匹配Rust实现
 */
enum class SplitAlgorithm {
    TWAP,       // 时间加权平均价格算法
    VWAP,       // 成交量加权平均价格算法
    Iceberg,    // 冰山算法 - 随机大小分割
    Custom      // 自定义算法
};

/**
 * @brief 订单分割参数 - 完全匹配Rust SplitParams
 */
struct SplitParams {
    size_t chunks = 5;                                  // 分割块数量
    uint64_t interval = 60;                             // 时间间隔(秒)
    uint8_t price_strategy = 0;                         // 价格策略(0=固定, 1=跟随市场, 2=激进)
    double max_deviation = 0.005;                       // 最大价格偏差(百分比)
    double min_chunk_size = 1.0;                        // 最小块大小
    double random_factor = 0.0;                         // 随机因子(0.0-1.0)
    std::unordered_map<std::string, double> extra_params; // 额外参数

    /**
     * @brief 默认构造函数
     */
    SplitParams() {
        extra_params["price_increment"] = 0.0;
    }

    nlohmann::json to_json() const;
    static SplitParams from_json(const nlohmann::json& j);
};

/**
 * @brief 订单块状态枚举 - 完全匹配Rust ChunkStatus
 */
enum class ChunkStatus {
    Pending,            // 等待执行
    Sent,               // 已发送
    PartiallyFilled,    // 部分成交
    Filled,             // 完全成交
    Failed,             // 执行失败
    Cancelled           // 已取消
};

/**
 * @brief 订单分割块 - 完全匹配Rust SplitOrderChunk
 */
struct SplitOrderChunk {
    std::string chunk_id;                       // 块ID
    std::optional<std::string> order_id;        // 关联的订单ID
    double amount = 0.0;                        // 数量
    double target_price = 0.0;                  // 目标价格
    std::optional<double> executed_price;       // 实际成交价格
    std::string scheduled_time;                 // 预定执行时间
    std::optional<std::string> execution_time;  // 实际执行时间
    ChunkStatus status = ChunkStatus::Pending;  // 状态
    std::string failure_reason;                 // 失败原因(仅当status为Failed时有效)
    double partially_filled_amount = 0.0;       // 部分成交数量

    nlohmann::json to_json() const;
    static SplitOrderChunk from_json(const nlohmann::json& j);
};

/**
 * @brief 订单分割执行计划 - 完全匹配Rust SplitOrderPlan
 */
class SplitOrderPlan {
private:
    std::mt19937 rng_;  // 随机数生成器

public:
    // 原始订单信息
    std::string original_order_id;              // 原始订单ID
    std::string code;                           // 合约代码
    double total_amount = 0.0;                  // 总数量
    double base_price = 0.0;                    // 基础价格
    int direction = 1;                          // 方向标识
    std::string start_time;                     // 开始时间

    // 分割策略
    SplitAlgorithm algorithm = SplitAlgorithm::TWAP;  // 算法类型
    SplitParams params;                               // 参数
    std::unordered_map<std::string, double> custom_params; // 自定义参数

    // 执行状态
    std::vector<SplitOrderChunk> chunks;        // 所有分割块
    size_t executed_chunks = 0;                 // 已执行块数
    double total_executed = 0.0;                // 总执行数量
    bool is_completed = false;                  // 是否完成
    bool is_cancelled = false;                  // 是否取消

    // 执行统计
    double avg_executed_price = 0.0;            // 平均执行价格
    std::optional<std::string> execution_start_time;  // 执行开始时间
    std::optional<std::string> execution_end_time;    // 执行结束时间

public:
    /**
     * @brief 构造函数 - 匹配Rust new方法
     */
    SplitOrderPlan(const std::string& order_id,
                   const std::string& code,
                   double total_amount,
                   double base_price,
                   int direction,
                   SplitAlgorithm algorithm,
                   const SplitParams& params);

    /**
     * @brief 生成执行计划 - 匹配Rust generate_plan方法
     */
    void generate_plan();

    /**
     * @brief 执行下一个块 - 匹配Rust execute_next_chunk方法
     */
    template<typename ExecuteFunc>
    std::optional<std::pair<bool, std::string>> execute_next_chunk(ExecuteFunc&& execute_func);

    /**
     * @brief 更新块状态 - 匹配Rust update_chunk_status方法
     */
    void update_chunk_status(const std::string& chunk_id,
                            ChunkStatus status,
                            std::optional<double> executed_price = std::nullopt,
                            const std::string& failure_reason = "");

    /**
     * @brief 取消剩余块 - 匹配Rust cancel_remaining方法
     */
    void cancel_remaining();

    /**
     * @brief 获取执行进度
     */
    double get_progress() const;

    /**
     * @brief 获取当前状态摘要
     */
    std::string get_status_summary() const;

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static SplitOrderPlan from_json(const nlohmann::json& j);

private:
    /**
     * @brief 生成TWAP计划 - 匹配Rust generate_twap_plan方法
     */
    void generate_twap_plan();

    /**
     * @brief 生成VWAP计划 - 匹配Rust generate_vwap_plan方法
     */
    void generate_vwap_plan();

    /**
     * @brief 生成冰山计划 - 匹配Rust generate_iceberg_plan方法
     */
    void generate_iceberg_plan();

    /**
     * @brief 生成自定义计划
     */
    void generate_custom_plan();

    /**
     * @brief 更新执行状态 - 匹配Rust update_execution_status方法
     */
    void update_execution_status();

    /**
     * @brief 获取当前时间字符串
     */
    std::string get_current_time() const;

    /**
     * @brief 计算调度时间
     */
    std::string calculate_scheduled_time(size_t chunk_index) const;
};

/**
 * @brief 算法订单管理器 - 完全匹配Rust AlgoOrderManager
 */
class AlgoOrderManager {
private:
    std::unordered_map<std::string, std::unique_ptr<SplitOrderPlan>> plans_;

public:
    /**
     * @brief 构造函数
     */
    AlgoOrderManager() = default;

    /**
     * @brief 析构函数
     */
    ~AlgoOrderManager() = default;

    // 禁止拷贝，允许移动
    AlgoOrderManager(const AlgoOrderManager&) = delete;
    AlgoOrderManager& operator=(const AlgoOrderManager&) = delete;
    AlgoOrderManager(AlgoOrderManager&&) = default;
    AlgoOrderManager& operator=(AlgoOrderManager&&) = default;

    /**
     * @brief 创建分割订单 - 匹配Rust create_split_order方法
     */
    std::string create_split_order(const std::string& code,
                                  double total_amount,
                                  double base_price,
                                  int direction,
                                  SplitAlgorithm algorithm,
                                  std::optional<SplitParams> params = std::nullopt);

    /**
     * @brief 执行下一个块 - 使用函数对象
     */
    template<typename ExecuteFunc>
    std::optional<std::pair<bool, std::string>> execute_next_chunk(
        const std::string& order_id, ExecuteFunc&& execute_func);

    /**
     * @brief 更新块状态
     */
    bool update_chunk_status(const std::string& order_id,
                            const std::string& chunk_id,
                            ChunkStatus status,
                            std::optional<double> executed_price = std::nullopt,
                            const std::string& failure_reason = "");

    /**
     * @brief 取消分割订单
     */
    bool cancel_split_order(const std::string& order_id);

    /**
     * @brief 获取计划
     */
    SplitOrderPlan* get_plan(const std::string& order_id);
    const SplitOrderPlan* get_plan(const std::string& order_id) const;

    /**
     * @brief 获取所有计划ID
     */
    std::vector<std::string> get_all_plan_ids() const;

    /**
     * @brief 获取活跃计划数量
     */
    size_t get_active_plan_count() const;

    /**
     * @brief 清理已完成的计划
     */
    void cleanup_completed_plans();

    /**
     * @brief 定期更新所有计划(供定时器调用)
     */
    template<typename ExecuteFunc>
    void update_all_plans(ExecuteFunc&& execute_func);

    /**
     * @brief 获取管理器状态
     */
    nlohmann::json get_manager_status() const;

    /**
     * @brief 序列化
     */
    nlohmann::json to_json() const;
    static std::unique_ptr<AlgoOrderManager> from_json(const nlohmann::json& j);

private:
    /**
     * @brief 生成唯一订单ID
     */
    std::string generate_order_id() const;
};

/**
 * @brief 算法工具函数命名空间
 */
namespace utils {
    /**
     * @brief 将算法枚举转换为字符串
     */
    std::string algorithm_to_string(SplitAlgorithm algo);

    /**
     * @brief 将字符串转换为算法枚举
     */
    SplitAlgorithm string_to_algorithm(const std::string& str);

    /**
     * @brief 将状态枚举转换为字符串
     */
    std::string status_to_string(ChunkStatus status);

    /**
     * @brief 将字符串转换为状态枚举
     */
    ChunkStatus string_to_status(const std::string& str);

    /**
     * @brief 创建预设的TWAP参数
     */
    SplitParams create_twap_params(size_t chunks, uint64_t interval);

    /**
     * @brief 创建预设的VWAP参数
     */
    SplitParams create_vwap_params(size_t chunks, uint64_t interval);

    /**
     * @brief 创建预设的冰山参数
     */
    SplitParams create_iceberg_params(size_t chunks, double min_chunk_size, double random_factor);

    /**
     * @brief 计算执行统计
     */
    struct ExecutionStats {
        double total_executed = 0.0;       // 总执行量
        double avg_price = 0.0;             // 平均价格
        double price_variance = 0.0;        // 价格方差
        size_t successful_chunks = 0;       // 成功块数
        size_t failed_chunks = 0;           // 失败块数
        std::chrono::milliseconds total_time{0}; // 总执行时间
    };

    ExecutionStats calculate_execution_stats(const SplitOrderPlan& plan);

    /**
     * @brief 验证分割参数有效性
     */
    bool validate_split_params(const SplitParams& params, double total_amount);
}

// 模板实现

template<typename ExecuteFunc>
std::optional<std::pair<bool, std::string>> SplitOrderPlan::execute_next_chunk(ExecuteFunc&& execute_func) {
    if (is_completed || is_cancelled) {
        return std::nullopt;
    }

    // 找到下一个待执行的块
    auto next_chunk_it = std::find_if(chunks.begin(), chunks.end(),
        [](const SplitOrderChunk& chunk) {
            return chunk.status == ChunkStatus::Pending;
        });

    if (next_chunk_it == chunks.end()) {
        return std::nullopt;
    }

    auto& chunk = *next_chunk_it;
    std::string now = get_current_time();

    // 记录执行开始时间
    if (!execution_start_time) {
        execution_start_time = now;
    }

    // 执行订单
    try {
        auto result = execute_func(code, chunk.amount, now, chunk.target_price, direction);

        if (result.has_value()) {
            // 执行成功
            chunk.order_id = result->first;  // 假设返回pair<order_id, success>
            chunk.status = ChunkStatus::Sent;
            chunk.execution_time = now;

            update_execution_status();
            return std::make_pair(true, chunk.chunk_id);
        } else {
            // 执行失败
            chunk.status = ChunkStatus::Failed;
            chunk.failure_reason = "Order execution failed";

            update_execution_status();
            return std::make_pair(false, chunk.chunk_id);
        }
    } catch (const std::exception& e) {
        chunk.status = ChunkStatus::Failed;
        chunk.failure_reason = e.what();
        update_execution_status();
        return std::make_pair(false, chunk.chunk_id);
    }
}

template<typename ExecuteFunc>
std::optional<std::pair<bool, std::string>> AlgoOrderManager::execute_next_chunk(
    const std::string& order_id, ExecuteFunc&& execute_func) {

    auto plan_it = plans_.find(order_id);
    if (plan_it == plans_.end()) {
        return std::nullopt;
    }

    return plan_it->second->execute_next_chunk(std::forward<ExecuteFunc>(execute_func));
}

template<typename ExecuteFunc>
void AlgoOrderManager::update_all_plans(ExecuteFunc&& execute_func) {
    for (auto& [order_id, plan] : plans_) {
        if (!plan->is_completed && !plan->is_cancelled) {
            plan->execute_next_chunk(std::forward<ExecuteFunc>(execute_func));
        }
    }
}

} // namespace qaultra::account::algo