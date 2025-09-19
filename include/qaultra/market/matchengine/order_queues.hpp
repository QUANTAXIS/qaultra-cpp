#pragma once

#include "domain.hpp"
#include <queue>
#include <unordered_map>
#include <map>
#include <vector>
#include <optional>
#include <memory>
#include <functional>

namespace qaultra::market::matchengine {

/**
 * @brief 订单索引结构 - 完全匹配Rust OrderIndex
 */
struct OrderIndex {
    uint64_t id;                    // 订单ID
    double price;                   // 价格
    int64_t timestamp;              // 时间戳
    OrderDirection order_side;      // 订单方向
    double volume;                  // 数量

    /**
     * @brief 构造函数
     */
    OrderIndex(uint64_t id, double price, int64_t timestamp,
               OrderDirection order_side, double volume)
        : id(id), price(price), timestamp(timestamp),
          order_side(order_side), volume(volume) {}

    /**
     * @brief 拷贝构造函数
     */
    OrderIndex(const OrderIndex& other) = default;
    OrderIndex& operator=(const OrderIndex& other) = default;

    /**
     * @brief 比较操作符 - 实现价格-时间优先级
     * 买单: 价格高优先, 同价格时间早优先
     * 卖单: 价格低优先, 同价格时间早优先
     */
    bool operator<(const OrderIndex& other) const {
        if (std::abs(price - other.price) > std::numeric_limits<double>::epsilon()) {
            switch (order_side) {
                case OrderDirection::BUY:
                    return price < other.price;  // 买单价格高优先，但priority_queue是大根堆，所以反向
                case OrderDirection::SELL:
                    return price > other.price;  // 卖单价格低优先，但priority_queue是大根堆，所以反向
            }
        }
        // 同价格按时间优先(FIFO) - 时间早的优先
        return timestamp > other.timestamp;  // priority_queue是大根堆，所以反向
    }

    bool operator==(const OrderIndex& other) const {
        return std::abs(price - other.price) < std::numeric_limits<double>::epsilon()
               && timestamp == other.timestamp;
    }

    bool operator!=(const OrderIndex& other) const {
        return !(*this == other);
    }
};

/**
 * @brief 订单队列类 - 完全匹配Rust OrderQueue
 * @tparam T 订单类型，必须实现OrderTrait接口
 */
template<typename T>
class OrderQueue {
private:
    std::priority_queue<OrderIndex> idx_queue_;     // 订单索引优先队列
    std::unordered_map<uint64_t, T> orders_;        // 订单存储
    uint64_t op_counter_;                           // 操作计数器
    uint64_t max_stalled_;                          // 最大游离索引数量
    OrderDirection queue_side_;                     // 队列方向

public:
    /**
     * @brief 构造函数 - 匹配Rust new方法
     */
    OrderQueue(OrderDirection direction, uint64_t max_stalled, size_t capacity)
        : op_counter_(0), max_stalled_(max_stalled), queue_side_(direction) {
        orders_.reserve(capacity);
    }

    /**
     * @brief 获取排序的订单列表 - 匹配Rust get_sorted_orders方法
     */
    std::optional<std::vector<OrderIndex>> get_sorted_orders() const {
        if (idx_queue_.empty()) {
            return std::nullopt;
        }

        // 复制队列内容到vector并排序
        std::priority_queue<OrderIndex> temp_queue = idx_queue_;
        std::vector<OrderIndex> orders;
        orders.reserve(temp_queue.size());

        while (!temp_queue.empty()) {
            orders.push_back(temp_queue.top());
            temp_queue.pop();
        }

        // 按照正确的顺序排序
        std::sort(orders.begin(), orders.end(), [this](const OrderIndex& a, const OrderIndex& b) {
            if (std::abs(a.price - b.price) > std::numeric_limits<double>::epsilon()) {
                switch (queue_side_) {
                    case OrderDirection::BUY:
                        // 买单：价格降序，同价格按时间升序
                        return a.price > b.price;
                    case OrderDirection::SELL:
                        // 卖单：价格升序，同价格按时间升序
                        return a.price < b.price;
                }
            }
            // 同价格按时间升序(FIFO)
            return a.timestamp < b.timestamp;
        });

        // 过滤已取消的订单
        orders.erase(std::remove_if(orders.begin(), orders.end(),
            [this](const OrderIndex& order) {
                return orders_.find(order.id) == orders_.end();
            }), orders.end());

        return orders.empty() ? std::nullopt : std::make_optional(orders);
    }

    /**
     * @brief 修改订单数量 - 匹配Rust modify_order_volume方法
     */
    bool modify_order_volume(uint64_t id, double new_volume) {
        // 更新底层订单
        auto it = orders_.find(id);
        if (it == orders_.end()) {
            return false;
        }
        it->second.set_volume(new_volume);

        // 更新索引队列 - 需要重建队列
        rebuild_idx_queue_for_order(id, new_volume);
        return true;
    }

    /**
     * @brief 查看队首订单 - 匹配Rust peek方法
     */
    const T* peek() {
        while (!idx_queue_.empty()) {
            uint64_t order_id = idx_queue_.top().id;
            auto it = orders_.find(order_id);
            if (it != orders_.end()) {
                return &it->second;
            } else {
                // 移除游离的索引
                idx_queue_.pop();
            }
        }
        return nullptr;
    }

    /**
     * @brief 弹出队首订单 - 匹配Rust pop方法
     */
    std::optional<T> pop() {
        while (!idx_queue_.empty()) {
            uint64_t order_id = idx_queue_.top().id;
            idx_queue_.pop();

            auto it = orders_.find(order_id);
            if (it != orders_.end()) {
                T order = std::move(it->second);
                orders_.erase(it);
                return order;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief 插入新订单 - 匹配Rust insert方法
     */
    bool insert(uint64_t id, double price, int64_t ts, double volume, T&& order) {
        if (orders_.find(id) != orders_.end()) {
            return false;  // 订单ID已存在
        }

        // 存储订单
        orders_[id] = std::move(order);

        // 添加索引
        idx_queue_.emplace(id, price, ts, queue_side_, volume);
        return true;
    }

    /**
     * @brief 修改订单 - 匹配Rust amend方法
     */
    bool amend(uint64_t id, double price, int64_t ts, double volume, T&& order) {
        if (orders_.find(id) == orders_.end()) {
            return false;
        }

        // 更新订单数据
        orders_[id] = std::move(order);

        // 重建索引
        rebuild_idx(id, price, ts, volume);
        return true;
    }

    /**
     * @brief 取消订单 - 匹配Rust cancel方法
     */
    bool cancel(uint64_t id) {
        auto it = orders_.find(id);
        if (it != orders_.end()) {
            orders_.erase(it);
            clean_check();
            return true;
        }
        return false;
    }

    /**
     * @brief 修改当前订单 - 匹配Rust modify_current_order方法
     * 注意: 不修改价格或时间，因为索引不变
     */
    bool modify_current_order(T&& new_order, double volume_delta) {
        if (idx_queue_.empty()) {
            return false;
        }

        uint64_t order_id = idx_queue_.top().id;
        auto it = orders_.find(order_id);
        if (it != orders_.end()) {
            orders_[order_id] = std::move(new_order);

            // 修改索引中的数量
            // 由于priority_queue不允许修改，我们需要访问内部结构
            // 这里使用const_cast是安全的，因为我们只修改volume不影响排序
            const_cast<OrderIndex&>(idx_queue_.top()).volume -= volume_delta;
            return true;
        }
        return false;
    }

    /**
     * @brief 移除指定订单 - 匹配Rust remove_order方法
     */
    bool remove_order(uint64_t id) {
        auto it = orders_.find(id);
        if (it != orders_.end()) {
            orders_.erase(it);
            clean_check();  // 清理游离索引
            return true;
        }
        return false;
    }

    /**
     * @brief 获取深度数据 - 匹配Rust get_depth方法
     */
    std::optional<std::map<std::string, std::vector<double>>> get_depth() {
        if (idx_queue_.empty()) {
            return std::nullopt;
        }

        std::map<std::string, std::vector<double>> depth_map;
        std::map<double, double> price_volume_map;

        // 复制队列并收集数据
        std::priority_queue<OrderIndex> temp_queue = idx_queue_;
        while (!temp_queue.empty()) {
            const OrderIndex& order_idx = temp_queue.top();
            if (orders_.find(order_idx.id) != orders_.end()) {
                price_volume_map[order_idx.price] += order_idx.volume;
            }
            temp_queue.pop();
        }

        std::vector<double> prices, volumes;
        for (const auto& [price, volume] : price_volume_map) {
            prices.push_back(price);
            volumes.push_back(volume);
        }

        // 卖单需要反转顺序 (价格从低到高)
        if (queue_side_ == OrderDirection::SELL) {
            std::reverse(prices.begin(), prices.end());
            std::reverse(volumes.begin(), volumes.end());
        }

        depth_map["prices"] = std::move(prices);
        depth_map["volumes"] = std::move(volumes);
        return depth_map;
    }

    /**
     * @brief 检查队列是否为空
     */
    bool empty() const {
        return orders_.empty();
    }

    /**
     * @brief 获取队列大小
     */
    size_t size() const {
        return orders_.size();
    }

private:
    /**
     * @brief 清理检查 - 匹配Rust clean_check方法
     */
    void clean_check() {
        remove_stalled();
        if (op_counter_ > max_stalled_) {
            op_counter_ = 0;
            remove_stalled();
        } else {
            op_counter_++;
        }
    }

    /**
     * @brief 移除游离的索引 - 匹配Rust remove_stalled方法
     */
    void remove_stalled() {
        std::priority_queue<OrderIndex> new_queue;

        while (!idx_queue_.empty()) {
            const OrderIndex& idx = idx_queue_.top();
            if (orders_.find(idx.id) != orders_.end()) {
                new_queue.push(idx);
            }
            idx_queue_.pop();
        }

        idx_queue_ = std::move(new_queue);
    }

    /**
     * @brief 重建索引 - 匹配Rust rebuild_idx方法
     */
    void rebuild_idx(uint64_t id, double price, int64_t ts, double volume) {
        std::priority_queue<OrderIndex> new_queue;

        // 重建队列，排除指定订单
        while (!idx_queue_.empty()) {
            const OrderIndex& idx = idx_queue_.top();
            if (idx.id != id) {
                new_queue.push(idx);
            }
            idx_queue_.pop();
        }

        // 添加新的索引
        new_queue.emplace(id, price, ts, queue_side_, volume);
        idx_queue_ = std::move(new_queue);
    }

    /**
     * @brief 为特定订单重建索引队列 - 仅更新数量
     */
    void rebuild_idx_queue_for_order(uint64_t id, double new_volume) {
        std::priority_queue<OrderIndex> new_queue;

        while (!idx_queue_.empty()) {
            OrderIndex idx = idx_queue_.top();
            idx_queue_.pop();

            if (idx.id == id) {
                idx.volume = new_volume;
            }
            new_queue.push(idx);
        }

        idx_queue_ = std::move(new_queue);
    }
};

} // namespace qaultra::market::matchengine