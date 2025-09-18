#pragma once

#include "../protocol/qifi.hpp"
#include "../arrow/arrow_kline.hpp"

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <future>

namespace qaultra::connector {

/// MongoDB连接配置
struct MongoConfig {
    std::string host = "localhost";             ///< 主机地址
    int port = 27017;                          ///< 端口号
    std::string database = "quantaxis";        ///< 数据库名
    std::string username = "";                 ///< 用户名
    std::string password = "";                 ///< 密码
    std::string auth_source = "admin";         ///< 认证数据库
    bool use_ssl = false;                      ///< 是否使用SSL
    int connection_timeout = 5000;             ///< 连接超时(毫秒)
    int socket_timeout = 30000;                ///< 套接字超时(毫秒)
    int max_pool_size = 10;                    ///< 最大连接池大小
};

/// 查询条件结构
struct QueryFilter {
    std::string code = "";                     ///< 股票代码
    std::string start_date = "";               ///< 开始日期
    std::string end_date = "";                 ///< 结束日期
    std::string frequency = "1D";              ///< 数据频率
    std::vector<std::string> fields;           ///< 查询字段
    int limit = 0;                             ///< 限制条数 (0为不限制)
    int skip = 0;                              ///< 跳过条数
};

/// 高性能MongoDB连接器
class MongoConnector {
public:
    /// 构造函数
    explicit MongoConnector(const MongoConfig& config);

    /// 析构函数
    ~MongoConnector();

    /// 连接到MongoDB
    bool connect();

    /// 断开连接
    void disconnect();

    /// 检查连接状态
    bool is_connected() const;

    /// 测试连接
    bool test_connection();

    // 账户数据操作
    /// @{

    /// 保存账户数据
    bool save_account(const protocol::QIFIAccount& account);

    /// 加载账户数据
    std::optional<protocol::QIFIAccount> load_account(const std::string& account_id);

    /// 更新账户数据
    bool update_account(const protocol::QIFIAccount& account);

    /// 删除账户数据
    bool delete_account(const std::string& account_id);

    /// 获取所有账户列表
    std::vector<std::string> get_account_list();

    /// @}

    // K线数据操作
    /// @{

    /// 保存K线数据
    bool save_kline_data(const std::string& collection_name,
                        const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines);

    /// 加载K线数据
    std::shared_ptr<arrow_data::ArrowKlineCollection> load_kline_data(
        const std::string& collection_name, const QueryFilter& filter);

    /// 批量保存K线数据
    bool batch_save_kline_data(const std::map<std::string,
                              std::shared_ptr<arrow_data::ArrowKlineCollection>>& data_map);

    /// 更新K线数据
    bool update_kline_data(const std::string& collection_name,
                          const QueryFilter& filter,
                          const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines);

    /// 删除K线数据
    bool delete_kline_data(const std::string& collection_name, const QueryFilter& filter);

    /// @}

    // 交易数据操作
    /// @{

    /// 保存交易记录
    bool save_trade(const protocol::QIFITrade& trade);

    /// 加载交易记录
    std::vector<protocol::QIFITrade> load_trades(const std::string& account_id,
                                                const std::string& start_date = "",
                                                const std::string& end_date = "");

    /// 保存订单记录
    bool save_order(const protocol::QIFIOrder& order);

    /// 加载订单记录
    std::vector<protocol::QIFIOrder> load_orders(const std::string& account_id,
                                                const std::string& start_date = "",
                                                const std::string& end_date = "");

    /// @}

    // 实时数据操作
    /// @{

    /// 保存实时行情
    bool save_tick_data(const std::string& collection_name,
                       const std::vector<protocol::MIFITick>& ticks);

    /// 加载实时行情
    std::vector<protocol::MIFITick> load_tick_data(const std::string& collection_name,
                                                  const QueryFilter& filter);

    /// @}

    // 数据库管理
    /// @{

    /// 创建索引
    bool create_index(const std::string& collection_name,
                     const std::map<std::string, int>& keys);

    /// 创建复合索引
    bool create_compound_index(const std::string& collection_name,
                              const std::vector<std::pair<std::string, int>>& keys);

    /// 获取集合统计信息
    std::map<std::string, int64_t> get_collection_stats(const std::string& collection_name);

    /// 清空集合
    bool clear_collection(const std::string& collection_name);

    /// 删除集合
    bool drop_collection(const std::string& collection_name);

    /// @}

    // 聚合查询
    /// @{

    /// 获取股票列表
    std::vector<std::string> get_stock_list();

    /// 获取日期范围
    std::pair<std::string, std::string> get_date_range(const std::string& collection_name,
                                                       const std::string& code = "");

    /// 统计数据
    std::map<std::string, double> get_statistics(const std::string& collection_name,
                                                 const QueryFilter& filter);

    /// @}

private:
    MongoConfig config_;                       ///< 配置信息
    std::unique_ptr<mongocxx::client> client_; ///< MongoDB客户端
    std::unique_ptr<mongocxx::database> db_;   ///< 数据库对象
    bool connected_ = false;                   ///< 连接状态

    static mongocxx::instance instance_;       ///< MongoDB实例(单例)

    /// 内部辅助方法
    std::string build_connection_string() const;
    bsoncxx::document::value build_filter_document(const QueryFilter& filter) const;
    bsoncxx::document::value qifi_account_to_bson(const protocol::QIFIAccount& account) const;
    protocol::QIFIAccount bson_to_qifi_account(const bsoncxx::document::view& doc) const;

    /// 错误处理
    void handle_mongodb_exception(const std::exception& e, const std::string& operation) const;
    bool validate_connection() const;
};

/// MongoDB工厂类
class MongoFactory {
public:
    /// 创建默认配置的连接器
    static std::unique_ptr<MongoConnector> create_default_connector();

    /// 创建带配置的连接器
    static std::unique_ptr<MongoConnector> create_connector(const MongoConfig& config);

    /// 创建集群连接器
    static std::unique_ptr<MongoConnector> create_cluster_connector(
        const std::vector<std::string>& hosts, const MongoConfig& base_config);
};

/// MongoDB工具函数
namespace mongo_utils {
    /// 验证连接字符串
    bool validate_connection_string(const std::string& connection_string);

    /// 解析连接字符串
    MongoConfig parse_connection_string(const std::string& connection_string);

    /// 优化集合性能
    bool optimize_collection(MongoConnector& connector, const std::string& collection_name);

    /// 创建标准索引
    bool create_standard_indexes(MongoConnector& connector);

    /// 数据库备份
    bool backup_database(MongoConnector& connector, const std::string& backup_path);

    /// 数据库恢复
    bool restore_database(MongoConnector& connector, const std::string& backup_path);

    /// 监控连接状态
    class ConnectionMonitor {
    public:
        explicit ConnectionMonitor(MongoConnector& connector);
        ~ConnectionMonitor();

        void start_monitoring();
        void stop_monitoring();
        bool is_healthy() const;

    private:
        MongoConnector& connector_;
        std::atomic<bool> monitoring_;
        std::thread monitor_thread_;
        mutable std::mutex status_mutex_;
        bool last_status_;
    };
}

/// 异步MongoDB连接器
class AsyncMongoConnector {
public:
    explicit AsyncMongoConnector(const MongoConfig& config);
    ~AsyncMongoConnector();

    /// 异步保存账户数据
    std::future<bool> save_account_async(const protocol::QIFIAccount& account);

    /// 异步加载账户数据
    std::future<std::optional<protocol::QIFIAccount>> load_account_async(const std::string& account_id);

    /// 异步保存K线数据
    std::future<bool> save_kline_data_async(const std::string& collection_name,
                                           const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines);

    /// 异步加载K线数据
    std::future<std::shared_ptr<arrow_data::ArrowKlineCollection>> load_kline_data_async(
        const std::string& collection_name, const QueryFilter& filter);

    /// 批量异步操作
    std::vector<std::future<bool>> batch_save_async(
        const std::vector<std::pair<std::string, protocol::QIFIAccount>>& accounts);

private:
    std::unique_ptr<MongoConnector> connector_;
    std::thread_pool<> thread_pool_;           ///< 线程池
};

} // namespace qaultra::connector