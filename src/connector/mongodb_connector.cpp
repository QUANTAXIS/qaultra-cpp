#include "../../include/qaultra/connector/mongodb_connector.hpp"
#include <mongocxx/uri.hpp>
#include <mongocxx/options/update.hpp>
#include <mongocxx/options/find.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <iostream>
#include <sstream>
#include <chrono>

namespace qaultra::connector {

// 静态成员初始化
mongocxx::instance MongoConnector::instance_{};

// ============================================================================
// MongoConnector 实现
// ============================================================================

MongoConnector::MongoConnector(const MongoConfig& config)
    : config_(config), connected_(false) {
}

MongoConnector::~MongoConnector() {
    disconnect();
}

std::string MongoConnector::build_connection_string() const {
    std::stringstream ss;
    ss << "mongodb://";

    // 添加认证信息
    if (!config_.username.empty()) {
        ss << config_.username;
        if (!config_.password.empty()) {
            ss << ":" << config_.password;
        }
        ss << "@";
    }

    // 添加主机和端口
    ss << config_.host << ":" << config_.port;

    // 添加数据库和认证源
    ss << "/" << config_.database;
    if (!config_.auth_source.empty() && !config_.username.empty()) {
        ss << "?authSource=" << config_.auth_source;
    }

    // 添加连接池配置
    if (config_.max_pool_size > 0) {
        if (ss.str().find('?') == std::string::npos) {
            ss << "?";
        } else {
            ss << "&";
        }
        ss << "maxPoolSize=" << config_.max_pool_size;
    }

    return ss.str();
}

bool MongoConnector::connect() {
    try {
        std::string connection_string = build_connection_string();

        // 创建客户端
        mongocxx::uri uri{connection_string};
        client_ = std::make_unique<mongocxx::client>(uri);

        // 获取数据库
        db_ = std::make_unique<mongocxx::database>((*client_)[config_.database]);

        // 测试连接
        if (!test_connection()) {
            std::cerr << "MongoDB connection test failed" << std::endl;
            return false;
        }

        connected_ = true;
        std::cout << "✓ MongoDB connected successfully to " << config_.host
                  << ":" << config_.port << "/" << config_.database << std::endl;
        return true;

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "connect");
        connected_ = false;
        return false;
    }
}

void MongoConnector::disconnect() {
    if (connected_) {
        client_.reset();
        db_.reset();
        connected_ = false;
        std::cout << "MongoDB disconnected" << std::endl;
    }
}

bool MongoConnector::is_connected() const {
    return connected_ && validate_connection();
}

bool MongoConnector::test_connection() {
    try {
        if (!client_) return false;

        // 执行 ping 命令测试连接
        auto admin_db = (*client_)["admin"];
        auto result = admin_db.run_command(
            bsoncxx::builder::stream::document{} << "ping" << 1
            << bsoncxx::builder::stream::finalize
        );

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Connection test failed: " << e.what() << std::endl;
        return false;
    }
}

bool MongoConnector::validate_connection() const {
    if (!client_ || !db_) {
        return false;
    }

    // 简单验证：尝试列出集合
    try {
        auto collections = db_->list_collections();
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// 账户数据操作
// ============================================================================

bsoncxx::document::value MongoConnector::qifi_account_to_bson(
    const protocol::QIFIAccount& account) const {

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::open_array;
    using bsoncxx::builder::stream::close_array;
    using bsoncxx::builder::stream::finalize;

    auto builder = document{};

    // 基本信息
    builder << "account_cookie" << account.account_cookie
            << "portfolio" << account.portfolio
            << "user_id" << account.user_id
            << "currency" << account.currency;

    // 资金信息
    builder << "pre_balance" << account.pre_balance
            << "deposit" << account.deposit
            << "withdraw" << account.withdraw
            << "close_profit" << account.close_profit
            << "commission" << account.commission
            << "premium" << account.premium
            << "static_balance" << account.static_balance
            << "position_profit" << account.position_profit
            << "float_profit" << account.float_profit
            << "balance" << account.balance
            << "margin" << account.margin
            << "frozen_margin" << account.frozen_margin
            << "frozen_commission" << account.frozen_commission
            << "frozen_premium" << account.frozen_premium
            << "available" << account.available
            << "risk_ratio" << account.risk_ratio;

    // 时间信息
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);

    builder << "updatetime" << std::string(buffer);

    return builder << finalize;
}

protocol::QIFIAccount MongoConnector::bson_to_qifi_account(
    const bsoncxx::document::view& doc) const {

    protocol::QIFIAccount account;

    // 基本信息
    if (doc["account_cookie"]) account.account_cookie = doc["account_cookie"].get_string().value.to_string();
    if (doc["portfolio"]) account.portfolio = doc["portfolio"].get_string().value.to_string();
    if (doc["user_id"]) account.user_id = doc["user_id"].get_string().value.to_string();
    if (doc["currency"]) account.currency = doc["currency"].get_string().value.to_string();

    // 资金信息
    if (doc["pre_balance"]) account.pre_balance = doc["pre_balance"].get_double().value;
    if (doc["deposit"]) account.deposit = doc["deposit"].get_double().value;
    if (doc["withdraw"]) account.withdraw = doc["withdraw"].get_double().value;
    if (doc["close_profit"]) account.close_profit = doc["close_profit"].get_double().value;
    if (doc["commission"]) account.commission = doc["commission"].get_double().value;
    if (doc["premium"]) account.premium = doc["premium"].get_double().value;
    if (doc["static_balance"]) account.static_balance = doc["static_balance"].get_double().value;
    if (doc["position_profit"]) account.position_profit = doc["position_profit"].get_double().value;
    if (doc["float_profit"]) account.float_profit = doc["float_profit"].get_double().value;
    if (doc["balance"]) account.balance = doc["balance"].get_double().value;
    if (doc["margin"]) account.margin = doc["margin"].get_double().value;
    if (doc["frozen_margin"]) account.frozen_margin = doc["frozen_margin"].get_double().value;
    if (doc["frozen_commission"]) account.frozen_commission = doc["frozen_commission"].get_double().value;
    if (doc["frozen_premium"]) account.frozen_premium = doc["frozen_premium"].get_double().value;
    if (doc["available"]) account.available = doc["available"].get_double().value;
    if (doc["risk_ratio"]) account.risk_ratio = doc["risk_ratio"].get_double().value;

    return account;
}

bool MongoConnector::save_account(const protocol::QIFIAccount& account) {
    if (!validate_connection()) {
        std::cerr << "Not connected to MongoDB" << std::endl;
        return false;
    }

    try {
        auto collection = (*db_)["account"];
        auto doc = qifi_account_to_bson(account);

        // Upsert: 存在则更新，不存在则插入
        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        auto filter = document{} << "account_cookie" << account.account_cookie << finalize;

        mongocxx::options::update options;
        options.upsert(true);

        auto result = collection.update_one(filter.view(),
                                           document{} << "$set" << doc << finalize,
                                           options);

        return result && (result->modified_count() > 0 || result->upserted_id());

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "save_account");
        return false;
    }
}

std::optional<protocol::QIFIAccount> MongoConnector::load_account(
    const std::string& account_id) {

    if (!validate_connection()) {
        return std::nullopt;
    }

    try {
        auto collection = (*db_)["account"];

        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        auto filter = document{} << "account_cookie" << account_id << finalize;
        auto result = collection.find_one(filter.view());

        if (result) {
            return bson_to_qifi_account(result->view());
        }

        return std::nullopt;

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "load_account");
        return std::nullopt;
    }
}

bool MongoConnector::update_account(const protocol::QIFIAccount& account) {
    return save_account(account);  // Upsert 逻辑已经包含更新
}

bool MongoConnector::delete_account(const std::string& account_id) {
    if (!validate_connection()) {
        return false;
    }

    try {
        auto collection = (*db_)["account"];

        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        auto filter = document{} << "account_cookie" << account_id << finalize;
        auto result = collection.delete_one(filter.view());

        return result && result->deleted_count() > 0;

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "delete_account");
        return false;
    }
}

std::vector<std::string> MongoConnector::get_account_list() {
    std::vector<std::string> accounts;

    if (!validate_connection()) {
        return accounts;
    }

    try {
        auto collection = (*db_)["account"];

        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        // 只查询 account_cookie 字段
        mongocxx::options::find options;
        options.projection(document{} << "account_cookie" << 1 << finalize);

        auto cursor = collection.find({}, options);

        for (auto&& doc : cursor) {
            if (doc["account_cookie"]) {
                accounts.push_back(doc["account_cookie"].get_string().value.to_string());
            }
        }

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "get_account_list");
    }

    return accounts;
}

// ============================================================================
// 数据库管理
// ============================================================================

bool MongoConnector::create_index(const std::string& collection_name,
                                  const std::map<std::string, int>& keys) {
    if (!validate_connection()) {
        return false;
    }

    try {
        auto collection = (*db_)[collection_name];

        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        auto index_builder = document{};
        for (const auto& [key, order] : keys) {
            index_builder << key << order;
        }

        collection.create_index(index_builder << finalize);
        return true;

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "create_index");
        return false;
    }
}

bool MongoConnector::create_compound_index(const std::string& collection_name,
                                           const std::vector<std::pair<std::string, int>>& keys) {
    if (!validate_connection()) {
        return false;
    }

    try {
        auto collection = (*db_)[collection_name];

        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        auto index_builder = document{};
        for (const auto& [key, order] : keys) {
            index_builder << key << order;
        }

        collection.create_index(index_builder << finalize);
        return true;

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "create_compound_index");
        return false;
    }
}

std::map<std::string, int64_t> MongoConnector::get_collection_stats(
    const std::string& collection_name) {

    std::map<std::string, int64_t> stats;

    if (!validate_connection()) {
        return stats;
    }

    try {
        auto collection = (*db_)[collection_name];

        // 获取文档数量
        stats["count"] = collection.count_documents({});

        // 可以添加更多统计信息
        // TODO: 实现更详细的统计

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "get_collection_stats");
    }

    return stats;
}

bool MongoConnector::clear_collection(const std::string& collection_name) {
    if (!validate_connection()) {
        return false;
    }

    try {
        auto collection = (*db_)[collection_name];
        auto result = collection.delete_many({});
        return result && result->deleted_count() >= 0;

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "clear_collection");
        return false;
    }
}

bool MongoConnector::drop_collection(const std::string& collection_name) {
    if (!validate_connection()) {
        return false;
    }

    try {
        auto collection = (*db_)[collection_name];
        collection.drop();
        return true;

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "drop_collection");
        return false;
    }
}

// ============================================================================
// 聚合查询
// ============================================================================

std::vector<std::string> MongoConnector::get_stock_list() {
    std::vector<std::string> stocks;

    if (!validate_connection()) {
        return stocks;
    }

    try {
        auto collection = (*db_)["stock_day"];

        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        // 使用聚合管道获取不重复的股票代码
        mongocxx::pipeline pipeline;
        pipeline.group(document{} << "_id" << "$code" << finalize);

        auto cursor = collection.aggregate(pipeline);

        for (auto&& doc : cursor) {
            if (doc["_id"]) {
                stocks.push_back(doc["_id"].get_string().value.to_string());
            }
        }

    } catch (const std::exception& e) {
        handle_mongodb_exception(e, "get_stock_list");
    }

    return stocks;
}

// ============================================================================
// 错误处理
// ============================================================================

void MongoConnector::handle_mongodb_exception(const std::exception& e,
                                              const std::string& operation) const {
    std::cerr << "MongoDB error in " << operation << ": " << e.what() << std::endl;
}

// ============================================================================
// MongoFactory 实现
// ============================================================================

std::unique_ptr<MongoConnector> MongoFactory::create_default_connector() {
    MongoConfig config;
    // 使用默认配置
    return std::make_unique<MongoConnector>(config);
}

std::unique_ptr<MongoConnector> MongoFactory::create_connector(const MongoConfig& config) {
    return std::make_unique<MongoConnector>(config);
}

std::unique_ptr<MongoConnector> MongoFactory::create_cluster_connector(
    const std::vector<std::string>& hosts, const MongoConfig& base_config) {

    // 构建集群连接字符串
    MongoConfig cluster_config = base_config;

    std::stringstream ss;
    for (size_t i = 0; i < hosts.size(); ++i) {
        if (i > 0) ss << ",";
        ss << hosts[i];
    }
    cluster_config.host = ss.str();

    return std::make_unique<MongoConnector>(cluster_config);
}

// ============================================================================
// mongo_utils 实现
// ============================================================================

namespace mongo_utils {

bool validate_connection_string(const std::string& connection_string) {
    // 简单验证连接字符串格式
    return connection_string.find("mongodb://") == 0 ||
           connection_string.find("mongodb+srv://") == 0;
}

bool create_standard_indexes(MongoConnector& connector) {
    bool success = true;

    // 为账户集合创建索引
    success &= connector.create_index("account", {{"account_cookie", 1}});
    success &= connector.create_index("account", {{"portfolio", 1}});

    // 为 K 线集合创建复合索引
    success &= connector.create_compound_index("stock_day",
        {{"code", 1}, {"date", -1}});

    return success;
}

} // namespace mongo_utils

} // namespace qaultra::connector
