#include "../../../include/qaultra/account/algo/algo.hpp"
#include "../../../include/qaultra/account/order.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <uuid/uuid.h>

namespace qaultra::account::algo {

// SplitParams 实现
nlohmann::json SplitParams::to_json() const {
    nlohmann::json j;
    j["chunks"] = chunks;
    j["interval"] = interval;
    j["price_strategy"] = price_strategy;
    j["max_deviation"] = max_deviation;
    j["min_chunk_size"] = min_chunk_size;
    j["random_factor"] = random_factor;
    j["extra_params"] = extra_params;
    return j;
}

SplitParams SplitParams::from_json(const nlohmann::json& j) {
    SplitParams params;
    params.chunks = j.value("chunks", 5);
    params.interval = j.value("interval", 60);
    params.price_strategy = j.value("price_strategy", 0);
    params.max_deviation = j.value("max_deviation", 0.005);
    params.min_chunk_size = j.value("min_chunk_size", 1.0);
    params.random_factor = j.value("random_factor", 0.0);

    if (j.contains("extra_params")) {
        params.extra_params = j["extra_params"].get<std::unordered_map<std::string, double>>();
    }

    return params;
}

// SplitOrderChunk 实现
nlohmann::json SplitOrderChunk::to_json() const {
    nlohmann::json j;
    j["chunk_id"] = chunk_id;
    j["order_id"] = order_id.value_or("");
    j["amount"] = amount;
    j["target_price"] = target_price;
    j["executed_price"] = executed_price.value_or(0.0);
    j["scheduled_time"] = scheduled_time;
    j["execution_time"] = execution_time.value_or("");
    j["status"] = static_cast<int>(status);
    j["failure_reason"] = failure_reason;
    j["partially_filled_amount"] = partially_filled_amount;
    return j;
}

SplitOrderChunk SplitOrderChunk::from_json(const nlohmann::json& j) {
    SplitOrderChunk chunk;
    chunk.chunk_id = j.value("chunk_id", "");

    std::string order_id_str = j.value("order_id", "");
    if (!order_id_str.empty()) {
        chunk.order_id = order_id_str;
    }

    chunk.amount = j.value("amount", 0.0);
    chunk.target_price = j.value("target_price", 0.0);

    double exec_price = j.value("executed_price", 0.0);
    if (exec_price > 0.0) {
        chunk.executed_price = exec_price;
    }

    chunk.scheduled_time = j.value("scheduled_time", "");

    std::string exec_time_str = j.value("execution_time", "");
    if (!exec_time_str.empty()) {
        chunk.execution_time = exec_time_str;
    }

    chunk.status = static_cast<ChunkStatus>(j.value("status", 0));
    chunk.failure_reason = j.value("failure_reason", "");
    chunk.partially_filled_amount = j.value("partially_filled_amount", 0.0);

    return chunk;
}

// SplitOrderPlan 实现
SplitOrderPlan::SplitOrderPlan(const std::string& order_id,
                               const std::string& code,
                               double total_amount,
                               double base_price,
                               int direction,
                               SplitAlgorithm algorithm,
                               const SplitParams& params)
    : original_order_id(order_id)
    , code(code)
    , total_amount(total_amount)
    , base_price(base_price)
    , direction(direction)
    , algorithm(algorithm)
    , params(params)
    , rng_(std::random_device{}())
{
    start_time = get_current_time();
}

void SplitOrderPlan::generate_plan() {
    chunks.clear();

    switch (algorithm) {
        case SplitAlgorithm::TWAP:
            generate_twap_plan();
            break;
        case SplitAlgorithm::VWAP:
            generate_vwap_plan();
            break;
        case SplitAlgorithm::Iceberg:
            generate_iceberg_plan();
            break;
        case SplitAlgorithm::Custom:
            generate_custom_plan();
            break;
    }
}

void SplitOrderPlan::generate_twap_plan() {
    double chunk_size = total_amount / params.chunks;

    for (size_t i = 0; i < params.chunks; ++i) {
        SplitOrderChunk chunk;
        chunk.chunk_id = original_order_id + "-" + std::to_string(i);
        chunk.amount = chunk_size;
        chunk.target_price = base_price;
        chunk.scheduled_time = calculate_scheduled_time(i);
        chunk.status = ChunkStatus::Pending;

        chunks.push_back(chunk);
    }
}

void SplitOrderPlan::generate_vwap_plan() {
    // 基于历史成交量分布的简化实现
    std::vector<double> volume_profile;

    if (params.chunks <= 1) {
        volume_profile = {1.0};
    } else if (params.chunks == 2) {
        volume_profile = {0.6, 0.4};
    } else if (params.chunks == 3) {
        volume_profile = {0.4, 0.2, 0.4};
    } else {
        volume_profile.resize(params.chunks);
        size_t mid = params.chunks / 2;

        for (size_t i = 0; i < params.chunks; ++i) {
            double distance_from_mid = std::abs(static_cast<double>(i) - static_cast<double>(mid)) / mid;
            volume_profile[i] = 1.0 - 0.5 * distance_from_mid;
        }

        // 归一化
        double sum = std::accumulate(volume_profile.begin(), volume_profile.end(), 0.0);
        for (auto& v : volume_profile) {
            v /= sum;
        }
    }

    for (size_t i = 0; i < params.chunks; ++i) {
        SplitOrderChunk chunk;
        chunk.chunk_id = original_order_id + "-" + std::to_string(i);
        chunk.amount = total_amount * volume_profile[i];
        chunk.target_price = base_price;
        chunk.scheduled_time = calculate_scheduled_time(i);
        chunk.status = ChunkStatus::Pending;

        chunks.push_back(chunk);
    }
}

void SplitOrderPlan::generate_iceberg_plan() {
    double remaining = total_amount;
    size_t i = 0;

    std::uniform_real_distribution<double> dist(0.0, 1.0);

    while (remaining > 0.0 && i < params.chunks) {
        double max_chunk = std::min(remaining, total_amount / (params.chunks - i) * (1.0 + params.random_factor));
        double min_chunk = std::max(params.min_chunk_size, max_chunk * (1.0 - params.random_factor));

        double chunk_amount;
        if (i == params.chunks - 1) {
            // 最后一块：使用所有剩余量
            chunk_amount = remaining;
        } else if (remaining <= params.min_chunk_size * 2.0) {
            // 剩余量太少，全部使用
            chunk_amount = remaining;
        } else {
            // 生成随机大小
            double range = max_chunk - min_chunk;
            chunk_amount = min_chunk + dist(rng_) * range;

            // 确保不会创建太小的剩余块
            if (remaining - chunk_amount < params.min_chunk_size && i < params.chunks - 1) {
                chunk_amount = remaining - params.min_chunk_size;
            }
        }

        SplitOrderChunk chunk;
        chunk.chunk_id = original_order_id + "-" + std::to_string(i);
        chunk.amount = chunk_amount;
        chunk.target_price = base_price;
        chunk.scheduled_time = calculate_scheduled_time(i);
        chunk.status = ChunkStatus::Pending;

        chunks.push_back(chunk);

        remaining -= chunk_amount;
        ++i;
    }
}

void SplitOrderPlan::generate_custom_plan() {
    // 自定义算法的基础实现，可以根据custom_params进行定制
    if (custom_params.empty()) {
        // 默认使用TWAP策略
        generate_twap_plan();
        return;
    }

    // 这里可以根据custom_params实现特定的分割逻辑
    // 目前作为示例，简单均分
    double chunk_size = total_amount / params.chunks;

    for (size_t i = 0; i < params.chunks; ++i) {
        SplitOrderChunk chunk;
        chunk.chunk_id = original_order_id + "-" + std::to_string(i);
        chunk.amount = chunk_size;
        chunk.target_price = base_price;
        chunk.scheduled_time = calculate_scheduled_time(i);
        chunk.status = ChunkStatus::Pending;

        chunks.push_back(chunk);
    }
}

void SplitOrderPlan::update_chunk_status(const std::string& chunk_id,
                                        ChunkStatus status,
                                        std::optional<double> executed_price,
                                        const std::string& failure_reason) {
    auto chunk_it = std::find_if(chunks.begin(), chunks.end(),
        [&chunk_id](const SplitOrderChunk& chunk) {
            return chunk.chunk_id == chunk_id;
        });

    if (chunk_it != chunks.end()) {
        chunk_it->status = status;

        if (executed_price) {
            chunk_it->executed_price = executed_price;

            if (status == ChunkStatus::Filled) {
                total_executed += chunk_it->amount;
                executed_chunks++;

                // 更新平均执行价格
                double total_value = avg_executed_price * (total_executed - chunk_it->amount)
                                   + (*executed_price) * chunk_it->amount;
                avg_executed_price = total_value / total_executed;
            }
        }

        if (status == ChunkStatus::Failed) {
            chunk_it->failure_reason = failure_reason;
        }

        update_execution_status();
    }
}

void SplitOrderPlan::cancel_remaining() {
    for (auto& chunk : chunks) {
        if (chunk.status == ChunkStatus::Pending || chunk.status == ChunkStatus::Sent) {
            chunk.status = ChunkStatus::Cancelled;
        }
    }

    is_cancelled = true;
    update_execution_status();
}

double SplitOrderPlan::get_progress() const {
    if (total_amount == 0.0) return 0.0;
    return total_executed / total_amount;
}

std::string SplitOrderPlan::get_status_summary() const {
    std::ostringstream oss;
    oss << "Plan " << original_order_id << ": ";
    oss << std::fixed << std::setprecision(2);
    oss << (get_progress() * 100.0) << "% complete, ";
    oss << executed_chunks << "/" << chunks.size() << " chunks executed";

    if (is_completed) {
        oss << " [COMPLETED]";
    } else if (is_cancelled) {
        oss << " [CANCELLED]";
    } else {
        oss << " [ACTIVE]";
    }

    return oss.str();
}

nlohmann::json SplitOrderPlan::to_json() const {
    nlohmann::json j;
    j["original_order_id"] = original_order_id;
    j["code"] = code;
    j["total_amount"] = total_amount;
    j["base_price"] = base_price;
    j["direction"] = direction;
    j["start_time"] = start_time;
    j["algorithm"] = static_cast<int>(algorithm);
    j["params"] = params.to_json();

    nlohmann::json chunks_json = nlohmann::json::array();
    for (const auto& chunk : chunks) {
        chunks_json.push_back(chunk.to_json());
    }
    j["chunks"] = chunks_json;

    j["executed_chunks"] = executed_chunks;
    j["total_executed"] = total_executed;
    j["is_completed"] = is_completed;
    j["is_cancelled"] = is_cancelled;
    j["avg_executed_price"] = avg_executed_price;
    j["execution_start_time"] = execution_start_time.value_or("");
    j["execution_end_time"] = execution_end_time.value_or("");

    return j;
}

SplitOrderPlan SplitOrderPlan::from_json(const nlohmann::json& j) {
    SplitOrderPlan plan(
        j.value("original_order_id", ""),
        j.value("code", ""),
        j.value("total_amount", 0.0),
        j.value("base_price", 0.0),
        j.value("direction", 1),
        static_cast<SplitAlgorithm>(j.value("algorithm", 0)),
        j.contains("params") ? SplitParams::from_json(j["params"]) : SplitParams{}
    );

    plan.start_time = j.value("start_time", "");
    plan.executed_chunks = j.value("executed_chunks", 0);
    plan.total_executed = j.value("total_executed", 0.0);
    plan.is_completed = j.value("is_completed", false);
    plan.is_cancelled = j.value("is_cancelled", false);
    plan.avg_executed_price = j.value("avg_executed_price", 0.0);

    std::string exec_start = j.value("execution_start_time", "");
    if (!exec_start.empty()) {
        plan.execution_start_time = exec_start;
    }

    std::string exec_end = j.value("execution_end_time", "");
    if (!exec_end.empty()) {
        plan.execution_end_time = exec_end;
    }

    if (j.contains("chunks")) {
        for (const auto& chunk_json : j["chunks"]) {
            plan.chunks.push_back(SplitOrderChunk::from_json(chunk_json));
        }
    }

    return plan;
}

void SplitOrderPlan::update_execution_status() {
    bool all_processed = std::all_of(chunks.begin(), chunks.end(),
        [](const SplitOrderChunk& chunk) {
            return chunk.status != ChunkStatus::Pending && chunk.status != ChunkStatus::Sent;
        });

    if (all_processed) {
        is_completed = true;
        execution_end_time = get_current_time();
    }
}

std::string SplitOrderPlan::get_current_time() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string SplitOrderPlan::calculate_scheduled_time(size_t chunk_index) const {
    auto now = std::chrono::system_clock::now();
    auto scheduled = now + std::chrono::seconds(chunk_index * params.interval);
    auto time_t = std::chrono::system_clock::to_time_t(scheduled);

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// AlgoOrderManager 实现
std::string AlgoOrderManager::create_split_order(const std::string& code,
                                                 double total_amount,
                                                 double base_price,
                                                 int direction,
                                                 SplitAlgorithm algorithm,
                                                 std::optional<SplitParams> params) {
    std::string order_id = generate_order_id();
    SplitParams final_params = params.value_or(SplitParams{});

    auto plan = std::make_unique<SplitOrderPlan>(
        order_id, code, total_amount, base_price, direction, algorithm, final_params);

    plan->generate_plan();
    plans_[order_id] = std::move(plan);

    return order_id;
}

bool AlgoOrderManager::update_chunk_status(const std::string& order_id,
                                          const std::string& chunk_id,
                                          ChunkStatus status,
                                          std::optional<double> executed_price,
                                          const std::string& failure_reason) {
    auto plan_it = plans_.find(order_id);
    if (plan_it == plans_.end()) {
        return false;
    }

    plan_it->second->update_chunk_status(chunk_id, status, executed_price, failure_reason);
    return true;
}

bool AlgoOrderManager::cancel_split_order(const std::string& order_id) {
    auto plan_it = plans_.find(order_id);
    if (plan_it == plans_.end()) {
        return false;
    }

    plan_it->second->cancel_remaining();
    return true;
}

SplitOrderPlan* AlgoOrderManager::get_plan(const std::string& order_id) {
    auto plan_it = plans_.find(order_id);
    return (plan_it != plans_.end()) ? plan_it->second.get() : nullptr;
}

const SplitOrderPlan* AlgoOrderManager::get_plan(const std::string& order_id) const {
    auto plan_it = plans_.find(order_id);
    return (plan_it != plans_.end()) ? plan_it->second.get() : nullptr;
}

std::vector<std::string> AlgoOrderManager::get_all_plan_ids() const {
    std::vector<std::string> ids;
    ids.reserve(plans_.size());

    for (const auto& [id, _] : plans_) {
        ids.push_back(id);
    }

    return ids;
}

size_t AlgoOrderManager::get_active_plan_count() const {
    return std::count_if(plans_.begin(), plans_.end(),
        [](const auto& pair) {
            return !pair.second->is_completed && !pair.second->is_cancelled;
        });
}

void AlgoOrderManager::cleanup_completed_plans() {
    auto it = plans_.begin();
    while (it != plans_.end()) {
        if (it->second->is_completed || it->second->is_cancelled) {
            it = plans_.erase(it);
        } else {
            ++it;
        }
    }
}

nlohmann::json AlgoOrderManager::get_manager_status() const {
    nlohmann::json status;
    status["total_plans"] = plans_.size();
    status["active_plans"] = get_active_plan_count();

    nlohmann::json plans_json = nlohmann::json::array();
    for (const auto& [id, plan] : plans_) {
        nlohmann::json plan_summary;
        plan_summary["id"] = id;
        plan_summary["code"] = plan->code;
        plan_summary["progress"] = plan->get_progress();
        plan_summary["status"] = plan->get_status_summary();
        plans_json.push_back(plan_summary);
    }
    status["plans"] = plans_json;

    return status;
}

nlohmann::json AlgoOrderManager::to_json() const {
    nlohmann::json j;
    nlohmann::json plans_json;

    for (const auto& [id, plan] : plans_) {
        plans_json[id] = plan->to_json();
    }
    j["plans"] = plans_json;

    return j;
}

std::unique_ptr<AlgoOrderManager> AlgoOrderManager::from_json(const nlohmann::json& j) {
    auto manager = std::make_unique<AlgoOrderManager>();

    if (j.contains("plans")) {
        for (const auto& [id, plan_json] : j["plans"].items()) {
            auto plan = std::make_unique<SplitOrderPlan>(
                SplitOrderPlan::from_json(plan_json));
            manager->plans_[id] = std::move(plan);
        }
    }

    return manager;
}

std::string AlgoOrderManager::generate_order_id() const {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

// Utility functions
namespace utils {

std::string algorithm_to_string(SplitAlgorithm algo) {
    switch (algo) {
        case SplitAlgorithm::TWAP: return "TWAP";
        case SplitAlgorithm::VWAP: return "VWAP";
        case SplitAlgorithm::Iceberg: return "Iceberg";
        case SplitAlgorithm::Custom: return "Custom";
        default: return "Unknown";
    }
}

SplitAlgorithm string_to_algorithm(const std::string& str) {
    if (str == "TWAP") return SplitAlgorithm::TWAP;
    if (str == "VWAP") return SplitAlgorithm::VWAP;
    if (str == "Iceberg") return SplitAlgorithm::Iceberg;
    if (str == "Custom") return SplitAlgorithm::Custom;
    return SplitAlgorithm::TWAP; // 默认值
}

std::string status_to_string(ChunkStatus status) {
    switch (status) {
        case ChunkStatus::Pending: return "Pending";
        case ChunkStatus::Sent: return "Sent";
        case ChunkStatus::PartiallyFilled: return "PartiallyFilled";
        case ChunkStatus::Filled: return "Filled";
        case ChunkStatus::Failed: return "Failed";
        case ChunkStatus::Cancelled: return "Cancelled";
        default: return "Unknown";
    }
}

ChunkStatus string_to_status(const std::string& str) {
    if (str == "Pending") return ChunkStatus::Pending;
    if (str == "Sent") return ChunkStatus::Sent;
    if (str == "PartiallyFilled") return ChunkStatus::PartiallyFilled;
    if (str == "Filled") return ChunkStatus::Filled;
    if (str == "Failed") return ChunkStatus::Failed;
    if (str == "Cancelled") return ChunkStatus::Cancelled;
    return ChunkStatus::Pending; // 默认值
}

SplitParams create_twap_params(size_t chunks, uint64_t interval) {
    SplitParams params;
    params.chunks = chunks;
    params.interval = interval;
    params.price_strategy = 0; // 固定价格
    params.random_factor = 0.0; // 无随机化
    return params;
}

SplitParams create_vwap_params(size_t chunks, uint64_t interval) {
    SplitParams params;
    params.chunks = chunks;
    params.interval = interval;
    params.price_strategy = 1; // 跟随市场价格
    params.random_factor = 0.1; // 少量随机化
    return params;
}

SplitParams create_iceberg_params(size_t chunks, double min_chunk_size, double random_factor) {
    SplitParams params;
    params.chunks = chunks;
    params.interval = 30; // 更频繁的执行
    params.min_chunk_size = min_chunk_size;
    params.random_factor = random_factor;
    params.price_strategy = 1; // 跟随市场价格
    return params;
}

ExecutionStats calculate_execution_stats(const SplitOrderPlan& plan) {
    ExecutionStats stats;

    double total_value = 0.0;
    std::vector<double> prices;

    for (const auto& chunk : plan.chunks) {
        if (chunk.status == ChunkStatus::Filled && chunk.executed_price) {
            stats.total_executed += chunk.amount;
            total_value += chunk.amount * (*chunk.executed_price);
            prices.push_back(*chunk.executed_price);
            stats.successful_chunks++;
        } else if (chunk.status == ChunkStatus::Failed) {
            stats.failed_chunks++;
        }
    }

    if (stats.total_executed > 0.0) {
        stats.avg_price = total_value / stats.total_executed;

        // 计算价格方差
        if (prices.size() > 1) {
            double variance = 0.0;
            for (double price : prices) {
                double diff = price - stats.avg_price;
                variance += diff * diff;
            }
            stats.price_variance = variance / (prices.size() - 1);
        }
    }

    // 计算执行时间（简化实现）
    if (plan.execution_start_time && plan.execution_end_time) {
        // 这里应该解析时间字符串并计算差值，简化为0
        stats.total_time = std::chrono::milliseconds(0);
    }

    return stats;
}

bool validate_split_params(const SplitParams& params, double total_amount) {
    if (params.chunks == 0) return false;
    if (params.min_chunk_size <= 0.0) return false;
    if (params.min_chunk_size * params.chunks > total_amount) return false;
    if (params.random_factor < 0.0 || params.random_factor > 1.0) return false;
    if (params.max_deviation < 0.0 || params.max_deviation > 1.0) return false;

    return true;
}

} // namespace utils

} // namespace qaultra::account::algo