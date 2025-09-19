#pragma once

#include "domain.hpp"
#include "order_queues.hpp"
#include <unordered_map>
#include <optional>
#include <vector>
#include <memory>
#include <chrono>
#include <string>
#include <set>
#include <map>
#include <nlohmann/json.hpp>

namespace qaultra::market::matchengine {

// 前向声明
template<typename Asset>
class OrderQueue;

/**
 * @brief 订单请求枚举 - 完全匹配Rust OrderRequest
 */
template<typename Asset>
class OrderRequest {
public:
    enum Type {
        NewMarketOrder,
        NewLimitOrder,
        AmendOrder,
        NewBestOrder,
        CancelOrder
    };

    Type type;
    Asset order_book_id;
    OrderDirection direction = OrderDirection::BUY;
    double price = 0.0;
    double volume = 0.0;
    int64_t ts = 0;
    uint64_t id = 0;

    /**
     * @brief 创建市价单请求
     */
    static OrderRequest new_market_order(const Asset& order_book_id,
                                       OrderDirection direction,
                                       double volume,
                                       int64_t ts) {
        OrderRequest req;
        req.type = NewMarketOrder;
        req.order_book_id = order_book_id;
        req.direction = direction;
        req.volume = volume;
        req.ts = ts;
        return req;
    }

    /**
     * @brief 创建限价单请求
     */
    static OrderRequest new_limit_order(const Asset& order_book_id,
                                      OrderDirection direction,
                                      double price,
                                      double volume,
                                      int64_t ts) {
        OrderRequest req;
        req.type = NewLimitOrder;
        req.order_book_id = order_book_id;
        req.direction = direction;
        req.price = price;
        req.volume = volume;
        req.ts = ts;
        return req;
    }

    /**
     * @brief 创建最优价单请求
     */
    static OrderRequest new_best_order(const Asset& order_book_id,
                                     OrderDirection direction,
                                     double volume,
                                     int64_t ts) {
        OrderRequest req;
        req.type = NewBestOrder;
        req.order_book_id = order_book_id;
        req.direction = direction;
        req.volume = volume;
        req.ts = ts;
        return req;
    }

    /**
     * @brief 创建修改订单请求
     */
    static OrderRequest amend_order(uint64_t id,
                                  OrderDirection direction,
                                  double price,
                                  double volume,
                                  int64_t ts) {
        OrderRequest req;
        req.type = AmendOrder;
        req.id = id;
        req.direction = direction;
        req.price = price;
        req.volume = volume;
        req.ts = ts;
        return req;
    }

    /**
     * @brief 创建取消订单请求
     */
    static OrderRequest cancel_order(uint64_t id, OrderDirection direction) {
        OrderRequest req;
        req.type = CancelOrder;
        req.id = id;
        req.direction = direction;
        return req;
    }
};

/**
 * @brief 价格包装类型，支持浮点数比较 - 完全匹配Rust Price
 */
class Price {
private:
    double value_;

public:
    Price(double value) : value_(value) {}

    double value() const { return value_; }

    bool operator==(const Price& other) const {
        return std::abs(value_ - other.value_) < std::numeric_limits<double>::epsilon();
    }

    bool operator<(const Price& other) const {
        return value_ < other.value_ && !(*this == other);
    }

    bool operator>(const Price& other) const {
        return value_ > other.value_ && !(*this == other);
    }

    bool operator<=(const Price& other) const {
        return *this < other || *this == other;
    }

    bool operator>=(const Price& other) const {
        return *this > other || *this == other;
    }
};

/**
 * @brief 订单簿类 - 完全匹配Rust Orderbook
 * @tparam Asset 资产类型
 */
template<typename Asset>
class Orderbook {
private:
    // 常量定义 - 匹配Rust
    static constexpr uint64_t MIN_SEQUENCE_ID = 1;
    static constexpr uint64_t MAX_SEQUENCE_ID = 10000000;  // 1千万
    static constexpr uint64_t MAX_STALLED_INDICES_IN_QUEUE = 1000;  // 1千
    static constexpr size_t ORDER_QUEUE_INIT_CAPACITY = 500000;  // 50万

    Asset order_book_id_;                           // 订单簿标识
    std::unique_ptr<OrderQueue<Asset>> bid_queue_;  // 买方队列
    std::unique_ptr<OrderQueue<Asset>> ask_queue_;  // 卖方队列
    uint64_t sequence_counter_;                     // 序列号生成器
    double lastprice_;                              // 最新成交价
    TradingState trading_state_;                    // 交易状态
    std::optional<double> auction_price_;           // 集合竞价价格
    std::optional<double> auction_volume_;          // 集合竞价成交量
    std::optional<double> theoretical_price_;       // 理论价格
    double prev_close_;                             // 前收盘价

public:
    /**
     * @brief 构造函数 - 匹配Rust new方法
     */
    Orderbook(const Asset& order_book_id, double prev_close = 0.0);

    /**
     * @brief 带集合竞价的构造函数 - 匹配Rust new_with_auction方法
     */
    static Orderbook new_with_auction(const Asset& order_book_id, double prev_close = 0.0);

    /**
     * @brief 禁止拷贝，允许移动
     */
    Orderbook(const Orderbook&) = delete;
    Orderbook& operator=(const Orderbook&) = delete;
    Orderbook(Orderbook&&) = default;
    Orderbook& operator=(Orderbook&&) = default;

    /**
     * @brief 设置交易状态
     */
    void start_pre_auction() { trading_state_ = TradingState::PreAuctionPeriod; }
    void start_auction_order() { trading_state_ = TradingState::AuctionOrder; }
    void start_auction_cancel() { trading_state_ = TradingState::AuctionCancel; }
    void start_auction_match() { trading_state_ = TradingState::AuctionMatch; }
    void start_continuous_trading() { trading_state_ = TradingState::ContinuousTrading; }
    void close_market() { trading_state_ = TradingState::Closed; }

    /**
     * @brief 计算集合竞价理论成交价格 - 匹配Rust calculate_theoretical_price方法
     *
     * 集合竞价价格需要同时满足三个条件：
     * 1. 成交量最大
     * 2. 高于基准价格的买入申报和低于基准价格的卖出申报全部满足（成交）
     * 3. 与基准价格相同的买卖双方中有一方申报全部满足（成交）
     *
     * 如果存在多个满足条件的价格：
     * - 上交所：选择所有候选价格的中间价
     * - 深交所：选择最接近前收盘价的价格
     */
    std::optional<double> calculate_theoretical_price();

    /**
     * @brief 执行集合竞价撮合 - 匹配Rust execute_auction方法
     */
    OrderProcessingResult execute_auction();

    /**
     * @brief 获取集合竞价状态信息 - 匹配Rust get_auction_status方法
     */
    AuctionStatus get_auction_status() const;

    /**
     * @brief 获取最优价格 - 匹配Rust get_best_price方法
     */
    double get_best_price(OrderDirection direction);

    /**
     * @brief 处理订单 - 匹配Rust process_order方法
     */
    OrderProcessingResult process_order(const OrderRequest<Asset>& order);

    /**
     * @brief 获取当前价差 - 匹配Rust current_spread方法
     */
    std::optional<std::pair<double, double>> current_spread();

    /**
     * @brief 获取一档行情 - 匹配Rust get_l1_tick方法
     */
    std::tuple<double, double, double, double, double> get_l1_tick();

    /**
     * @brief 显示完整深度 - 匹配Rust display_full_depth方法
     */
    void display_full_depth();

    /**
     * @brief 获取完整深度数据 - 匹配Rust get_full_depth方法
     */
    std::pair<std::optional<std::map<std::string, std::vector<double>>>,
              std::optional<std::map<std::string, std::vector<double>>>> get_full_depth();

    /**
     * @brief 打印订单簿状态 - 匹配Rust plot_orderbook方法
     */
    void plot_orderbook();

    /**
     * @brief 获取深度信息 - 匹配Rust get_depth方法
     */
    void get_depth();

    // Getters
    double get_last_price() const { return lastprice_; }
    TradingState get_trading_state() const { return trading_state_; }
    const Asset& get_order_book_id() const { return order_book_id_; }

private:
    /**
     * @brief 处理集合竞价期间的限价单
     */
    void handle_auction_limit_order(OrderProcessingResult& results, const OrderRequest<Asset>& order);

    /**
     * @brief 处理集合竞价期间的撤单
     */
    void handle_auction_cancel(OrderProcessingResult& results, const OrderRequest<Asset>& order);

    /**
     * @brief 处理连续交易期间的订单
     */
    void handle_continuous_trading(OrderProcessingResult& results, const OrderRequest<Asset>& order);

    /**
     * @brief 处理市价单 - 匹配Rust process_market_order方法
     */
    void process_market_order(OrderProcessingResult& results,
                             uint64_t order_id,
                             const Asset& order_book_id,
                             OrderDirection direction,
                             double volume);

    /**
     * @brief 处理限价单 - 匹配Rust process_limit_order方法
     */
    void process_limit_order(OrderProcessingResult& results,
                            uint64_t order_id,
                            const Asset& order_book_id,
                            OrderDirection direction,
                            double price,
                            double volume,
                            int64_t ts);

    /**
     * @brief 处理最优价单 - 匹配Rust process_best_order方法
     */
    void process_best_order(OrderProcessingResult& results,
                           const Asset& order_book_id,
                           OrderDirection direction,
                           double volume,
                           int64_t ts);

    /**
     * @brief 处理订单修改 - 匹配Rust process_order_amend方法
     */
    void process_order_amend(OrderProcessingResult& results,
                            uint64_t order_id,
                            OrderDirection direction,
                            double price,
                            double volume,
                            int64_t ts);

    /**
     * @brief 处理订单取消 - 匹配Rust process_order_cancel方法
     */
    void process_order_cancel(OrderProcessingResult& results,
                             uint64_t order_id,
                             OrderDirection direction);

    /**
     * @brief 存储新的限价单 - 匹配Rust store_new_limit_order方法
     */
    void store_new_limit_order(OrderProcessingResult& results,
                              uint64_t order_id,
                              const Asset& order_book_id,
                              OrderDirection direction,
                              double price,
                              double volume,
                              int64_t ts);

    /**
     * @brief 订单撮合 - 匹配Rust order_matching方法
     */
    bool order_matching(OrderProcessingResult& results,
                       const Order<Asset>& opposite_order,
                       uint64_t order_id,
                       const Asset& order_book_id,
                       OrderType order_type,
                       OrderDirection direction,
                       double volume);

    /**
     * @brief 生成下一个序列号
     */
    uint64_t next_sequence_id() {
        sequence_counter_++;
        if (sequence_counter_ > MAX_SEQUENCE_ID) {
            sequence_counter_ = MIN_SEQUENCE_ID;
        }
        return sequence_counter_;
    }

    /**
     * @brief 验证订单请求 - 简化版本
     */
    bool validate_order(const OrderRequest<Asset>& order) const;

    /**
     * @brief 获取当前时间戳(纳秒)
     */
    static int64_t get_current_timestamp_nanos() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

// 别名定义 - 匹配Rust
using OrderProcessingResult = std::vector<std::variant<Success, Failed>>;

} // namespace qaultra::market::matchengine