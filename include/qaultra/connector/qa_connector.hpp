#pragma once

#include "database_connector.hpp"
#include "qaultra/protocol/qifi.hpp"
#include <chrono>
#include <memory>

namespace qaultra::connector {

// 前向声明
class QA_Account;

/**
 * @brief QA专用MongoDB连接器
 * 提供量化交易系统专用的数据库操作接口
 */
class QAMongoClient {
public:
    explicit QAMongoClient(const std::string& uri);
    explicit QAMongoClient(const DatabaseConfig& config);
    ~QAMongoClient();

    // 连接管理
    bool connect();
    bool disconnect();
    bool is_connected() const;

    // 用户验证
    bool check_user(const std::string& account_cookie, const std::string& password);

    // QIFI操作
    std::optional<protocol::qifi::QIFI> get_qifi(const std::string& account_cookie);
    bool save_qifi_slice(const protocol::qifi::QIFI& slice);
    bool save_his_qifi_slice(const protocol::qifi::QIFI& slice);
    bool save_sim_qifi_slice(const protocol::qifi::QIFI& slice);

    // Account操作
    std::unique_ptr<QA_Account> get_account(const std::string& account_cookie);
    bool save_account(const QA_Account& account);
    bool save_account_history(const QA_Account& account);

    // 账户列表
    std::vector<std::string> get_account_list();

    // 批量操作
    bool save_multiple_accounts(const std::vector<std::reference_wrapper<const QA_Account>>& accounts);

    // 数据库统计
    struct DatabaseStats {
        int total_accounts = 0;
        int total_history_records = 0;
        int total_sim_records = 0;
        std::string last_update_time;
    };
    DatabaseStats get_database_stats();

    // 配置访问
    const DatabaseConfig& get_config() const;

private:
    std::unique_ptr<MongoConnector> connector_;
    DatabaseConfig config_;
    mutable std::mutex mutex_;

    // 辅助方法
    std::string get_current_time_string() const;
    nlohmann::json qifi_to_json(const protocol::qifi::QIFI& qifi) const;
    protocol::qifi::QIFI json_to_qifi(const nlohmann::json& json) const;

    // 集合名称常量
    static constexpr const char* ACCOUNT_COLLECTION = "account";
    static constexpr const char* HISTORY_COLLECTION = "history";
    static constexpr const char* SIM_COLLECTION = "sim";
    static constexpr const char* REALTIME_DATABASE = "QAREALTIME";
};

/**
 * @brief QA专用ClickHouse连接器
 * 用于高性能时序数据存储和分析
 */
class QAClickHouseClient {
public:
    explicit QAClickHouseClient(const DatabaseConfig& config);
    ~QAClickHouseClient();

    // 连接管理
    bool connect();
    bool disconnect();
    bool is_connected() const;

    // 市场数据操作
    struct MarketDataRecord {
        std::string code;
        std::chrono::system_clock::time_point datetime;
        double open = 0.0;
        double high = 0.0;
        double low = 0.0;
        double close = 0.0;
        double volume = 0.0;
        double amount = 0.0;

        nlohmann::json to_json() const;
        static MarketDataRecord from_json(const nlohmann::json& json);
    };

    bool insert_market_data(const MarketDataRecord& record);
    bool insert_market_data_batch(const std::vector<MarketDataRecord>& records);

    std::vector<MarketDataRecord> query_market_data(
        const std::string& code,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time);

    // 交易记录操作
    struct TradeRecord {
        std::string account_cookie;
        std::string trade_id;
        std::string code;
        std::chrono::system_clock::time_point datetime;
        std::string direction; // "BUY" or "SELL"
        double price = 0.0;
        double volume = 0.0;
        double amount = 0.0;
        std::string status;

        nlohmann::json to_json() const;
        static TradeRecord from_json(const nlohmann::json& json);
    };

    bool insert_trade_record(const TradeRecord& record);
    bool insert_trade_records_batch(const std::vector<TradeRecord>& records);

    std::vector<TradeRecord> query_trade_records(
        const std::string& account_cookie,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time);

    // 性能分析数据
    struct PerformanceRecord {
        std::string account_cookie;
        std::chrono::system_clock::time_point datetime;
        double total_balance = 0.0;
        double available_cash = 0.0;
        double market_value = 0.0;
        double pnl = 0.0;
        double pnl_ratio = 0.0;

        nlohmann::json to_json() const;
        static PerformanceRecord from_json(const nlohmann::json& json);
    };

    bool insert_performance_record(const PerformanceRecord& record);
    std::vector<PerformanceRecord> query_performance_records(
        const std::string& account_cookie,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time);

    // 表管理
    bool create_market_data_table();
    bool create_trade_records_table();
    bool create_performance_table();
    bool optimize_all_tables();

    // 聚合查询
    struct AggregationResult {
        nlohmann::json data;
        std::string query_time;
        int row_count = 0;
    };

    AggregationResult execute_aggregation_query(const std::string& query);

private:
    std::unique_ptr<ClickHouseConnector> connector_;
    DatabaseConfig config_;
    mutable std::mutex mutex_;

    // 表名常量
    static constexpr const char* MARKET_DATA_TABLE = "market_data";
    static constexpr const char* TRADE_RECORDS_TABLE = "trade_records";
    static constexpr const char* PERFORMANCE_TABLE = "performance_records";

    // 辅助方法
    std::string time_point_to_string(const std::chrono::system_clock::time_point& tp) const;
    std::chrono::system_clock::time_point string_to_time_point(const std::string& str) const;
};

/**
 * @brief QA数据库管理器
 * 统一管理MongoDB和ClickHouse连接
 */
class QADatabaseManager {
public:
    struct DatabaseConfiguration {
        DatabaseConfig mongo_config;
        DatabaseConfig clickhouse_config;
        bool enable_mongo = true;
        bool enable_clickhouse = false;
        bool auto_sync = false;  // 自动同步数据到ClickHouse
    };

    explicit QADatabaseManager(const DatabaseConfiguration& config);
    ~QADatabaseManager();

    // 连接管理
    bool connect_all();
    bool disconnect_all();
    bool is_connected() const;

    // 获取连接器
    QAMongoClient* get_mongo_client();
    QAClickHouseClient* get_clickhouse_client();

    // 统一账户操作
    std::unique_ptr<QA_Account> get_account(const std::string& account_cookie);
    bool save_account(const QA_Account& account, bool sync_to_clickhouse = false);

    // 数据同步
    bool sync_account_to_clickhouse(const std::string& account_cookie);
    bool sync_all_accounts_to_clickhouse();

    // 健康检查
    struct HealthStatus {
        bool mongo_healthy = false;
        bool clickhouse_healthy = false;
        std::string mongo_error;
        std::string clickhouse_error;
        std::chrono::system_clock::time_point last_check;
    };

    HealthStatus check_health();

    // 配置访问
    const DatabaseConfiguration& get_configuration() const { return config_; }

private:
    DatabaseConfiguration config_;
    std::unique_ptr<QAMongoClient> mongo_client_;
    std::unique_ptr<QAClickHouseClient> clickhouse_client_;
    mutable std::mutex mutex_;

    void initialize_clients();
};

/**
 * @brief 全局数据库管理器
 * 提供单例模式的数据库访问
 */
class GlobalDatabaseManager {
public:
    static GlobalDatabaseManager& instance();

    // 初始化
    bool initialize(const QADatabaseManager::DatabaseConfiguration& config);
    bool is_initialized() const;

    // 访问数据库管理器
    QADatabaseManager* get_manager();
    QAMongoClient* get_mongo_client();
    QAClickHouseClient* get_clickhouse_client();

    // 配置管理
    void set_default_mongo_config(const DatabaseConfig& config);
    void set_default_clickhouse_config(const DatabaseConfig& config);

    DatabaseConfig get_default_mongo_config() const;
    DatabaseConfig get_default_clickhouse_config() const;

private:
    GlobalDatabaseManager() = default;
    ~GlobalDatabaseManager() = default;

    std::unique_ptr<QADatabaseManager> manager_;
    DatabaseConfig default_mongo_config_;
    DatabaseConfig default_clickhouse_config_;
    bool initialized_ = false;
    mutable std::mutex mutex_;

    // 禁用拷贝和赋值
    GlobalDatabaseManager(const GlobalDatabaseManager&) = delete;
    GlobalDatabaseManager& operator=(const GlobalDatabaseManager&) = delete;
};

} // namespace qaultra::connector