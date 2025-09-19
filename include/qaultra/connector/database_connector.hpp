#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include <functional>
#include <mutex>

#include <nlohmann/json.hpp>

namespace qaultra::connector {

// 前向声明
class QAAccount;
struct QIFI;

// 数据库类型枚举
enum class DatabaseType {
    MongoDB,
    ClickHouse,
    PostgreSQL,
    MySQL,
    Redis
};

/**
 * @brief 数据库操作结果
 */
struct DatabaseResult {
    bool success = false;
    std::string error_message;
    std::optional<nlohmann::json> data;

    DatabaseResult() = default;
    explicit DatabaseResult(bool success) : success(success) {}
    DatabaseResult(bool success, const std::string& error)
        : success(success), error_message(error) {}
    DatabaseResult(bool success, const nlohmann::json& data)
        : success(success), data(data) {}
};

/**
 * @brief 数据库连接配置
 */
struct DatabaseConfig {
    std::string uri;
    std::string database_name;
    std::string username;
    std::string password;
    int timeout_seconds = 30;
    int max_connections = 10;
    bool use_ssl = false;

    // 扩展配置
    std::unordered_map<std::string, std::string> extra_options;
};

/**
 * @brief 查询选项
 */
struct QueryOptions {
    int limit = 0;              // 0表示无限制
    int skip = 0;               // 跳过记录数
    int batch_size = 1000;      // 批次大小
    nlohmann::json projection;  // 投影字段
    nlohmann::json sort;        // 排序规则

    QueryOptions() = default;
    QueryOptions(int limit) : limit(limit) {}
    QueryOptions(int limit, int skip) : limit(limit), skip(skip) {}
};

/**
 * @brief 抽象数据库连接器接口
 */
class IDatabaseConnector {
public:
    virtual ~IDatabaseConnector() = default;

    // 连接管理
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool is_connected() const = 0;
    virtual bool test_connection() = 0;

    // 基础CRUD操作
    virtual DatabaseResult insert_one(const std::string& collection,
                                    const nlohmann::json& document) = 0;
    virtual DatabaseResult insert_many(const std::string& collection,
                                     const std::vector<nlohmann::json>& documents) = 0;
    virtual DatabaseResult find_one(const std::string& collection,
                                   const nlohmann::json& filter,
                                   const QueryOptions& options = {}) = 0;
    virtual DatabaseResult find_many(const std::string& collection,
                                    const nlohmann::json& filter,
                                    const QueryOptions& options = {}) = 0;
    virtual DatabaseResult update_one(const std::string& collection,
                                     const nlohmann::json& filter,
                                     const nlohmann::json& update,
                                     bool upsert = false) = 0;
    virtual DatabaseResult update_many(const std::string& collection,
                                      const nlohmann::json& filter,
                                      const nlohmann::json& update) = 0;
    virtual DatabaseResult delete_one(const std::string& collection,
                                     const nlohmann::json& filter) = 0;
    virtual DatabaseResult delete_many(const std::string& collection,
                                      const nlohmann::json& filter) = 0;

    // 聚合操作
    virtual DatabaseResult aggregate(const std::string& collection,
                                   const std::vector<nlohmann::json>& pipeline) = 0;

    // 统计操作
    virtual DatabaseResult count_documents(const std::string& collection,
                                          const nlohmann::json& filter = {}) = 0;

    // 索引管理
    virtual DatabaseResult create_index(const std::string& collection,
                                       const nlohmann::json& keys,
                                       const nlohmann::json& options = {}) = 0;
    virtual DatabaseResult list_indexes(const std::string& collection) = 0;
    virtual DatabaseResult drop_index(const std::string& collection,
                                     const std::string& index_name) = 0;

    // 批量操作
    virtual DatabaseResult bulk_write(const std::string& collection,
                                     const std::vector<nlohmann::json>& operations) = 0;

    // 事务支持
    virtual DatabaseResult begin_transaction() = 0;
    virtual DatabaseResult commit_transaction() = 0;
    virtual DatabaseResult abort_transaction() = 0;

    // 配置信息
    virtual const DatabaseConfig& get_config() const = 0;
    virtual std::string get_connection_string() const = 0;
};

/**
 * @brief MongoDB连接器实现
 */
class MongoConnector : public IDatabaseConnector {
public:
    explicit MongoConnector(const DatabaseConfig& config);
    ~MongoConnector() override;

    // 实现接口
    bool connect() override;
    bool disconnect() override;
    bool is_connected() const override;
    bool test_connection() override;

    DatabaseResult insert_one(const std::string& collection,
                             const nlohmann::json& document) override;
    DatabaseResult insert_many(const std::string& collection,
                              const std::vector<nlohmann::json>& documents) override;
    DatabaseResult find_one(const std::string& collection,
                           const nlohmann::json& filter,
                           const QueryOptions& options = {}) override;
    DatabaseResult find_many(const std::string& collection,
                            const nlohmann::json& filter,
                            const QueryOptions& options = {}) override;
    DatabaseResult update_one(const std::string& collection,
                             const nlohmann::json& filter,
                             const nlohmann::json& update,
                             bool upsert = false) override;
    DatabaseResult update_many(const std::string& collection,
                              const nlohmann::json& filter,
                              const nlohmann::json& update) override;
    DatabaseResult delete_one(const std::string& collection,
                             const nlohmann::json& filter) override;
    DatabaseResult delete_many(const std::string& collection,
                              const nlohmann::json& filter) override;
    DatabaseResult aggregate(const std::string& collection,
                           const std::vector<nlohmann::json>& pipeline) override;
    DatabaseResult count_documents(const std::string& collection,
                                  const nlohmann::json& filter = {}) override;
    DatabaseResult create_index(const std::string& collection,
                               const nlohmann::json& keys,
                               const nlohmann::json& options = {}) override;
    DatabaseResult list_indexes(const std::string& collection) override;
    DatabaseResult drop_index(const std::string& collection,
                             const std::string& index_name) override;
    DatabaseResult bulk_write(const std::string& collection,
                             const std::vector<nlohmann::json>& operations) override;
    DatabaseResult begin_transaction() override;
    DatabaseResult commit_transaction() override;
    DatabaseResult abort_transaction() override;

    const DatabaseConfig& get_config() const override { return config_; }
    std::string get_connection_string() const override;

    // MongoDB特有功能
    DatabaseResult create_collection(const std::string& collection_name,
                                   const nlohmann::json& options = {});
    DatabaseResult drop_collection(const std::string& collection_name);
    DatabaseResult list_collections();
    DatabaseResult get_database_stats();

private:
    DatabaseConfig config_;
    void* client_impl_;    // 隐藏MongoDB实现细节
    mutable std::mutex mutex_;
    bool connected_;

    // 内部辅助方法
    nlohmann::json bson_to_json(const void* bson_doc) const;
    void* json_to_bson(const nlohmann::json& json_doc) const;
    void cleanup_impl();
};

/**
 * @brief ClickHouse连接器实现
 */
class ClickHouseConnector : public IDatabaseConnector {
public:
    explicit ClickHouseConnector(const DatabaseConfig& config);
    ~ClickHouseConnector() override;

    // 实现接口
    bool connect() override;
    bool disconnect() override;
    bool is_connected() const override;
    bool test_connection() override;

    DatabaseResult insert_one(const std::string& collection,
                             const nlohmann::json& document) override;
    DatabaseResult insert_many(const std::string& collection,
                              const std::vector<nlohmann::json>& documents) override;
    DatabaseResult find_one(const std::string& collection,
                           const nlohmann::json& filter,
                           const QueryOptions& options = {}) override;
    DatabaseResult find_many(const std::string& collection,
                            const nlohmann::json& filter,
                            const QueryOptions& options = {}) override;
    DatabaseResult update_one(const std::string& collection,
                             const nlohmann::json& filter,
                             const nlohmann::json& update,
                             bool upsert = false) override;
    DatabaseResult update_many(const std::string& collection,
                              const nlohmann::json& filter,
                              const nlohmann::json& update) override;
    DatabaseResult delete_one(const std::string& collection,
                             const nlohmann::json& filter) override;
    DatabaseResult delete_many(const std::string& collection,
                              const nlohmann::json& filter) override;
    DatabaseResult aggregate(const std::string& collection,
                           const std::vector<nlohmann::json>& pipeline) override;
    DatabaseResult count_documents(const std::string& collection,
                                  const nlohmann::json& filter = {}) override;
    DatabaseResult create_index(const std::string& collection,
                               const nlohmann::json& keys,
                               const nlohmann::json& options = {}) override;
    DatabaseResult list_indexes(const std::string& collection) override;
    DatabaseResult drop_index(const std::string& collection,
                             const std::string& index_name) override;
    DatabaseResult bulk_write(const std::string& collection,
                             const std::vector<nlohmann::json>& operations) override;
    DatabaseResult begin_transaction() override;
    DatabaseResult commit_transaction() override;
    DatabaseResult abort_transaction() override;

    const DatabaseConfig& get_config() const override { return config_; }
    std::string get_connection_string() const override;

    // ClickHouse特有功能
    DatabaseResult execute_query(const std::string& query);
    DatabaseResult create_table(const std::string& table_name,
                               const std::string& schema);
    DatabaseResult drop_table(const std::string& table_name);
    DatabaseResult get_table_schema(const std::string& table_name);
    DatabaseResult optimize_table(const std::string& table_name);

private:
    DatabaseConfig config_;
    void* client_impl_;    // 隐藏ClickHouse实现细节
    mutable std::mutex mutex_;
    bool connected_;

    // 内部辅助方法
    std::string json_filter_to_sql(const nlohmann::json& filter) const;
    std::string json_to_insert_values(const nlohmann::json& document) const;
    void cleanup_impl();
};

/**
 * @brief 连接器工厂
 */
class ConnectorFactory {
public:

    static std::unique_ptr<IDatabaseConnector> create_connector(
        DatabaseType type, const DatabaseConfig& config);

    static std::unique_ptr<MongoConnector> create_mongo_connector(
        const DatabaseConfig& config);

    static std::unique_ptr<ClickHouseConnector> create_clickhouse_connector(
        const DatabaseConfig& config);

    // 预设配置
    static DatabaseConfig create_mongo_config(
        const std::string& uri = "mongodb://localhost:27017",
        const std::string& database = "quantaxis");

    static DatabaseConfig create_clickhouse_config(
        const std::string& host = "localhost",
        int port = 9000,
        const std::string& database = "quantaxis",
        const std::string& username = "default",
        const std::string& password = "");
};

/**
 * @brief 连接池管理器
 */
class ConnectionPool {
public:
    ConnectionPool(DatabaseType type, const DatabaseConfig& config,
                   int max_connections = 10);
    ~ConnectionPool();

    // 获取连接
    std::shared_ptr<IDatabaseConnector> get_connection();
    void return_connection(std::shared_ptr<IDatabaseConnector> conn);

    // 池状态
    int get_active_connections() const;
    int get_available_connections() const;
    int get_total_connections() const;

    // 池管理
    void resize_pool(int new_size);
    void clear_pool();

private:
    DatabaseType type_;
    DatabaseConfig config_;
    int max_connections_;

    std::vector<std::shared_ptr<IDatabaseConnector>> connections_;
    std::vector<bool> connection_status_;  // true = in use, false = available

    mutable std::mutex pool_mutex_;

    void initialize_pool();
    std::shared_ptr<IDatabaseConnector> create_new_connection();
};

} // namespace qaultra::connector