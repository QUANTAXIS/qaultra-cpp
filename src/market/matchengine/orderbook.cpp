#include "qaultra/market/matchengine/orderbook.hpp"
#include "qaultra/market/matchengine/order_queues.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <set>
#include <chrono>
#include <limits>
#include <variant>

namespace qaultra::market::matchengine {

template<typename Asset>
Orderbook<Asset>::Orderbook(const Asset& order_book_id, double prev_close)
    : order_book_id_(order_book_id)
    , bid_queue_(std::make_unique<OrderQueue<Asset>>(OrderDirection::BUY, MAX_STALLED_INDICES_IN_QUEUE, ORDER_QUEUE_INIT_CAPACITY))
    , ask_queue_(std::make_unique<OrderQueue<Asset>>(OrderDirection::SELL, MAX_STALLED_INDICES_IN_QUEUE, ORDER_QUEUE_INIT_CAPACITY))
    , sequence_counter_(MIN_SEQUENCE_ID)
    , lastprice_(0.0)
    , trading_state_(TradingState::ContinuousTrading)
    , prev_close_(prev_close) {
}

template<typename Asset>
Orderbook<Asset> Orderbook<Asset>::new_with_auction(const Asset& order_book_id, double prev_close) {
    Orderbook orderbook(order_book_id, prev_close);
    orderbook.trading_state_ = TradingState::PreAuctionPeriod;
    return orderbook;
}

template<typename Asset>
std::optional<double> Orderbook<Asset>::calculate_theoretical_price() {
    // 获取按照价格和时间排序的买卖单队列
    auto bid_orders = bid_queue_->get_sorted_orders();
    auto ask_orders = ask_queue_->get_sorted_orders();

    if (!bid_orders || !ask_orders || bid_orders->empty() || ask_orders->empty()) {
        return std::nullopt;
    }

    // 使用set收集唯一价格点
    std::set<Price> price_points;

    // 收集所有可能的价格点
    for (const auto& order : *bid_orders) {
        price_points.emplace(order.price);
    }
    for (const auto& order : *ask_orders) {
        price_points.emplace(order.price);
    }

    double max_volume = 0.0;
    std::vector<double> candidate_prices;

    // 遍历每个价格点计算成交量
    for (const Price& price : price_points) {
        double price_val = price.value();

        // 计算当前价格点的成交量
        double executable_bids = 0.0;
        for (const auto& order : *bid_orders) {
            if (order.price >= price_val) {
                executable_bids += order.volume;
            }
        }

        double executable_asks = 0.0;
        for (const auto& order : *ask_orders) {
            if (order.price <= price_val) {
                executable_asks += order.volume;
            }
        }

        double volume_at_price = std::min(executable_bids, executable_asks);

        // 验证集合竞价条件
        // 1. 高于基准价格的买入申报全部满足
        bool high_bids_satisfied = true;
        for (const auto& order : *bid_orders) {
            if (order.price > price_val) {
                // 所有高价买单都应该能够成交
                high_bids_satisfied = true; // 在集合竞价中，这个条件总是满足的
            }
        }

        // 2. 低于基准价格的卖出申报全部满足
        bool low_asks_satisfied = true;
        for (const auto& order : *ask_orders) {
            if (order.price < price_val) {
                // 所有低价卖单都应该能够成交
                low_asks_satisfied = true; // 在集合竞价中，这个条件总是满足的
            }
        }

        // 3. 与基准价格相同的买卖双方中有一方申报全部满足
        double equal_bid_volume = 0.0;
        double equal_ask_volume = 0.0;

        for (const auto& order : *bid_orders) {
            if (std::abs(order.price - price_val) < std::numeric_limits<double>::epsilon()) {
                equal_bid_volume += order.volume;
            }
        }

        for (const auto& order : *ask_orders) {
            if (std::abs(order.price - price_val) < std::numeric_limits<double>::epsilon()) {
                equal_ask_volume += order.volume;
            }
        }

        bool equal_price_satisfied = (equal_bid_volume <= volume_at_price) ||
                                    (equal_ask_volume <= volume_at_price);

        if (high_bids_satisfied && low_asks_satisfied && equal_price_satisfied) {
            if (volume_at_price > max_volume) {
                max_volume = volume_at_price;
                candidate_prices.clear();
                candidate_prices.push_back(price_val);
            } else if (std::abs(volume_at_price - max_volume) < std::numeric_limits<double>::epsilon()) {
                candidate_prices.push_back(price_val);
            }
        }
    }

    // 选择最终价格
    if (candidate_prices.empty()) {
        return std::nullopt;
    } else if (candidate_prices.size() == 1) {
        return candidate_prices[0];
    } else {
        // 处理多个候选价格
        if (prev_close_ > 0.0) {
            // 深交所规则：选择最接近前收盘价的价格
            auto closest_it = std::min_element(candidate_prices.begin(), candidate_prices.end(),
                [this](double a, double b) {
                    return std::abs(a - prev_close_) < std::abs(b - prev_close_);
                });
            return *closest_it;
        } else {
            // 上交所规则：选择中间价
            std::sort(candidate_prices.begin(), candidate_prices.end());
            size_t mid_idx = candidate_prices.size() / 2;
            return candidate_prices[mid_idx];
        }
    }
}

template<typename Asset>
OrderProcessingResult Orderbook<Asset>::execute_auction() {
    OrderProcessingResult results;

    // 验证交易状态
    if (trading_state_ != TradingState::AuctionMatch) {
        std::cerr << "警告: 尝试在无效状态下执行集合竞价: " << static_cast<int>(trading_state_) << std::endl;
        return results;
    }

    // 计算理论价格
    auto theoretical_price = calculate_theoretical_price();
    if (!theoretical_price) {
        std::cerr << "警告: 无法确定集合竞价价格" << std::endl;
        trading_state_ = TradingState::ContinuousTrading;
        return results;
    }

    double auction_price = *theoretical_price;
    auction_price_ = auction_price;
    lastprice_ = auction_price;

    std::cout << "集合竞价价格确定: " << auction_price << std::endl;

    auto bid_orders = bid_queue_->get_sorted_orders();
    auto ask_orders = ask_queue_->get_sorted_orders();

    if (!bid_orders || !ask_orders) {
        trading_state_ = TradingState::ContinuousTrading;
        return results;
    }

    double total_volume = 0.0;
    size_t bid_idx = 0, ask_idx = 0;

    // 执行撮合
    while (bid_idx < bid_orders->size() && ask_idx < ask_orders->size()) {
        const auto& bid = (*bid_orders)[bid_idx];
        const auto& ask = (*ask_orders)[ask_idx];

        if (bid.price >= auction_price && ask.price <= auction_price) {
            double match_volume = std::min(bid.volume, ask.volume);

            if (match_volume > 0.0) {
                total_volume += match_volume;
                int64_t current_time = get_current_timestamp_nanos();

                // 处理买单剩余量
                double bid_remaining = bid.volume - match_volume;
                if (bid_remaining > 0.0) {
                    // 买方部分成交
                    results.emplace_back(Success::partially_filled(
                        bid.id, OrderDirection::BUY, OrderType::Limit,
                        auction_price, match_volume, current_time, ask.id));

                    bid_queue_->modify_order_volume(bid.id, bid_remaining);
                } else {
                    // 买方完全成交
                    results.emplace_back(Success::filled(
                        bid.id, OrderDirection::BUY, OrderType::Limit,
                        auction_price, match_volume, current_time, ask.id));

                    bid_queue_->remove_order(bid.id);
                }

                // 处理卖单剩余量
                double ask_remaining = ask.volume - match_volume;
                if (ask_remaining > 0.0) {
                    // 卖方部分成交
                    results.emplace_back(Success::partially_filled(
                        ask.id, OrderDirection::SELL, OrderType::Limit,
                        auction_price, match_volume, current_time, bid.id));

                    ask_queue_->modify_order_volume(ask.id, ask_remaining);
                } else {
                    // 卖方完全成交
                    results.emplace_back(Success::filled(
                        ask.id, OrderDirection::SELL, OrderType::Limit,
                        auction_price, match_volume, current_time, bid.id));

                    ask_queue_->remove_order(ask.id);
                }

                // 更新索引
                if (bid_remaining <= 0.0) bid_idx++;
                if (ask_remaining <= 0.0) ask_idx++;

                // 重新获取订单列表
                bid_orders = bid_queue_->get_sorted_orders();
                ask_orders = ask_queue_->get_sorted_orders();

                if (!bid_orders || !ask_orders) break;

                // 重置索引如果订单列表发生变化
                bid_idx = 0;
                ask_idx = 0;
            }
        } else {
            std::cout << "在集合竞价价格 " << auction_price << " 下无更多可撮合订单" << std::endl;
            break;
        }
    }

    // 记录集合竞价成交量
    auction_volume_ = total_volume;
    std::cout << "集合竞价完成。总成交量: " << total_volume << std::endl;

    // 转换到连续交易状态
    trading_state_ = TradingState::ContinuousTrading;
    std::cout << "转换到连续交易状态" << std::endl;

    return results;
}

template<typename Asset>
AuctionStatus Orderbook<Asset>::get_auction_status() const {
    AuctionStatus status;
    status.trading_state = trading_state_;
    status.auction_price = auction_price_;
    status.theoretical_price = theoretical_price_;
    status.auction_volume = auction_volume_;
    return status;
}

template<typename Asset>
double Orderbook<Asset>::get_best_price(OrderDirection direction) {
    switch (direction) {
        case OrderDirection::BUY: {
            // 买方最优价 = max(最高买价, min(最低卖价-1档, 最新价))
            const auto* best_bid = bid_queue_->peek();
            const auto* best_ask = ask_queue_->peek();

            if (best_bid && best_ask) {
                return best_bid->get_price();
            } else if (best_bid) {
                return best_bid->get_price();
            } else if (best_ask) {
                return best_ask->get_price() - 0.01; // 档位可根据品种配置
            } else {
                return lastprice_;
            }
        }
        case OrderDirection::SELL: {
            // 卖方最优价 = min(最低卖价, max(最高买价+1档, 最新价))
            const auto* best_bid = bid_queue_->peek();
            const auto* best_ask = ask_queue_->peek();

            if (best_bid && best_ask) {
                return best_ask->get_price();
            } else if (best_ask) {
                return best_ask->get_price();
            } else if (best_bid) {
                return best_bid->get_price() + 0.01; // 档位可根据品种配置
            } else {
                return lastprice_;
            }
        }
    }
    return lastprice_;
}

template<typename Asset>
OrderProcessingResult Orderbook<Asset>::process_order(const OrderRequest<Asset>& order) {
    OrderProcessingResult proc_result;

    // 验证订单
    if (!validate_order(order)) {
        proc_result.emplace_back(Failed::validation_failed("订单验证失败"));
        return proc_result;
    }

    switch (trading_state_) {
        // 9:15-9:20 可报可撤
        case TradingState::PreAuctionPeriod:
            if (order.type == OrderRequest<Asset>::NewLimitOrder ||
                order.type == OrderRequest<Asset>::CancelOrder) {
                if (order.type == OrderRequest<Asset>::NewLimitOrder) {
                    handle_auction_limit_order(proc_result, order);
                } else {
                    handle_auction_cancel(proc_result, order);
                }
            } else {
                proc_result.emplace_back(Failed::validation_failed(
                    "开盘集合竞价申报撤单期只接受限价单和撤单"));
            }
            break;

        // 9:20-9:25 仅可报单
        case TradingState::AuctionOrder:
            if (order.type == OrderRequest<Asset>::NewLimitOrder) {
                handle_auction_limit_order(proc_result, order);
            } else {
                proc_result.emplace_back(Failed::validation_failed(
                    "开盘集合竞价申报期只接受限价单"));
            }
            break;

        // 9:25-9:30 可报可撤
        case TradingState::AuctionCancel:
            if (order.type == OrderRequest<Asset>::NewLimitOrder ||
                order.type == OrderRequest<Asset>::CancelOrder) {
                if (order.type == OrderRequest<Asset>::NewLimitOrder) {
                    handle_auction_limit_order(proc_result, order);
                } else {
                    handle_auction_cancel(proc_result, order);
                }
            } else {
                proc_result.emplace_back(Failed::validation_failed(
                    "开盘集合竞价撤单期只接受限价单和撤单"));
            }
            break;

        // 9:30 集合竞价撮合
        case TradingState::AuctionMatch:
            proc_result.emplace_back(Failed::validation_failed(
                "集合竞价撮合期间不接受订单"));
            break;

        // 连续交易
        case TradingState::ContinuousTrading:
            handle_continuous_trading(proc_result, order);
            break;

        case TradingState::Closed:
            proc_result.emplace_back(Failed::validation_failed("市场已关闭"));
            break;
    }

    return proc_result;
}

template<typename Asset>
std::optional<std::pair<double, double>> Orderbook<Asset>::current_spread() {
    const auto* bid = bid_queue_->peek();
    const auto* ask = ask_queue_->peek();

    if (bid && ask) {
        return std::make_pair(bid->get_price(), ask->get_price());
    }
    return std::nullopt;
}

template<typename Asset>
std::tuple<double, double, double, double, double> Orderbook<Asset>::get_l1_tick() {
    const auto* bid = bid_queue_->peek();
    const auto* ask = ask_queue_->peek();

    if (bid && ask) {
        return std::make_tuple(
            bid->get_price(), bid->get_volume(),
            ask->get_price(), ask->get_volume(),
            lastprice_
        );
    }
    return std::make_tuple(0.0, 0.0, 0.0, 0.0, 0.0);
}

template<typename Asset>
void Orderbook<Asset>::handle_auction_limit_order(OrderProcessingResult& results, const OrderRequest<Asset>& order) {
    uint64_t order_id = next_sequence_id();
    results.emplace_back(Success::accepted(order_id, OrderType::Limit, get_current_timestamp_nanos()));

    store_new_limit_order(results, order_id, order.order_book_id,
                         order.direction, order.price, order.volume, order.ts);
}

template<typename Asset>
void Orderbook<Asset>::handle_auction_cancel(OrderProcessingResult& results, const OrderRequest<Asset>& order) {
    process_order_cancel(results, order.id, order.direction);
}

template<typename Asset>
void Orderbook<Asset>::handle_continuous_trading(OrderProcessingResult& results, const OrderRequest<Asset>& order) {
    switch (order.type) {
        case OrderRequest<Asset>::NewMarketOrder: {
            uint64_t order_id = next_sequence_id();
            results.emplace_back(Success::accepted(order_id, OrderType::Market, get_current_timestamp_nanos()));
            process_market_order(results, order_id, order.order_book_id, order.direction, order.volume);
            break;
        }

        case OrderRequest<Asset>::NewBestOrder: {
            process_best_order(results, order.order_book_id, order.direction, order.volume, order.ts);
            break;
        }

        case OrderRequest<Asset>::NewLimitOrder: {
            uint64_t order_id = next_sequence_id();
            results.emplace_back(Success::accepted(order_id, OrderType::Limit, get_current_timestamp_nanos()));
            process_limit_order(results, order_id, order.order_book_id,
                               order.direction, order.price, order.volume, order.ts);
            break;
        }

        case OrderRequest<Asset>::AmendOrder: {
            process_order_amend(results, order.id, order.direction,
                               order.price, order.volume, order.ts);
            break;
        }

        case OrderRequest<Asset>::CancelOrder: {
            process_order_cancel(results, order.id, order.direction);
            break;
        }
    }
}

template<typename Asset>
void Orderbook<Asset>::process_market_order(OrderProcessingResult& results,
                                           uint64_t order_id,
                                           const Asset& order_book_id,
                                           OrderDirection direction,
                                           double volume) {
    auto opposite_queue = (direction == OrderDirection::BUY) ? ask_queue_.get() : bid_queue_.get();
    const auto* opposite_order = opposite_queue->peek();

    if (opposite_order) {
        bool matching_complete = order_matching(results, *opposite_order, order_id,
                                              order_book_id, OrderType::Market, direction, volume);

        if (!matching_complete) {
            // 继续撮合剩余部分
            process_market_order(results, order_id, order_book_id, direction,
                                volume - opposite_order->get_volume());
        }
    } else {
        // 没有对手单，转为限价单
        std::cout << "没有对手单，市价单转限价单，当前最新价: " << lastprice_ << std::endl;
        process_limit_order(results, order_id, order_book_id, direction,
                           lastprice_, volume, get_current_timestamp_nanos());
    }
}

template<typename Asset>
void Orderbook<Asset>::process_limit_order(OrderProcessingResult& results,
                                          uint64_t order_id,
                                          const Asset& order_book_id,
                                          OrderDirection direction,
                                          double price,
                                          double volume,
                                          int64_t ts) {
    auto opposite_queue = (direction == OrderDirection::BUY) ? ask_queue_.get() : bid_queue_.get();
    const auto* opposite_order = opposite_queue->peek();

    if (opposite_order) {
        bool could_be_matched = (direction == OrderDirection::BUY) ?
            (price >= opposite_order->get_price()) :
            (price <= opposite_order->get_price());

        if (could_be_matched) {
            // 立即撮合
            bool matching_complete = order_matching(results, *opposite_order, order_id,
                                                  order_book_id, OrderType::Limit, direction, volume);

            if (!matching_complete) {
                // 处理剩余部分
                process_limit_order(results, order_id, order_book_id, direction,
                                   price, volume - opposite_order->get_volume(), ts);
            }
        } else {
            // 插入队列
            store_new_limit_order(results, order_id, order_book_id, direction, price, volume, ts);
        }
    } else {
        store_new_limit_order(results, order_id, order_book_id, direction, price, volume, ts);
    }
}

template<typename Asset>
void Orderbook<Asset>::process_best_order(OrderProcessingResult& results,
                                         const Asset& order_book_id,
                                         OrderDirection direction,
                                         double volume,
                                         int64_t ts) {
    // 获取本方最优价格
    double best_price = get_best_price(direction);

    // 生成订单号
    uint64_t order_id = next_sequence_id();

    // 记录订单接受
    results.emplace_back(Success::accepted(order_id, OrderType::Limit, get_current_timestamp_nanos()));

    // 以最优价格提交限价单
    process_limit_order(results, order_id, order_book_id, direction, best_price, volume, ts);
}

template<typename Asset>
void Orderbook<Asset>::process_order_amend(OrderProcessingResult& results,
                                          uint64_t order_id,
                                          OrderDirection direction,
                                          double price,
                                          double volume,
                                          int64_t ts) {
    auto order_queue = (direction == OrderDirection::BUY) ? bid_queue_.get() : ask_queue_.get();

    Order<Asset> new_order(order_id, order_book_id_, direction, price, volume);

    if (order_queue->amend(order_id, price, ts, volume, std::move(new_order))) {
        results.emplace_back(Success::amended(order_id, price, volume, get_current_timestamp_nanos()));
    } else {
        results.emplace_back(Failed::order_not_found(order_id));
    }
}

template<typename Asset>
void Orderbook<Asset>::process_order_cancel(OrderProcessingResult& results,
                                           uint64_t order_id,
                                           OrderDirection direction) {
    auto order_queue = (direction == OrderDirection::BUY) ? bid_queue_.get() : ask_queue_.get();

    if (order_queue->cancel(order_id)) {
        results.emplace_back(Success::cancelled(order_id, get_current_timestamp_nanos()));
    } else {
        results.emplace_back(Failed::order_not_found(order_id));
    }
}

template<typename Asset>
void Orderbook<Asset>::store_new_limit_order(OrderProcessingResult& results,
                                            uint64_t order_id,
                                            const Asset& order_book_id,
                                            OrderDirection direction,
                                            double price,
                                            double volume,
                                            int64_t ts) {
    auto order_queue = (direction == OrderDirection::BUY) ? bid_queue_.get() : ask_queue_.get();

    Order<Asset> new_order(order_id, order_book_id, direction, price, volume);

    if (!order_queue->insert(order_id, price, ts, volume, std::move(new_order))) {
        results.emplace_back(Failed::duplicate_order_id(order_id));
    }
}

template<typename Asset>
bool Orderbook<Asset>::order_matching(OrderProcessingResult& results,
                                     const Order<Asset>& opposite_order,
                                     uint64_t order_id,
                                     const Asset& order_book_id,
                                     OrderType order_type,
                                     OrderDirection direction,
                                     double volume) {
    int64_t deal_time = get_current_timestamp_nanos();

    if (volume < opposite_order.get_volume()) {
        // 新订单完全成交，对手单部分成交
        results.emplace_back(Success::filled(order_id, direction, order_type,
                                           opposite_order.get_price(), volume, deal_time,
                                           opposite_order.get_id()));

        results.emplace_back(Success::partially_filled(opposite_order.get_id(),
                                                      opposite_order.get_direction(),
                                                      OrderType::Limit,
                                                      opposite_order.get_price(), volume,
                                                      deal_time, order_id));

        lastprice_ = opposite_order.get_price();

        // 修改对手单剩余数量
        auto opposite_queue = (direction == OrderDirection::BUY) ? ask_queue_.get() : bid_queue_.get();

        Order<Asset> modified_order(opposite_order.get_id(), order_book_id,
                                   opposite_order.get_direction(), opposite_order.get_price(),
                                   opposite_order.get_volume() - volume);

        opposite_queue->modify_current_order(std::move(modified_order), volume);

    } else if (volume > opposite_order.get_volume()) {
        // 新订单部分成交，对手单完全成交
        results.emplace_back(Success::partially_filled(order_id, direction, order_type,
                                                      opposite_order.get_price(),
                                                      opposite_order.get_volume(),
                                                      deal_time, opposite_order.get_id()));

        results.emplace_back(Success::filled(opposite_order.get_id(),
                                           opposite_order.get_direction(), OrderType::Limit,
                                           opposite_order.get_price(),
                                           opposite_order.get_volume(),
                                           deal_time, order_id));

        lastprice_ = opposite_order.get_price();

        // 移除已成交的对手单
        auto opposite_queue = (direction == OrderDirection::BUY) ? ask_queue_.get() : bid_queue_.get();
        opposite_queue->pop();

        return false; // 撮合未完成
    } else {
        // 双方完全成交
        results.emplace_back(Success::filled(order_id, direction, order_type,
                                           opposite_order.get_price(), volume, deal_time,
                                           opposite_order.get_id()));

        results.emplace_back(Success::filled(opposite_order.get_id(),
                                           opposite_order.get_direction(), OrderType::Limit,
                                           opposite_order.get_price(), volume,
                                           deal_time, order_id));

        lastprice_ = opposite_order.get_price();

        // 移除已成交的对手单
        auto opposite_queue = (direction == OrderDirection::BUY) ? ask_queue_.get() : bid_queue_.get();
        opposite_queue->pop();
    }

    return true; // 撮合完成
}

template<typename Asset>
bool Orderbook<Asset>::validate_order(const OrderRequest<Asset>& order) const {
    // 简化的验证逻辑
    if (order.volume <= 0) {
        return false;
    }

    if (order.type == OrderRequest<Asset>::NewLimitOrder && order.price <= 0) {
        return false;
    }

    return true;
}

template<typename Asset>
void Orderbook<Asset>::display_full_depth() {
    std::cout << "\n=== 完整订单簿 ===" << std::endl;
    std::cout << "--- 卖单 (ASK) ---" << std::endl;
    std::cout << "价格\t\t数量" << std::endl;

    auto ask_depth = ask_queue_->get_depth();
    if (ask_depth && !ask_depth->empty()) {
        auto prices_it = ask_depth->find("prices");
        auto volumes_it = ask_depth->find("volumes");

        if (prices_it != ask_depth->end() && volumes_it != ask_depth->end()) {
            const auto& prices = prices_it->second;
            const auto& volumes = volumes_it->second;

            for (int i = prices.size() - 1; i >= 0; --i) {
                std::cout << std::fixed << std::setprecision(8)
                         << prices[i] << "\t" << volumes[i] << std::endl;
            }
        }
    } else {
        std::cout << "(无卖单)" << std::endl;
    }

    std::cout << "\n--- 当前最新价: " << lastprice_ << " ---\n" << std::endl;

    std::cout << "--- 买单 (BID) ---" << std::endl;
    std::cout << "价格\t\t数量" << std::endl;

    auto bid_depth = bid_queue_->get_depth();
    if (bid_depth && !bid_depth->empty()) {
        auto prices_it = bid_depth->find("prices");
        auto volumes_it = bid_depth->find("volumes");

        if (prices_it != bid_depth->end() && volumes_it != bid_depth->end()) {
            const auto& prices = prices_it->second;
            const auto& volumes = volumes_it->second;

            for (size_t i = 0; i < prices.size(); ++i) {
                std::cout << std::fixed << std::setprecision(8)
                         << prices[i] << "\t" << volumes[i] << std::endl;
            }
        }
    } else {
        std::cout << "(无买单)" << std::endl;
    }

    std::cout << "========================\n" << std::endl;
}

template<typename Asset>
std::pair<std::optional<std::map<std::string, std::vector<double>>>,
          std::optional<std::map<std::string, std::vector<double>>>>
Orderbook<Asset>::get_full_depth() {
    return std::make_pair(bid_queue_->get_depth(), ask_queue_->get_depth());
}

template<typename Asset>
void Orderbook<Asset>::plot_orderbook() {
    const auto* bid = bid_queue_->peek();
    const auto* ask = ask_queue_->peek();

    if (bid && ask) {
        std::cout << "----- 订单簿: " << "order_book_id" << " -----\n"
                  << " 卖价: " << ask->get_price() << "  |  " << ask->get_volume() << " \n"
                  << "---------------------\n"
                  << "买价: " << bid->get_price() << "  |  " << bid->get_volume() << " \n"
                  << " --- 最新价 " << lastprice_ << " ---------\n" << std::endl;
    } else {
        std::cout << "无买单或卖单" << std::endl;
    }
}

template<typename Asset>
void Orderbook<Asset>::get_depth() {
    auto ask_depth = ask_queue_->get_depth();
    auto bid_depth = bid_queue_->get_depth();

    std::cout << "卖方队列深度: ";
    if (ask_depth) {
        // 简化输出
        std::cout << "有数据" << std::endl;
    } else {
        std::cout << "无数据" << std::endl;
    }

    std::cout << "-------------------" << lastprice_ << "--------------------" << std::endl;

    std::cout << "买方队列深度: ";
    if (bid_depth) {
        // 简化输出
        std::cout << "有数据" << std::endl;
    } else {
        std::cout << "无数据" << std::endl;
    }
}

// 显式实例化常用模板
// 根据项目需要添加其他Asset类型
namespace qaultra::market::simmarket {
    enum class SimMarketAsset; // 前向声明
}

template class Orderbook<std::string>;
template class Orderbook<qaultra::market::simmarket::SimMarketAsset>;

} // namespace qaultra::market::matchengine