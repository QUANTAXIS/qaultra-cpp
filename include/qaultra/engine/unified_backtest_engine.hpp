#pragma once

#include "../account/unified_account.hpp"
#include "../data/unified_datatype.hpp"
#include "../protocol/qifi.hpp"
#include "../protocol/mifi.hpp"
#include "../protocol/tifi.hpp"
#include "../util/uuid_generator.hpp"
#include "../market/simmarket.hpp"
#include <nlohmann/json.hpp>

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <queue>
#include <chrono>
#include <thread>
#include <future>

namespace qaultra::engine {

/**
 * @brief 统一回测配置结构
 */
struct UnifiedBacktestConfig {
    std::string start_date = "2024-01-01";         // 开始日期
    std::string end_date = "2024-12-31";           // 结束日期
    double initial_cash = 1000000.0;               // 初始资金
    double commission_rate = 0.0003;               // 手续费率
    double slippage = 0.0001;                      // 滑点
    std::string benchmark = "000300.XSHG";         // 基准指数

    // 引擎配置
    bool enable_matching_engine = false;           // 是否启用撮合引擎
    bool enable_parallel_processing = true;        // 是否启用并行处理
    int max_threads = std::thread::hardware_concurrency(); // 最大线程数

    // 风控配置
    double max_position_ratio = 0.3;               // 单只股票最大仓位比例
    double stop_loss_ratio = -0.1;                 // 止损比例
    double take_profit_ratio = 0.2;                // 止盈比例

    // 其他配置
    bool enable_cache = true;                      // 是否启用缓存
    bool enable_logging = true;                    // 是否启用日志
    bool enable_performance_tracking = true;       // 是否启用性能跟踪
    std::string log_level = "INFO";                // 日志级别

    nlohmann::json to_json() const;
    static UnifiedBacktestConfig from_json(const nlohmann::json& j);
};

/**
 * @brief 统一回测结果结构
 */
struct UnifiedBacktestResults {
    // 基本指标
    double total_return = 0.0;                     // 总收益率
    double annual_return = 0.0;                    // 年化收益率
    double sharpe_ratio = 0.0;                     // 夏普比率
    double sortino_ratio = 0.0;                    // 索提诺比率
    double calmar_ratio = 0.0;                     // 卡尔玛比率
    double max_drawdown = 0.0;                     // 最大回撤
    double volatility = 0.0;                       // 波动率
    double downside_deviation = 0.0;               // 下行偏差

    // 交易统计
    double win_rate = 0.0;                         // 胜率
    double profit_factor = 0.0;                    // 盈亏比
    int total_trades = 0;                          // 总交易次数
    int winning_trades = 0;                        // 盈利交易次数
    int losing_trades = 0;                         // 亏损交易次数
    double final_value = 0.0;                      // 最终资产价值
    double max_single_trade_profit = 0.0;          // 最大单笔盈利
    double max_single_trade_loss = 0.0;            // 最大单笔亏损

    // 风险指标
    double beta = 0.0;                             // Beta系数
    double alpha = 0.0;                            // Alpha系数
    double var_95 = 0.0;                           // 95%置信度VaR
    double cvar_95 = 0.0;                          // 95%置信度CVaR
    double information_ratio = 0.0;                // 信息比率

    // 时间序列数据
    std::vector<double> equity_curve;              // 资产曲线
    std::vector<double> daily_returns;             // 每日收益率
    std::vector<double> benchmark_returns;         // 基准收益率
    std::vector<std::string> trade_dates;          // 交易日期

    // 详细交易记录
    std::vector<protocol::tifi::Trade> trade_history;

    // 策略特定指标
    std::unordered_map<std::string, double> strategy_metrics;

    nlohmann::json to_json() const;
    static UnifiedBacktestResults from_json(const nlohmann::json& j);
};

/**
 * @brief 事件类型枚举
 */
enum class EventType {
    MARKET_DATA,        // 市场数据事件
    ORDER,              // 订单事件
    TRADE,              // 成交事件
    TIMER,              // 定时器事件
    POSITION,           // 持仓事件
    ACCOUNT,            // 账户事件
    STRATEGY,           // 策略事件
    RISK                // 风控事件
};

/**
 * @brief 基础事件类
 */
struct Event {
    EventType type;
    std::string timestamp;
    std::unordered_map<std::string, std::string> data;

    Event(EventType t) : type(t) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        timestamp = std::to_string(time_t);
    }
    virtual ~Event() = default;
};

/**
 * @brief 市场数据事件
 */
struct MarketDataEvent : public Event {
    data::UnifiedKline kline;

    MarketDataEvent(const data::UnifiedKline& k)
        : Event(EventType::MARKET_DATA), kline(k) {}
};

/**
 * @brief 订单事件
 */
struct OrderEvent : public Event {
    protocol::tifi::Order order;

    OrderEvent(const protocol::tifi::Order& o)
        : Event(EventType::ORDER), order(o) {}
};

/**
 * @brief 成交事件
 */
struct TradeEvent : public Event {
    protocol::tifi::Trade trade;

    TradeEvent(const protocol::tifi::Trade& t)
        : Event(EventType::TRADE), trade(t) {}
};

/**
 * @brief 事件引擎
 */
class EventEngine {
public:
    using EventHandler = std::function<void(std::shared_ptr<Event>)>;

    EventEngine() = default;
    ~EventEngine() = default;

    void register_handler(EventType type, EventHandler handler);
    void put_event(std::shared_ptr<Event> event);
    void process_events();
    void clear_events();
    void start();
    void stop();

private:
    std::unordered_map<EventType, std::vector<EventHandler>> handlers_;
    std::queue<std::shared_ptr<Event>> event_queue_;
    std::mutex queue_mutex_;
    std::atomic<bool> running_{false};
    std::thread processing_thread_;

    void process_events_loop();
};

/**
 * @brief 统一策略上下文
 */
struct UnifiedStrategyContext {
    std::shared_ptr<account::UnifiedAccount> account;
    std::vector<std::string> universe;            // 股票池
    std::string current_date;                     // 当前日期
    std::unordered_map<std::string, double> current_prices; // 当前价格

    // 数据访问接口
    double get_price(const std::string& symbol) const;
    std::vector<double> get_history(const std::string& symbol, int window, const std::string& field = "close") const;
    std::optional<account::Position> get_position(const std::string& symbol) const;
    double get_cash() const;
    double get_total_value() const;

    // 数据缓存
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, std::shared_ptr<data::MarketDataContainer>> data_cache_;

    // 日志接口
    void log(const std::string& message, const std::string& level = "INFO") const;
};

/**
 * @brief 统一策略基类
 */
class UnifiedStrategy {
public:
    UnifiedStrategy(const std::string& name = "UnifiedStrategy") : name_(name) {}
    virtual ~UnifiedStrategy() = default;

    /**
     * @brief 策略初始化
     */
    virtual void initialize(UnifiedStrategyContext& context) = 0;

    /**
     * @brief 处理市场数据
     */
    virtual void handle_data(UnifiedStrategyContext& context) = 0;

    /**
     * @brief 开盘前处理
     */
    virtual void before_market_open(UnifiedStrategyContext& context) {}

    /**
     * @brief 收盘后处理
     */
    virtual void after_market_close(UnifiedStrategyContext& context) {}

    /**
     * @brief 策略结束
     */
    virtual void finalize(UnifiedStrategyContext& context) {}

    /**
     * @brief 获取策略参数
     */
    virtual std::unordered_map<std::string, double> get_parameters() const = 0;

    /**
     * @brief 设置策略参数
     */
    virtual void set_parameter(const std::string& name, double value) = 0;

    /**
     * @brief 获取策略名称
     */
    const std::string& get_name() const { return name_; }

    /**
     * @brief 设置策略名称
     */
    void set_name(const std::string& name) { name_ = name; }

protected:
    std::string name_;
    std::unordered_map<std::string, double> parameters_;
};

/**
 * @brief 统一移动平均策略
 */
class UnifiedSMAStrategy : public UnifiedStrategy {
public:
    UnifiedSMAStrategy(int fast_window = 5, int slow_window = 20, const std::string& name = "UnifiedSMA")
        : UnifiedStrategy(name), fast_window_(fast_window), slow_window_(slow_window) {}

    void initialize(UnifiedStrategyContext& context) override;
    void handle_data(UnifiedStrategyContext& context) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    int fast_window_;
    int slow_window_;
    std::unordered_map<std::string, bool> positions_;
};

/**
 * @brief 统一动量策略
 */
class UnifiedMomentumStrategy : public UnifiedStrategy {
public:
    UnifiedMomentumStrategy(int lookback_window = 20, double threshold = 0.05, const std::string& name = "UnifiedMomentum")
        : UnifiedStrategy(name), lookback_window_(lookback_window), threshold_(threshold) {}

    void initialize(UnifiedStrategyContext& context) override;
    void handle_data(UnifiedStrategyContext& context) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    int lookback_window_;
    double threshold_;
    std::unordered_map<std::string, std::vector<double>> price_history_;
};

/**
 * @brief 统一均值回归策略
 */
class UnifiedMeanReversionStrategy : public UnifiedStrategy {
public:
    UnifiedMeanReversionStrategy(int window = 20, double z_score_threshold = 2.0, const std::string& name = "UnifiedMeanReversion")
        : UnifiedStrategy(name), window_(window), z_score_threshold_(z_score_threshold) {}

    void initialize(UnifiedStrategyContext& context) override;
    void handle_data(UnifiedStrategyContext& context) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    void set_parameter(const std::string& name, double value) override;

private:
    int window_;
    double z_score_threshold_;
    std::unordered_map<std::string, std::vector<double>> price_buffer_;
};

/**
 * @brief 数据管理器
 */
class UnifiedDataManager {
public:
    UnifiedDataManager() = default;
    ~UnifiedDataManager() = default;

    // 数据加载
    bool load_data(const std::string& symbol, const std::string& start_date,
                   const std::string& end_date, const std::string& frequency = "1d");
    bool load_data_from_file(const std::string& filename);
    bool load_data_from_database();

    // 数据访问
    std::optional<data::UnifiedKline> get_data(const std::string& symbol, const std::string& date) const;
    std::vector<data::UnifiedKline> get_data_range(const std::string& symbol,
                                                   const std::string& start_date,
                                                   const std::string& end_date) const;
    std::shared_ptr<data::MarketDataContainer> get_symbol_data(const std::string& symbol) const;

    // 数据管理
    void add_data(const std::string& symbol, const data::UnifiedKline& kline);
    void clear_data();
    bool has_data(const std::string& symbol) const;
    std::vector<std::string> get_available_symbols() const;
    std::vector<std::string> get_date_index() const;

    // 当前状态
    void set_current_date(const std::string& date);
    std::string get_current_date() const;

private:
    std::unordered_map<std::string, std::shared_ptr<data::MarketDataContainer>> data_;
    std::vector<std::string> date_index_;
    std::string current_date_;
    mutable std::mutex data_mutex_;
};

/**
 * @brief 统一回测引擎主类
 * 集成simple和full版本的最佳功能
 */
class UnifiedBacktestEngine {
public:
    explicit UnifiedBacktestEngine(const UnifiedBacktestConfig& config);
    ~UnifiedBacktestEngine();

    /**
     * @brief 添加策略
     */
    void add_strategy(std::shared_ptr<UnifiedStrategy> strategy);

    /**
     * @brief 设置股票池
     */
    void set_universe(const std::vector<std::string>& symbols);

    /**
     * @brief 加载数据
     */
    bool load_data(const std::string& data_source = "");

    /**
     * @brief 运行回测
     */
    UnifiedBacktestResults run();

    /**
     * @brief 保存结果
     */
    bool save_results(const std::string& filename) const;

    /**
     * @brief 获取性能总结
     */
    std::unordered_map<std::string, double> get_performance_summary() const;

    /**
     * @brief 获取资产曲线数据
     */
    std::vector<std::pair<std::string, double>> get_equity_curve() const;

    /**
     * @brief 获取交易分析
     */
    std::unordered_map<std::string, std::vector<double>> get_trade_analysis() const;

    /**
     * @brief 设置事件回调
     */
    void set_on_trade_callback(std::function<void(const protocol::tifi::Trade&)> callback);
    void set_on_order_callback(std::function<void(const protocol::tifi::Order&)> callback);

    /**
     * @brief 获取当前账户状态
     */
    protocol::qifi::QIFI get_current_account_state() const;

    /**
     * @brief 强制停止回测
     */
    void stop();

    /**
     * @brief 检查是否正在运行
     */
    bool is_running() const { return is_running_; }

private:
    UnifiedBacktestConfig config_;
    UnifiedBacktestResults results_;

    // 核心组件
    std::shared_ptr<account::UnifiedAccount> account_;
    std::unique_ptr<market::SimMarket> market_sim_;
    std::shared_ptr<EventEngine> event_engine_;
    std::unique_ptr<UnifiedDataManager> data_manager_;
    std::vector<std::shared_ptr<UnifiedStrategy>> strategies_;

    // 数据管理
    std::vector<std::string> universe_;
    std::vector<std::string> date_index_;

    // 运行状态
    std::atomic<bool> is_running_{false};
    size_t current_index_ = 0;
    std::string current_date_;

    // 性能记录
    std::vector<double> daily_equity_;
    std::vector<protocol::tifi::Trade> trade_history_;
    std::vector<std::string> trade_dates_;

    // 线程安全
    mutable std::mutex results_mutex_;
    mutable std::mutex state_mutex_;

    // 回调函数
    std::function<void(const protocol::tifi::Trade&)> on_trade_callback_;
    std::function<void(const protocol::tifi::Order&)> on_order_callback_;

    // 内部方法
    void initialize_components();
    void setup_event_handlers();
    bool validate_configuration() const;
    void run_single_day(const std::string& date);
    void update_market_data(const std::string& date);
    void execute_strategies(UnifiedStrategyContext& context);
    void process_orders();
    void record_daily_performance();
    void calculate_performance_metrics();

    // 性能计算方法
    double calculate_sharpe_ratio() const;
    double calculate_sortino_ratio() const;
    double calculate_calmar_ratio() const;
    double calculate_max_drawdown() const;
    double calculate_annual_return() const;
    double calculate_volatility() const;
    double calculate_downside_deviation() const;
    double calculate_win_rate() const;
    double calculate_profit_factor() const;
    double calculate_beta() const;
    double calculate_alpha() const;
    double calculate_var_95() const;
    double calculate_cvar_95() const;
    double calculate_information_ratio() const;

    // 工具方法
    std::vector<double> calculate_returns(const std::vector<double>& prices) const;
    std::vector<double> load_benchmark_data() const;
    void log_message(const std::string& message, const std::string& level = "INFO") const;

    // 事件处理器
    void on_market_data_event(std::shared_ptr<Event> event);
    void on_order_event(std::shared_ptr<Event> event);
    void on_trade_event(std::shared_ptr<Event> event);
};

/**
 * @brief 性能分析工具函数
 */
namespace unified_utils {
    double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate = 0.0);
    double calculate_sortino_ratio(const std::vector<double>& returns, double risk_free_rate = 0.0);
    double calculate_calmar_ratio(double annual_return, double max_drawdown);
    double calculate_max_drawdown(const std::vector<double>& equity_curve);
    double calculate_annual_return(const std::vector<double>& equity_curve, int trading_days = 252);
    double calculate_volatility(const std::vector<double>& returns, bool annualized = true);
    double calculate_downside_deviation(const std::vector<double>& returns, double risk_free_rate = 0.0);
    double calculate_win_rate(const std::vector<double>& trade_returns);
    double calculate_profit_factor(const std::vector<double>& trade_returns);
    std::vector<double> calculate_rolling_sharpe(const std::vector<double>& returns, int window);
    double calculate_beta(const std::vector<double>& strategy_returns, const std::vector<double>& benchmark_returns);
    double calculate_alpha(const std::vector<double>& strategy_returns, const std::vector<double>& benchmark_returns, double risk_free_rate = 0.0);
    double calculate_var_95(const std::vector<double>& returns);
    double calculate_cvar_95(const std::vector<double>& returns);
    double calculate_information_ratio(const std::vector<double>& strategy_returns, const std::vector<double>& benchmark_returns);

    // 时间序列工具
    std::vector<std::string> generate_trading_dates(const std::string& start_date, const std::string& end_date);
    bool is_trading_day(const std::string& date);
    std::string add_trading_days(const std::string& date, int days);
}

/**
 * @brief 统一策略工厂函数
 */
namespace unified_factory {
    std::shared_ptr<UnifiedStrategy> create_sma_strategy(int fast_window = 5, int slow_window = 20, const std::string& name = "SMA");
    std::shared_ptr<UnifiedStrategy> create_momentum_strategy(int lookback_window = 20, double threshold = 0.05, const std::string& name = "Momentum");
    std::shared_ptr<UnifiedStrategy> create_mean_reversion_strategy(int window = 20, double z_score_threshold = 2.0, const std::string& name = "MeanReversion");

    // 从配置创建策略
    std::shared_ptr<UnifiedStrategy> create_strategy_from_config(const nlohmann::json& config);

    // 组合策略创建
    std::vector<std::shared_ptr<UnifiedStrategy>> create_multi_strategy_portfolio(const nlohmann::json& config);
}

} // namespace qaultra::engine