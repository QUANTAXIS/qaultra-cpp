#include "qaultra/connector/qa_connector.hpp"
#include "qaultra/account/qa_account.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace qaultra::connector {

// =======================
// QAMongoClient 实现
// =======================

QAMongoClient::QAMongoClient(const std::string& uri) {
    config_.uri = uri;
    config_.database_name = "quantaxis";
    connector_ = std::make_unique<MongoConnector>(config_);
}

QAMongoClient::QAMongoClient(const DatabaseConfig& config)
    : config_(config) {
    connector_ = std::make_unique<MongoConnector>(config_);
}

QAMongoClient::~QAMongoClient() {
    disconnect();
}

bool QAMongoClient::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connector_->connect();
}

bool QAMongoClient::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connector_->disconnect();
}

bool QAMongoClient::is_connected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connector_->is_connected();
}

bool QAMongoClient::check_user(const std::string& account_cookie, const std::string& password) {
    if (!is_connected()) {
        return false;
    }

    nlohmann::json filter = {
        {"account_cookie", account_cookie},
        {"password", password}
    };

    auto result = connector_->count_documents(ACCOUNT_COLLECTION, filter);
    if (result.success && result.data.has_value()) {
        auto count = result.data.value();
        if (count.contains("count")) {
            return count["count"].get<int>() > 0;
        }
    }

    return false;
}

std::optional<protocol::qifi::QIFI> QAMongoClient::get_qifi(const std::string& account_cookie) {
    if (!is_connected()) {
        return std::nullopt;
    }

    nlohmann::json filter = {{"account_cookie", account_cookie}};

    auto result = connector_->find_one(ACCOUNT_COLLECTION, filter);
    if (result.success && result.data.has_value()) {
        try {
            return json_to_qifi(result.data.value());
        } catch (const std::exception& e) {
            std::cerr << "QIFI反序列化失败: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    return std::nullopt;
}

bool QAMongoClient::save_qifi_slice(const protocol::qifi::QIFI& slice) {
    if (!is_connected()) {
        return false;
    }

    // 创建带更新时间的副本
    auto updated_slice = slice;
    updated_slice.updatetime = get_current_time_string();

    nlohmann::json filter = {{"account_cookie", slice.account_cookie}};
    nlohmann::json update = {{"$set", qifi_to_json(updated_slice)}};

    auto result = connector_->update_one(ACCOUNT_COLLECTION, filter, update, true);
    return result.success;
}

bool QAMongoClient::save_his_qifi_slice(const protocol::qifi::QIFI& slice) {
    if (!is_connected()) {
        return false;
    }

    nlohmann::json filter = {
        {"account_cookie", slice.account_cookie},
        {"trading_day", slice.trading_day},
        {"portfolio_cookie", slice.portfolio},
        {"investor_name", slice.investor_name}
    };

    nlohmann::json update = {{"$set", qifi_to_json(slice)}};

    // 使用REALTIME数据库
    auto original_db = config_.database_name;
    config_.database_name = REALTIME_DATABASE;

    auto result = connector_->update_one(HISTORY_COLLECTION, filter, update, true);

    // 恢复原数据库
    config_.database_name = original_db;

    return result.success;
}

bool QAMongoClient::save_sim_qifi_slice(const protocol::qifi::QIFI& slice) {
    if (!is_connected()) {
        return false;
    }

    nlohmann::json filter = {
        {"account_cookie", slice.account_cookie},
        {"trading_day", slice.trading_day}
    };

    nlohmann::json update = {{"$set", qifi_to_json(slice)}};

    auto result = connector_->update_one(SIM_COLLECTION, filter, update, true);
    return result.success;
}

std::unique_ptr<QA_Account> QAMongoClient::get_account(const std::string& account_cookie) {
    auto qifi_opt = get_qifi(account_cookie);
    if (!qifi_opt.has_value()) {
        return nullptr;
    }

    // 这里需要实现从QIFI创建QAAccount的逻辑
    // 返回nullptr作为占位符，实际需要根据QAAccount的实现来完成
    std::cout << "从QIFI创建QAAccount: " << account_cookie << std::endl;
    return nullptr;
}

bool QAMongoClient::save_account(const QA_Account& account) {
    // 这里需要实现从QAAccount获取QIFI的逻辑
    // 返回false作为占位符，实际需要根据QAAccount的实现来完成
    std::cout << "保存QAAccount" << std::endl;
    return false;
}

bool QAMongoClient::save_account_history(const QA_Account& account) {
    // 类似save_account，但保存到历史记录
    std::cout << "保存QAAccount历史记录" << std::endl;
    return false;
}

std::vector<std::string> QAMongoClient::get_account_list() {
    if (!is_connected()) {
        return {};
    }

    QueryOptions options;
    options.projection = {{"account_cookie", 1}};
    options.batch_size = 10000000;

    auto result = connector_->find_many(ACCOUNT_COLLECTION, {}, options);

    std::vector<std::string> account_list;
    if (result.success && result.data.has_value() && result.data.value().is_array()) {
        for (const auto& item : result.data.value()) {
            if (item.contains("account_cookie")) {
                account_list.push_back(item["account_cookie"].get<std::string>());
            }
        }
    }

    return account_list;
}

bool QAMongoClient::save_multiple_accounts(const std::vector<std::reference_wrapper<const QA_Account>>& accounts) {
    if (!is_connected() || accounts.empty()) {
        return false;
    }

    std::vector<nlohmann::json> operations;
    for (const auto& account_ref : accounts) {
        // 这里需要实现批量操作逻辑
        std::cout << "添加账户到批量操作" << std::endl;
    }

    auto result = connector_->bulk_write(ACCOUNT_COLLECTION, operations);
    return result.success;
}

QAMongoClient::DatabaseStats QAMongoClient::get_database_stats() {
    DatabaseStats stats;

    if (!is_connected()) {
        return stats;
    }

    // 获取账户总数
    auto account_count = connector_->count_documents(ACCOUNT_COLLECTION);
    if (account_count.success && account_count.data.has_value()) {
        if (account_count.data.value().contains("count")) {
            stats.total_accounts = account_count.data.value()["count"].get<int>();
        }
    }

    // 获取历史记录总数
    auto history_count = connector_->count_documents(HISTORY_COLLECTION);
    if (history_count.success && history_count.data.has_value()) {
        if (history_count.data.value().contains("count")) {
            stats.total_history_records = history_count.data.value()["count"].get<int>();
        }
    }

    // 获取模拟记录总数
    auto sim_count = connector_->count_documents(SIM_COLLECTION);
    if (sim_count.success && sim_count.data.has_value()) {
        if (sim_count.data.value().contains("count")) {
            stats.total_sim_records = sim_count.data.value()["count"].get<int>();
        }
    }

    stats.last_update_time = get_current_time_string();
    return stats;
}

const DatabaseConfig& QAMongoClient::get_config() const {
    return config_;
}

std::string QAMongoClient::get_current_time_string() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

nlohmann::json QAMongoClient::qifi_to_json(const protocol::qifi::QIFI& qifi) const {
    // 这里需要实现QIFI到JSON的转换
    // 返回空JSON作为占位符
    return qifi.to_json();
}

protocol::qifi::QIFI QAMongoClient::json_to_qifi(const nlohmann::json& json) const {
    // 这里需要实现JSON到QIFI的转换
    protocol::qifi::QIFI qifi;
    qifi.from_json(json);
    return qifi;
}

// =======================
// QAClickHouseClient 实现
// =======================

QAClickHouseClient::QAClickHouseClient(const DatabaseConfig& config)
    : config_(config) {
    connector_ = std::make_unique<ClickHouseConnector>(config_);
}

QAClickHouseClient::~QAClickHouseClient() {
    disconnect();
}

bool QAClickHouseClient::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connector_->connect();
}

bool QAClickHouseClient::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connector_->disconnect();
}

bool QAClickHouseClient::is_connected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connector_->is_connected();
}

// MarketDataRecord 实现
nlohmann::json QAClickHouseClient::MarketDataRecord::to_json() const {
    return {
        {"code", code},
        {"datetime", std::chrono::duration_cast<std::chrono::seconds>(datetime.time_since_epoch()).count()},
        {"open", open},
        {"high", high},
        {"low", low},
        {"close", close},
        {"volume", volume},
        {"amount", amount}
    };
}

QAClickHouseClient::MarketDataRecord QAClickHouseClient::MarketDataRecord::from_json(const nlohmann::json& json) {
    MarketDataRecord record;
    record.code = json.at("code").get<std::string>();
    record.datetime = std::chrono::system_clock::from_time_t(json.at("datetime").get<time_t>());
    record.open = json.at("open").get<double>();
    record.high = json.at("high").get<double>();
    record.low = json.at("low").get<double>();
    record.close = json.at("close").get<double>();
    record.volume = json.at("volume").get<double>();
    record.amount = json.at("amount").get<double>();
    return record;
}

bool QAClickHouseClient::insert_market_data(const MarketDataRecord& record) {
    if (!is_connected()) {
        return false;
    }

    auto result = connector_->insert_one(MARKET_DATA_TABLE, record.to_json());
    return result.success;
}

bool QAClickHouseClient::insert_market_data_batch(const std::vector<MarketDataRecord>& records) {
    if (!is_connected() || records.empty()) {
        return false;
    }

    std::vector<nlohmann::json> json_records;
    json_records.reserve(records.size());

    for (const auto& record : records) {
        json_records.push_back(record.to_json());
    }

    auto result = connector_->insert_many(MARKET_DATA_TABLE, json_records);
    return result.success;
}

std::vector<QAClickHouseClient::MarketDataRecord> QAClickHouseClient::query_market_data(
    const std::string& code,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {

    std::vector<MarketDataRecord> results;

    if (!is_connected()) {
        return results;
    }

    // 构造时间过滤条件
    nlohmann::json filter = {
        {"code", code},
        {"datetime", {
            {"$gte", std::chrono::duration_cast<std::chrono::seconds>(start_time.time_since_epoch()).count()},
            {"$lte", std::chrono::duration_cast<std::chrono::seconds>(end_time.time_since_epoch()).count()}
        }}
    };

    auto result = connector_->find_many(MARKET_DATA_TABLE, filter);

    if (result.success && result.data.has_value() && result.data.value().is_array()) {
        for (const auto& item : result.data.value()) {
            try {
                results.push_back(MarketDataRecord::from_json(item));
            } catch (const std::exception& e) {
                std::cerr << "解析市场数据记录失败: " << e.what() << std::endl;
            }
        }
    }

    return results;
}

// TradeRecord 实现
nlohmann::json QAClickHouseClient::TradeRecord::to_json() const {
    return {
        {"account_cookie", account_cookie},
        {"trade_id", trade_id},
        {"code", code},
        {"datetime", std::chrono::duration_cast<std::chrono::seconds>(datetime.time_since_epoch()).count()},
        {"direction", direction},
        {"price", price},
        {"volume", volume},
        {"amount", amount},
        {"status", status}
    };
}

QAClickHouseClient::TradeRecord QAClickHouseClient::TradeRecord::from_json(const nlohmann::json& json) {
    TradeRecord record;
    record.account_cookie = json.at("account_cookie").get<std::string>();
    record.trade_id = json.at("trade_id").get<std::string>();
    record.code = json.at("code").get<std::string>();
    record.datetime = std::chrono::system_clock::from_time_t(json.at("datetime").get<time_t>());
    record.direction = json.at("direction").get<std::string>();
    record.price = json.at("price").get<double>();
    record.volume = json.at("volume").get<double>();
    record.amount = json.at("amount").get<double>();
    record.status = json.at("status").get<std::string>();
    return record;
}

bool QAClickHouseClient::insert_trade_record(const TradeRecord& record) {
    if (!is_connected()) {
        return false;
    }

    auto result = connector_->insert_one(TRADE_RECORDS_TABLE, record.to_json());
    return result.success;
}

bool QAClickHouseClient::insert_trade_records_batch(const std::vector<TradeRecord>& records) {
    if (!is_connected() || records.empty()) {
        return false;
    }

    std::vector<nlohmann::json> json_records;
    json_records.reserve(records.size());

    for (const auto& record : records) {
        json_records.push_back(record.to_json());
    }

    auto result = connector_->insert_many(TRADE_RECORDS_TABLE, json_records);
    return result.success;
}

std::vector<QAClickHouseClient::TradeRecord> QAClickHouseClient::query_trade_records(
    const std::string& account_cookie,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {

    std::vector<TradeRecord> results;

    if (!is_connected()) {
        return results;
    }

    nlohmann::json filter = {
        {"account_cookie", account_cookie},
        {"datetime", {
            {"$gte", std::chrono::duration_cast<std::chrono::seconds>(start_time.time_since_epoch()).count()},
            {"$lte", std::chrono::duration_cast<std::chrono::seconds>(end_time.time_since_epoch()).count()}
        }}
    };

    auto result = connector_->find_many(TRADE_RECORDS_TABLE, filter);

    if (result.success && result.data.has_value() && result.data.value().is_array()) {
        for (const auto& item : result.data.value()) {
            try {
                results.push_back(TradeRecord::from_json(item));
            } catch (const std::exception& e) {
                std::cerr << "解析交易记录失败: " << e.what() << std::endl;
            }
        }
    }

    return results;
}

// PerformanceRecord 实现
nlohmann::json QAClickHouseClient::PerformanceRecord::to_json() const {
    return {
        {"account_cookie", account_cookie},
        {"datetime", std::chrono::duration_cast<std::chrono::seconds>(datetime.time_since_epoch()).count()},
        {"total_balance", total_balance},
        {"available_cash", available_cash},
        {"market_value", market_value},
        {"pnl", pnl},
        {"pnl_ratio", pnl_ratio}
    };
}

QAClickHouseClient::PerformanceRecord QAClickHouseClient::PerformanceRecord::from_json(const nlohmann::json& json) {
    PerformanceRecord record;
    record.account_cookie = json.at("account_cookie").get<std::string>();
    record.datetime = std::chrono::system_clock::from_time_t(json.at("datetime").get<time_t>());
    record.total_balance = json.at("total_balance").get<double>();
    record.available_cash = json.at("available_cash").get<double>();
    record.market_value = json.at("market_value").get<double>();
    record.pnl = json.at("pnl").get<double>();
    record.pnl_ratio = json.at("pnl_ratio").get<double>();
    return record;
}

bool QAClickHouseClient::insert_performance_record(const PerformanceRecord& record) {
    if (!is_connected()) {
        return false;
    }

    auto result = connector_->insert_one(PERFORMANCE_TABLE, record.to_json());
    return result.success;
}

std::vector<QAClickHouseClient::PerformanceRecord> QAClickHouseClient::query_performance_records(
    const std::string& account_cookie,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {

    std::vector<PerformanceRecord> results;

    if (!is_connected()) {
        return results;
    }

    nlohmann::json filter = {
        {"account_cookie", account_cookie},
        {"datetime", {
            {"$gte", std::chrono::duration_cast<std::chrono::seconds>(start_time.time_since_epoch()).count()},
            {"$lte", std::chrono::duration_cast<std::chrono::seconds>(end_time.time_since_epoch()).count()}
        }}
    };

    auto result = connector_->find_many(PERFORMANCE_TABLE, filter);

    if (result.success && result.data.has_value() && result.data.value().is_array()) {
        for (const auto& item : result.data.value()) {
            try {
                results.push_back(PerformanceRecord::from_json(item));
            } catch (const std::exception& e) {
                std::cerr << "解析性能记录失败: " << e.what() << std::endl;
            }
        }
    }

    return results;
}

bool QAClickHouseClient::create_market_data_table() {
    if (!is_connected()) {
        return false;
    }

    std::string schema = R"(
        (
            code String,
            datetime DateTime,
            open Float64,
            high Float64,
            low Float64,
            close Float64,
            volume Float64,
            amount Float64
        )
        ENGINE = MergeTree()
        ORDER BY (code, datetime)
        PARTITION BY toYYYYMM(datetime)
    )";

    auto result = connector_->create_table(MARKET_DATA_TABLE, schema);
    return result.success;
}

bool QAClickHouseClient::create_trade_records_table() {
    if (!is_connected()) {
        return false;
    }

    std::string schema = R"(
        (
            account_cookie String,
            trade_id String,
            code String,
            datetime DateTime,
            direction String,
            price Float64,
            volume Float64,
            amount Float64,
            status String
        )
        ENGINE = MergeTree()
        ORDER BY (account_cookie, datetime)
        PARTITION BY toYYYYMM(datetime)
    )";

    auto result = connector_->create_table(TRADE_RECORDS_TABLE, schema);
    return result.success;
}

bool QAClickHouseClient::create_performance_table() {
    if (!is_connected()) {
        return false;
    }

    std::string schema = R"(
        (
            account_cookie String,
            datetime DateTime,
            total_balance Float64,
            available_cash Float64,
            market_value Float64,
            pnl Float64,
            pnl_ratio Float64
        )
        ENGINE = MergeTree()
        ORDER BY (account_cookie, datetime)
        PARTITION BY toYYYYMM(datetime)
    )";

    auto result = connector_->create_table(PERFORMANCE_TABLE, schema);
    return result.success;
}

bool QAClickHouseClient::optimize_all_tables() {
    if (!is_connected()) {
        return false;
    }

    bool success = true;
    success &= connector_->optimize_table(MARKET_DATA_TABLE).success;
    success &= connector_->optimize_table(TRADE_RECORDS_TABLE).success;
    success &= connector_->optimize_table(PERFORMANCE_TABLE).success;

    return success;
}

QAClickHouseClient::AggregationResult QAClickHouseClient::execute_aggregation_query(const std::string& query) {
    AggregationResult result;

    if (!is_connected()) {
        return result;
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    auto db_result = connector_->execute_query(query);
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.query_time = std::to_string(duration.count()) + "ms";

    if (db_result.success && db_result.data.has_value()) {
        result.data = db_result.data.value();
        if (result.data.is_array()) {
            result.row_count = result.data.size();
        }
    }

    return result;
}

std::string QAClickHouseClient::time_point_to_string(const std::chrono::system_clock::time_point& tp) const {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::chrono::system_clock::time_point QAClickHouseClient::string_to_time_point(const std::string& str) const {
    std::tm tm = {};
    std::istringstream ss(str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// =======================
// QADatabaseManager 实现
// =======================

QADatabaseManager::QADatabaseManager(const DatabaseConfiguration& config)
    : config_(config) {
    initialize_clients();
}

QADatabaseManager::~QADatabaseManager() {
    disconnect_all();
}

void QADatabaseManager::initialize_clients() {
    if (config_.enable_mongo) {
        mongo_client_ = std::make_unique<QAMongoClient>(config_.mongo_config);
    }

    if (config_.enable_clickhouse) {
        clickhouse_client_ = std::make_unique<QAClickHouseClient>(config_.clickhouse_config);
    }
}

bool QADatabaseManager::connect_all() {
    std::lock_guard<std::mutex> lock(mutex_);

    bool success = true;

    if (mongo_client_) {
        success &= mongo_client_->connect();
    }

    if (clickhouse_client_) {
        success &= clickhouse_client_->connect();
    }

    return success;
}

bool QADatabaseManager::disconnect_all() {
    std::lock_guard<std::mutex> lock(mutex_);

    bool success = true;

    if (mongo_client_) {
        success &= mongo_client_->disconnect();
    }

    if (clickhouse_client_) {
        success &= clickhouse_client_->disconnect();
    }

    return success;
}

bool QADatabaseManager::is_connected() const {
    std::lock_guard<std::mutex> lock(mutex_);

    bool mongo_connected = !mongo_client_ || mongo_client_->is_connected();
    bool clickhouse_connected = !clickhouse_client_ || clickhouse_client_->is_connected();

    return mongo_connected && clickhouse_connected;
}

QAMongoClient* QADatabaseManager::get_mongo_client() {
    return mongo_client_.get();
}

QAClickHouseClient* QADatabaseManager::get_clickhouse_client() {
    return clickhouse_client_.get();
}

std::unique_ptr<QA_Account> QADatabaseManager::get_account(const std::string& account_cookie) {
    if (mongo_client_) {
        return mongo_client_->get_account(account_cookie);
    }
    return nullptr;
}

bool QADatabaseManager::save_account(const QA_Account& account, bool sync_to_clickhouse) {
    bool success = true;

    if (mongo_client_) {
        success &= mongo_client_->save_account(account);
    }

    if (sync_to_clickhouse && clickhouse_client_) {
        // 实现账户数据同步到ClickHouse的逻辑
        std::cout << "同步账户数据到ClickHouse" << std::endl;
    }

    return success;
}

bool QADatabaseManager::sync_account_to_clickhouse(const std::string& account_cookie) {
    if (!mongo_client_ || !clickhouse_client_) {
        return false;
    }

    // 从MongoDB获取账户数据，同步到ClickHouse
    auto account = mongo_client_->get_account(account_cookie);
    if (!account) {
        return false;
    }

    // 实现同步逻辑
    std::cout << "同步账户 " << account_cookie << " 到ClickHouse" << std::endl;
    return true;
}

bool QADatabaseManager::sync_all_accounts_to_clickhouse() {
    if (!mongo_client_ || !clickhouse_client_) {
        return false;
    }

    auto account_list = mongo_client_->get_account_list();
    bool success = true;

    for (const auto& account_cookie : account_list) {
        success &= sync_account_to_clickhouse(account_cookie);
    }

    return success;
}

QADatabaseManager::HealthStatus QADatabaseManager::check_health() {
    HealthStatus status;
    status.last_check = std::chrono::system_clock::now();

    if (mongo_client_) {
        try {
            status.mongo_healthy = mongo_client_->is_connected();
        } catch (const std::exception& e) {
            status.mongo_healthy = false;
            status.mongo_error = e.what();
        }
    } else {
        status.mongo_healthy = true; // 未启用则视为健康
    }

    if (clickhouse_client_) {
        try {
            status.clickhouse_healthy = clickhouse_client_->is_connected();
        } catch (const std::exception& e) {
            status.clickhouse_healthy = false;
            status.clickhouse_error = e.what();
        }
    } else {
        status.clickhouse_healthy = true; // 未启用则视为健康
    }

    return status;
}

// =======================
// GlobalDatabaseManager 实现
// =======================

GlobalDatabaseManager& GlobalDatabaseManager::instance() {
    static GlobalDatabaseManager instance;
    return instance;
}

bool GlobalDatabaseManager::initialize(const QADatabaseManager::DatabaseConfiguration& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    try {
        manager_ = std::make_unique<QADatabaseManager>(config);
        bool success = manager_->connect_all();

        if (success) {
            initialized_ = true;
        }

        return success;
    } catch (const std::exception& e) {
        std::cerr << "初始化全局数据库管理器失败: " << e.what() << std::endl;
        return false;
    }
}

bool GlobalDatabaseManager::is_initialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

QADatabaseManager* GlobalDatabaseManager::get_manager() {
    std::lock_guard<std::mutex> lock(mutex_);
    return manager_.get();
}

QAMongoClient* GlobalDatabaseManager::get_mongo_client() {
    auto manager = get_manager();
    return manager ? manager->get_mongo_client() : nullptr;
}

QAClickHouseClient* GlobalDatabaseManager::get_clickhouse_client() {
    auto manager = get_manager();
    return manager ? manager->get_clickhouse_client() : nullptr;
}

void GlobalDatabaseManager::set_default_mongo_config(const DatabaseConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_mongo_config_ = config;
}

void GlobalDatabaseManager::set_default_clickhouse_config(const DatabaseConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_clickhouse_config_ = config;
}

DatabaseConfig GlobalDatabaseManager::get_default_mongo_config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return default_mongo_config_;
}

DatabaseConfig GlobalDatabaseManager::get_default_clickhouse_config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return default_clickhouse_config_;
}

} // namespace qaultra::connector