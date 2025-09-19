#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace qaultra::market::matchengine {

/**
 * @brief 订单方向枚举 - 完全匹配Rust OrderDirection
 */
enum class OrderDirection {
    BUY,    // 买入
    SELL    // 卖出
};

/**
 * @brief 订单类型枚举 - 完全匹配Rust OrderType
 */
enum class OrderType {
    Market, // 市价单
    Limit   // 限价单
};

/**
 * @brief 交易状态枚举 - 完全匹配Rust TradingState
 */
enum class TradingState {
    PreAuctionPeriod,       // 开盘集合竞价申报撤单期 (9:15-9:20)
    AuctionOrder,           // 开盘集合竞价申报期 (9:20-9:25)
    AuctionCancel,          // 开盘集合竞价撤单期 (9:25-9:30)
    AuctionMatch,           // 开盘集合竞价撮合时间点 (9:30)
    ContinuousTrading,      // 连续交易期
    Closed                  // 闭市
};

/**
 * @brief 订单特征接口 - 匹配Rust OrderTrait
 */
class OrderTrait {
public:
    virtual ~OrderTrait() = default;
    virtual uint64_t get_id() const = 0;
    virtual double get_volume() const = 0;
    virtual void set_volume(double volume) = 0;
    virtual double get_price() const = 0;
};

/**
 * @brief 通用订单类 - 完全匹配Rust Order<Asset>
 * @tparam Asset 资产类型，可以是字符串、枚举等
 */
template<typename Asset>
class Order : public OrderTrait {
public:
    uint64_t order_id;          // 订单ID
    Asset order_book_id;        // 订单簿ID(资产标识)
    OrderDirection direction;   // 方向
    double price;               // 价格
    double volume;              // 数量

public:
    /**
     * @brief 构造函数
     */
    Order(uint64_t id, const Asset& asset_id, OrderDirection dir, double p, double vol)
        : order_id(id), order_book_id(asset_id), direction(dir), price(p), volume(vol) {}

    /**
     * @brief 实现OrderTrait接口
     */
    uint64_t get_id() const override { return order_id; }
    double get_volume() const override { return volume; }
    void set_volume(double vol) override { volume = vol; }
    double get_price() const override { return price; }

    /**
     * @brief 获取方向
     */
    OrderDirection get_direction() const { return direction; }

    /**
     * @brief 获取资产ID
     */
    const Asset& get_asset_id() const { return order_book_id; }

    /**
     * @brief 是否为买单
     */
    bool is_buy() const { return direction == OrderDirection::BUY; }

    /**
     * @brief 是否为卖单
     */
    bool is_sell() const { return direction == OrderDirection::SELL; }

    /**
     * @brief 克隆订单
     */
    Order<Asset> clone() const {
        return Order<Asset>(order_id, order_book_id, direction, price, volume);
    }

    /**
     * @brief 序列化为JSON
     */
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["order_id"] = order_id;
        j["order_book_id"] = order_book_id;
        j["direction"] = static_cast<int>(direction);
        j["price"] = price;
        j["volume"] = volume;
        return j;
    }

    /**
     * @brief 从JSON反序列化
     */
    static Order<Asset> from_json(const nlohmann::json& j) {
        return Order<Asset>(
            j.at("order_id").get<uint64_t>(),
            j.at("order_book_id").get<Asset>(),
            static_cast<OrderDirection>(j.at("direction").get<int>()),
            j.at("price").get<double>(),
            j.at("volume").get<double>()
        );
    }

    /**
     * @brief 比较操作符
     */
    bool operator==(const Order<Asset>& other) const {
        return order_id == other.order_id &&
               order_book_id == other.order_book_id &&
               direction == other.direction &&
               price == other.price &&
               volume == other.volume;
    }

    bool operator!=(const Order<Asset>& other) const {
        return !(*this == other);
    }
};

/**
 * @brief 集合竞价状态信息 - 完全匹配Rust AuctionStatus
 */
struct AuctionStatus {
    TradingState trading_state = TradingState::Closed;  // 交易状态
    std::optional<double> auction_price;                // 集合竞价价格
    std::optional<double> theoretical_price;            // 理论价格
    std::optional<double> auction_volume;               // 集合竞价成交量

    /**
     * @brief 序列化为JSON
     */
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["trading_state"] = static_cast<int>(trading_state);
        j["auction_price"] = auction_price.value_or(0.0);
        j["theoretical_price"] = theoretical_price.value_or(0.0);
        j["auction_volume"] = auction_volume.value_or(0.0);
        return j;
    }

    /**
     * @brief 从JSON反序列化
     */
    static AuctionStatus from_json(const nlohmann::json& j) {
        AuctionStatus status;
        status.trading_state = static_cast<TradingState>(j.value("trading_state", 0));

        double ap = j.value("auction_price", 0.0);
        if (ap > 0.0) status.auction_price = ap;

        double tp = j.value("theoretical_price", 0.0);
        if (tp > 0.0) status.theoretical_price = tp;

        double av = j.value("auction_volume", 0.0);
        if (av > 0.0) status.auction_volume = av;

        return status;
    }
};

/**
 * @brief 成功结果枚举 - 完全匹配Rust Success
 */
struct Success {
    enum Type {
        Accepted,           // 订单已接受
        Filled,             // 订单已成交
        PartiallyFilled,    // 订单部分成交
        Amended,            // 订单已修改
        Cancelled           // 订单已取消
    };

    Type type;
    uint64_t id = 0;
    uint64_t order_id = 0;
    uint64_t opposite_order_id = 0;
    OrderDirection direction = OrderDirection::BUY;
    OrderType order_type = OrderType::Limit;
    double price = 0.0;
    double volume = 0.0;
    int64_t ts = 0;  // 时间戳

    /**
     * @brief 创建Accepted结果
     */
    static Success accepted(uint64_t id, OrderType order_type, int64_t ts) {
        Success s;
        s.type = Accepted;
        s.id = id;
        s.order_type = order_type;
        s.ts = ts;
        return s;
    }

    /**
     * @brief 创建Filled结果
     */
    static Success filled(uint64_t order_id, OrderDirection direction, OrderType order_type,
                         double price, double volume, int64_t ts, uint64_t opposite_order_id) {
        Success s;
        s.type = Filled;
        s.order_id = order_id;
        s.direction = direction;
        s.order_type = order_type;
        s.price = price;
        s.volume = volume;
        s.ts = ts;
        s.opposite_order_id = opposite_order_id;
        return s;
    }

    /**
     * @brief 创建PartiallyFilled结果
     */
    static Success partially_filled(uint64_t order_id, OrderDirection direction, OrderType order_type,
                                   double price, double volume, int64_t ts, uint64_t opposite_order_id) {
        Success s;
        s.type = PartiallyFilled;
        s.order_id = order_id;
        s.direction = direction;
        s.order_type = order_type;
        s.price = price;
        s.volume = volume;
        s.ts = ts;
        s.opposite_order_id = opposite_order_id;
        return s;
    }

    /**
     * @brief 创建Amended结果
     */
    static Success amended(uint64_t id, double price, double volume, int64_t ts) {
        Success s;
        s.type = Amended;
        s.id = id;
        s.price = price;
        s.volume = volume;
        s.ts = ts;
        return s;
    }

    /**
     * @brief 创建Cancelled结果
     */
    static Success cancelled(uint64_t id, int64_t ts) {
        Success s;
        s.type = Cancelled;
        s.id = id;
        s.ts = ts;
        return s;
    }

    nlohmann::json to_json() const;
    static Success from_json(const nlohmann::json& j);
};

/**
 * @brief 失败结果枚举 - 完全匹配Rust Failed
 */
struct Failed {
    enum Type {
        ValidationFailed,   // 验证失败
        DuplicateOrderID,   // 重复订单ID
        NoMatch,           // 无匹配
        OrderNotFound      // 订单未找到
    };

    Type type;
    uint64_t order_id = 0;
    std::string message;

    /**
     * @brief 创建ValidationFailed结果
     */
    static Failed validation_failed(const std::string& message) {
        Failed f;
        f.type = ValidationFailed;
        f.message = message;
        return f;
    }

    /**
     * @brief 创建DuplicateOrderID结果
     */
    static Failed duplicate_order_id(uint64_t order_id) {
        Failed f;
        f.type = DuplicateOrderID;
        f.order_id = order_id;
        return f;
    }

    /**
     * @brief 创建NoMatch结果
     */
    static Failed no_match(uint64_t order_id) {
        Failed f;
        f.type = NoMatch;
        f.order_id = order_id;
        return f;
    }

    /**
     * @brief 创建OrderNotFound结果
     */
    static Failed order_not_found(uint64_t order_id) {
        Failed f;
        f.type = OrderNotFound;
        f.order_id = order_id;
        return f;
    }

    nlohmann::json to_json() const;
    static Failed from_json(const nlohmann::json& j);
};

// 类型别名
using OrderProcessingResult = std::vector<std::variant<Success, Failed>>;

/**
 * @brief 工具函数命名空间
 */
namespace utils {
    /**
     * @brief 将OrderDirection转换为字符串
     */
    std::string direction_to_string(OrderDirection direction);

    /**
     * @brief 将字符串转换为OrderDirection
     */
    OrderDirection string_to_direction(const std::string& str);

    /**
     * @brief 将OrderType转换为字符串
     */
    std::string order_type_to_string(OrderType order_type);

    /**
     * @brief 将字符串转换为OrderType
     */
    OrderType string_to_order_type(const std::string& str);

    /**
     * @brief 将TradingState转换为字符串
     */
    std::string trading_state_to_string(TradingState state);

    /**
     * @brief 将字符串转换为TradingState
     */
    TradingState string_to_trading_state(const std::string& str);

    /**
     * @brief 获取当前时间戳(纳秒)
     */
    int64_t get_timestamp_nanos();
}

} // namespace qaultra::market::matchengine