#include "qaultra/connector/database_connector.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>

namespace qaultra::connector {

// =======================
// MongoConnector 实现
// =======================

MongoConnector::MongoConnector(const DatabaseConfig& config)
    : config_(config), client_impl_(nullptr), connected_(false) {
}

MongoConnector::~MongoConnector() {
    disconnect();
    cleanup_impl();
}

bool MongoConnector::connect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (connected_) {
        return true;
    }

    try {
        // 注意：这是简化实现，实际需要链接MongoDB C++驱动
        // 这里我们提供接口框架，实际MongoDB连接需要添加依赖
        std::cout << "连接到MongoDB: " << config_.uri << std::endl;
        connected_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "MongoDB连接失败: " << e.what() << std::endl;
        return false;
    }
}

bool MongoConnector::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return true;
    }

    try {
        std::cout << "断开MongoDB连接" << std::endl;
        connected_ = false;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "MongoDB断开连接失败: " << e.what() << std::endl;
        return false;
    }
}

bool MongoConnector::is_connected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}

bool MongoConnector::test_connection() {
    if (!is_connected()) {
        return false;
    }

    try {
        // 执行简单的ping操作
        return true;
    } catch (const std::exception& e) {
        std::cerr << "MongoDB连接测试失败: " << e.what() << std::endl;
        return false;
    }
}

DatabaseResult MongoConnector::insert_one(const std::string& collection,
                                         const nlohmann::json& document) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        // 模拟插入操作
        std::cout << "插入文档到集合 " << collection << ": " << document.dump() << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("插入失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::insert_many(const std::string& collection,
                                          const std::vector<nlohmann::json>& documents) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "批量插入 " << documents.size() << " 个文档到集合 " << collection << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("批量插入失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::find_one(const std::string& collection,
                                       const nlohmann::json& filter,
                                       const QueryOptions& options) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        // 模拟查询操作
        nlohmann::json result = {
            {"_id", "507f1f77bcf86cd799439011"},
            {"account_cookie", "test_account"},
            {"balance", 100000.0}
        };
        return DatabaseResult(true, result);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("查询失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::find_many(const std::string& collection,
                                        const nlohmann::json& filter,
                                        const QueryOptions& options) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        // 模拟查询多个文档
        nlohmann::json results = nlohmann::json::array();
        for (int i = 0; i < std::min(options.limit > 0 ? options.limit : 10, 10); ++i) {
            nlohmann::json doc = {
                {"_id", "507f1f77bcf86cd79943901" + std::to_string(i)},
                {"account_cookie", "account_" + std::to_string(i)},
                {"balance", 100000.0 + i * 1000}
            };
            results.push_back(doc);
        }
        return DatabaseResult(true, results);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("查询失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::update_one(const std::string& collection,
                                         const nlohmann::json& filter,
                                         const nlohmann::json& update,
                                         bool upsert) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "更新集合 " << collection << " 中的文档" << std::endl;
        std::cout << "过滤条件: " << filter.dump() << std::endl;
        std::cout << "更新内容: " << update.dump() << std::endl;
        std::cout << "Upsert: " << (upsert ? "是" : "否") << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("更新失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::update_many(const std::string& collection,
                                          const nlohmann::json& filter,
                                          const nlohmann::json& update) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "批量更新集合 " << collection << " 中的文档" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("批量更新失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::delete_one(const std::string& collection,
                                         const nlohmann::json& filter) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "删除集合 " << collection << " 中的一个文档" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("删除失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::delete_many(const std::string& collection,
                                          const nlohmann::json& filter) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "批量删除集合 " << collection << " 中的文档" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("批量删除失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::aggregate(const std::string& collection,
                                        const std::vector<nlohmann::json>& pipeline) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "对集合 " << collection << " 执行聚合操作" << std::endl;
        nlohmann::json result = nlohmann::json::array();
        return DatabaseResult(true, result);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("聚合操作失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::count_documents(const std::string& collection,
                                              const nlohmann::json& filter) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        nlohmann::json result = {{"count", 100}};
        return DatabaseResult(true, result);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("计数失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::create_index(const std::string& collection,
                                           const nlohmann::json& keys,
                                           const nlohmann::json& options) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "在集合 " << collection << " 上创建索引" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("创建索引失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::list_indexes(const std::string& collection) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        nlohmann::json indexes = nlohmann::json::array();
        indexes.push_back({{"name", "_id_"}, {"key", {{"_id", 1}}}});
        return DatabaseResult(true, indexes);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("列出索引失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::drop_index(const std::string& collection,
                                         const std::string& index_name) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "删除集合 " << collection << " 上的索引 " << index_name << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("删除索引失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::bulk_write(const std::string& collection,
                                         const std::vector<nlohmann::json>& operations) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "对集合 " << collection << " 执行 " << operations.size() << " 个批量操作" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("批量操作失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::begin_transaction() {
    try {
        std::cout << "开始MongoDB事务" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("开始事务失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::commit_transaction() {
    try {
        std::cout << "提交MongoDB事务" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("提交事务失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::abort_transaction() {
    try {
        std::cout << "回滚MongoDB事务" << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("回滚事务失败: ") + std::string(e.what()));
    }
}

std::string MongoConnector::get_connection_string() const {
    return config_.uri + "/" + config_.database_name;
}

DatabaseResult MongoConnector::create_collection(const std::string& collection_name,
                                                const nlohmann::json& options) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "创建集合: " << collection_name << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("创建集合失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::drop_collection(const std::string& collection_name) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "删除集合: " << collection_name << std::endl;
        return DatabaseResult(true);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("删除集合失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::list_collections() {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        nlohmann::json collections = nlohmann::json::array();
        collections.push_back("account");
        collections.push_back("history");
        collections.push_back("sim");
        return DatabaseResult(true, collections);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("列出集合失败: ") + std::string(e.what()));
    }
}

DatabaseResult MongoConnector::get_database_stats() {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        nlohmann::json stats = {
            {"collections", 3},
            {"dataSize", 1024000},
            {"indexSize", 512000}
        };
        return DatabaseResult(true, stats);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("获取数据库统计失败: ") + std::string(e.what()));
    }
}

nlohmann::json MongoConnector::bson_to_json(const void* bson_doc) const {
    // 实际实现需要BSON到JSON的转换
    return nlohmann::json();
}

void* MongoConnector::json_to_bson(const nlohmann::json& json_doc) const {
    // 实际实现需要JSON到BSON的转换
    return nullptr;
}

void MongoConnector::cleanup_impl() {
    // 清理MongoDB相关资源
}

// =======================
// ClickHouseConnector 实现
// =======================

ClickHouseConnector::ClickHouseConnector(const DatabaseConfig& config)
    : config_(config), client_impl_(nullptr), connected_(false) {
}

ClickHouseConnector::~ClickHouseConnector() {
    disconnect();
    cleanup_impl();
}

bool ClickHouseConnector::connect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (connected_) {
        return true;
    }

    try {
        std::cout << "连接到ClickHouse: " << config_.uri << std::endl;
        connected_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ClickHouse连接失败: " << e.what() << std::endl;
        return false;
    }
}

bool ClickHouseConnector::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return true;
    }

    try {
        std::cout << "断开ClickHouse连接" << std::endl;
        connected_ = false;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ClickHouse断开连接失败: " << e.what() << std::endl;
        return false;
    }
}

bool ClickHouseConnector::is_connected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}

bool ClickHouseConnector::test_connection() {
    if (!is_connected()) {
        return false;
    }

    try {
        auto result = execute_query("SELECT 1");
        return result.success;
    } catch (const std::exception& e) {
        std::cerr << "ClickHouse连接测试失败: " << e.what() << std::endl;
        return false;
    }
}

DatabaseResult ClickHouseConnector::insert_one(const std::string& collection,
                                              const nlohmann::json& document) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string values = json_to_insert_values(document);
        std::string query = "INSERT INTO " + collection + " VALUES " + values;
        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("插入失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::insert_many(const std::string& collection,
                                               const std::vector<nlohmann::json>& documents) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::ostringstream query_stream;
        query_stream << "INSERT INTO " << collection << " VALUES ";

        for (size_t i = 0; i < documents.size(); ++i) {
            if (i > 0) query_stream << ", ";
            query_stream << json_to_insert_values(documents[i]);
        }

        return execute_query(query_stream.str());
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("批量插入失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::find_one(const std::string& collection,
                                            const nlohmann::json& filter,
                                            const QueryOptions& options) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string where_clause = json_filter_to_sql(filter);
        std::string query = "SELECT * FROM " + collection;
        if (!where_clause.empty()) {
            query += " WHERE " + where_clause;
        }
        query += " LIMIT 1";

        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("查询失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::find_many(const std::string& collection,
                                             const nlohmann::json& filter,
                                             const QueryOptions& options) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string where_clause = json_filter_to_sql(filter);
        std::string query = "SELECT * FROM " + collection;
        if (!where_clause.empty()) {
            query += " WHERE " + where_clause;
        }
        if (options.limit > 0) {
            query += " LIMIT " + std::to_string(options.limit);
        }
        if (options.skip > 0) {
            query += " OFFSET " + std::to_string(options.skip);
        }

        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("查询失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::update_one(const std::string& collection,
                                              const nlohmann::json& filter,
                                              const nlohmann::json& update,
                                              bool upsert) {
    // ClickHouse不支持传统的UPDATE操作，需要使用ALTER TABLE或INSERT新数据
    return DatabaseResult(false, std::string("ClickHouse不支持UPDATE操作，请使用INSERT新数据"));
}

DatabaseResult ClickHouseConnector::update_many(const std::string& collection,
                                               const nlohmann::json& filter,
                                               const nlohmann::json& update) {
    return DatabaseResult(false, std::string("ClickHouse不支持UPDATE操作，请使用INSERT新数据"));
}

DatabaseResult ClickHouseConnector::delete_one(const std::string& collection,
                                              const nlohmann::json& filter) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string where_clause = json_filter_to_sql(filter);
        std::string query = "ALTER TABLE " + collection + " DELETE WHERE " + where_clause;
        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("删除失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::delete_many(const std::string& collection,
                                               const nlohmann::json& filter) {
    return delete_one(collection, filter); // ClickHouse的DELETE会删除所有匹配的行
}

DatabaseResult ClickHouseConnector::aggregate(const std::string& collection,
                                             const std::vector<nlohmann::json>& pipeline) {
    // ClickHouse使用SQL聚合，需要将MongoDB聚合管道转换为SQL
    return DatabaseResult(false, std::string("ClickHouse聚合需要使用原生SQL语法"));
}

DatabaseResult ClickHouseConnector::count_documents(const std::string& collection,
                                                   const nlohmann::json& filter) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string where_clause = json_filter_to_sql(filter);
        std::string query = "SELECT COUNT(*) as count FROM " + collection;
        if (!where_clause.empty()) {
            query += " WHERE " + where_clause;
        }

        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("计数失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::create_index(const std::string& collection,
                                                const nlohmann::json& keys,
                                                const nlohmann::json& options) {
    // ClickHouse索引通过表引擎创建，不支持后期添加索引
    return DatabaseResult(false, std::string("ClickHouse索引需要在创建表时指定"));
}

DatabaseResult ClickHouseConnector::list_indexes(const std::string& collection) {
    return DatabaseResult(false, std::string("ClickHouse不支持列出索引操作"));
}

DatabaseResult ClickHouseConnector::drop_index(const std::string& collection,
                                              const std::string& index_name) {
    return DatabaseResult(false, std::string("ClickHouse不支持删除索引操作"));
}

DatabaseResult ClickHouseConnector::bulk_write(const std::string& collection,
                                              const std::vector<nlohmann::json>& operations) {
    // 简化实现：只处理插入操作
    std::vector<nlohmann::json> documents;
    for (const auto& op : operations) {
        if (op.contains("insertOne") && op["insertOne"].contains("document")) {
            documents.push_back(op["insertOne"]["document"]);
        }
    }

    if (!documents.empty()) {
        return insert_many(collection, documents);
    }

    return DatabaseResult(true);
}

DatabaseResult ClickHouseConnector::begin_transaction() {
    return DatabaseResult(false, std::string("ClickHouse不支持事务"));
}

DatabaseResult ClickHouseConnector::commit_transaction() {
    return DatabaseResult(false, std::string("ClickHouse不支持事务"));
}

DatabaseResult ClickHouseConnector::abort_transaction() {
    return DatabaseResult(false, std::string("ClickHouse不支持事务"));
}

std::string ClickHouseConnector::get_connection_string() const {
    return config_.uri + "/" + config_.database_name;
}

DatabaseResult ClickHouseConnector::execute_query(const std::string& query) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::cout << "执行ClickHouse查询: " << query << std::endl;

        // 模拟查询结果
        nlohmann::json result = nlohmann::json::array();
        if (query.find("SELECT") != std::string::npos) {
            if (query.find("COUNT") != std::string::npos) {
                result.push_back({{"count", 100}});
            } else {
                result.push_back({
                    {"id", 1},
                    {"account_cookie", "test_account"},
                    {"timestamp", "2024-01-01 00:00:00"}
                });
            }
        }

        return DatabaseResult(true, result);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("查询执行失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::create_table(const std::string& table_name,
                                                const std::string& schema) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string query = "CREATE TABLE " + table_name + " " + schema;
        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("创建表失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::drop_table(const std::string& table_name) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string query = "DROP TABLE " + table_name;
        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("删除表失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::get_table_schema(const std::string& table_name) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string query = "DESCRIBE TABLE " + table_name;
        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("获取表结构失败: ") + std::string(e.what()));
    }
}

DatabaseResult ClickHouseConnector::optimize_table(const std::string& table_name) {
    if (!is_connected()) {
        return DatabaseResult(false, std::string("未连接到数据库"));
    }

    try {
        std::string query = "OPTIMIZE TABLE " + table_name;
        return execute_query(query);
    } catch (const std::exception& e) {
        return DatabaseResult(false, std::string("优化表失败: ") + std::string(e.what()));
    }
}

std::string ClickHouseConnector::json_filter_to_sql(const nlohmann::json& filter) const {
    if (filter.empty()) {
        return "";
    }

    std::vector<std::string> conditions;
    for (const auto& [key, value] : filter.items()) {
        if (value.is_string()) {
            conditions.push_back(key + " = '" + value.get<std::string>() + "'");
        } else if (value.is_number()) {
            conditions.push_back(key + " = " + std::to_string(value.get<double>()));
        }
    }

    std::string result;
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (i > 0) result += " AND ";
        result += conditions[i];
    }

    return result;
}

std::string ClickHouseConnector::json_to_insert_values(const nlohmann::json& document) const {
    std::vector<std::string> values;
    for (const auto& [key, value] : document.items()) {
        if (value.is_string()) {
            values.push_back("'" + value.get<std::string>() + "'");
        } else if (value.is_number()) {
            values.push_back(std::to_string(value.get<double>()));
        } else {
            values.push_back("'" + value.dump() + "'");
        }
    }

    return "(" + std::accumulate(values.begin(), values.end(), std::string(),
                                [](const std::string& a, const std::string& b) {
                                    return a.empty() ? b : a + ", " + b;
                                }) + ")";
}

void ClickHouseConnector::cleanup_impl() {
    // 清理ClickHouse相关资源
}

// =======================
// ConnectorFactory 实现
// =======================

std::unique_ptr<IDatabaseConnector> ConnectorFactory::create_connector(
    DatabaseType type, const DatabaseConfig& config) {

    switch (type) {
        case DatabaseType::MongoDB:
            return std::make_unique<MongoConnector>(config);
        case DatabaseType::ClickHouse:
            return std::make_unique<ClickHouseConnector>(config);
        default:
            throw std::runtime_error("不支持的数据库类型");
    }
}

std::unique_ptr<MongoConnector> ConnectorFactory::create_mongo_connector(
    const DatabaseConfig& config) {
    return std::make_unique<MongoConnector>(config);
}

std::unique_ptr<ClickHouseConnector> ConnectorFactory::create_clickhouse_connector(
    const DatabaseConfig& config) {
    return std::make_unique<ClickHouseConnector>(config);
}

DatabaseConfig ConnectorFactory::create_mongo_config(
    const std::string& uri, const std::string& database) {
    DatabaseConfig config;
    config.uri = uri;
    config.database_name = database;
    config.timeout_seconds = 30;
    config.max_connections = 10;
    return config;
}

DatabaseConfig ConnectorFactory::create_clickhouse_config(
    const std::string& host, int port, const std::string& database,
    const std::string& username, const std::string& password) {
    DatabaseConfig config;
    config.uri = "tcp://" + host + ":" + std::to_string(port);
    config.database_name = database;
    config.username = username;
    config.password = password;
    config.timeout_seconds = 30;
    config.max_connections = 10;
    return config;
}

// =======================
// ConnectionPool 实现
// =======================

ConnectionPool::ConnectionPool(DatabaseType type, const DatabaseConfig& config, int max_connections)
    : type_(type), config_(config), max_connections_(max_connections) {
    initialize_pool();
}

ConnectionPool::~ConnectionPool() {
    clear_pool();
}

void ConnectionPool::initialize_pool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    connections_.reserve(max_connections_);
    connection_status_.reserve(max_connections_);

    for (int i = 0; i < max_connections_; ++i) {
        auto conn = create_new_connection();
        if (conn && conn->connect()) {
            connections_.push_back(conn);
            connection_status_.push_back(false); // available
        }
    }
}

std::shared_ptr<IDatabaseConnector> ConnectionPool::create_new_connection() {
    return ConnectorFactory::create_connector(type_, config_);
}

std::shared_ptr<IDatabaseConnector> ConnectionPool::get_connection() {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    // 查找可用连接
    for (size_t i = 0; i < connections_.size(); ++i) {
        if (!connection_status_[i]) {
            connection_status_[i] = true; // mark as in use
            return connections_[i];
        }
    }

    // 如果没有可用连接，创建新的（如果未达到最大限制）
    if (connections_.size() < static_cast<size_t>(max_connections_)) {
        auto conn = create_new_connection();
        if (conn && conn->connect()) {
            connections_.push_back(conn);
            connection_status_.push_back(true); // mark as in use
            return conn;
        }
    }

    return nullptr; // 无可用连接
}

void ConnectionPool::return_connection(std::shared_ptr<IDatabaseConnector> conn) {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    // 查找并标记为可用
    for (size_t i = 0; i < connections_.size(); ++i) {
        if (connections_[i] == conn) {
            connection_status_[i] = false; // mark as available
            break;
        }
    }
}

int ConnectionPool::get_active_connections() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return std::count(connection_status_.begin(), connection_status_.end(), true);
}

int ConnectionPool::get_available_connections() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return std::count(connection_status_.begin(), connection_status_.end(), false);
}

int ConnectionPool::get_total_connections() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return connections_.size();
}

void ConnectionPool::resize_pool(int new_size) {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    if (new_size < static_cast<int>(connections_.size())) {
        // 缩小连接池 - 移除多余连接
        connections_.resize(new_size);
        connection_status_.resize(new_size);
    }

    max_connections_ = new_size;
}

void ConnectionPool::clear_pool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    for (auto& conn : connections_) {
        if (conn) {
            conn->disconnect();
        }
    }

    connections_.clear();
    connection_status_.clear();
}

} // namespace qaultra::connector