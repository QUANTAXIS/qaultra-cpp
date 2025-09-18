#pragma once

#include "../arrow/arrow_kline.hpp"
#include "../protocol/qifi.hpp"

#include <clickhouse/client.h>
#include <clickhouse/columns/column.h>
#include <clickhouse/columns/array.h>
#include <clickhouse/columns/date.h>
#include <clickhouse/columns/decimal.h>
#include <clickhouse/columns/string.h>
#include <clickhouse/columns/numeric.h>

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <future>
#include <atomic>

namespace qaultra::connector {

/// ClickHouse连接配置
struct ClickHouseConfig {
    std::string host = "localhost";            ///< 主机地址
    int port = 9000;                          ///< 端口号
    std::string database = "quantaxis";       ///< 数据库名
    std::string username = "default";         ///< 用户名
    std::string password = "";                ///< 密码
    bool compression = true;                  ///< 是否启用压缩
    int ping_before_query = 1;                ///< 查询前是否ping
    int send_retries = 3;                     ///< 发送重试次数
    int recv_timeout = 60000;                 ///< 接收超时(毫秒)
    int send_timeout = 60000;                 ///< 发送超时(毫秒)
    int tcp_keepalive = 1;                    ///< TCP保活
};

/// 时间聚合类型
enum class AggregationType {
    MINUTE_1 = 1,                             ///< 1分钟
    MINUTE_5 = 5,                             ///< 5分钟
    MINUTE_15 = 15,                           ///< 15分钟
    MINUTE_30 = 30,                           ///< 30分钟
    HOUR_1 = 60,                              ///< 1小时
    HOUR_4 = 240,                             ///< 4小时
    DAY_1 = 1440,                             ///< 1天
    WEEK_1 = 10080,                           ///< 1周
    MONTH_1 = 43200                           ///< 1月(近似)
};

/// 查询参数结构
struct ClickHouseQuery {
    std::string table_name = "";              ///< 表名
    std::vector<std::string> symbols;         ///< 股票代码列表
    std::string start_time = "";              ///< 开始时间
    std::string end_time = "";                ///< 结束时间
    std::vector<std::string> columns;         ///< 查询列
    std::string where_clause = "";            ///< WHERE条件
    std::string order_by = "";                ///< 排序
    int limit = 0;                            ///< 限制行数
    int offset = 0;                           ///< 偏移量
    AggregationType aggregation = AggregationType::DAY_1; ///< 聚合类型
};

/// 高性能ClickHouse连接器
class ClickHouseConnector {
public:
    /// 构造函数
    explicit ClickHouseConnector(const ClickHouseConfig& config);

    /// 析构函数
    ~ClickHouseConnector();

    /// 连接到ClickHouse
    bool connect();

    /// 断开连接
    void disconnect();

    /// 检查连接状态
    bool is_connected() const;

    /// 测试连接
    bool test_connection();

    // 表管理
    /// @{

    /// 创建K线数据表
    bool create_kline_table(const std::string& table_name);

    /// 创建交易数据表
    bool create_trade_table(const std::string& table_name);

    /// 创建账户快照表
    bool create_account_snapshot_table(const std::string& table_name);

    /// 删除表
    bool drop_table(const std::string& table_name);

    /// 检查表是否存在
    bool table_exists(const std::string& table_name);

    /// 获取表结构
    std::map<std::string, std::string> get_table_schema(const std::string& table_name);

    /// @}

    // K线数据操作
    /// @{

    /// 批量插入K线数据
    bool insert_kline_data(const std::string& table_name,
                          const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines);

    /// 查询K线数据
    std::shared_ptr<arrow_data::ArrowKlineCollection> query_kline_data(const ClickHouseQuery& query);

    /// 聚合K线数据
    std::shared_ptr<arrow_data::ArrowKlineCollection> aggregate_kline_data(
        const std::string& table_name, const std::string& symbol,
        const std::string& start_time, const std::string& end_time,
        AggregationType aggregation_type);

    /// 更新K线数据
    bool update_kline_data(const std::string& table_name, const ClickHouseQuery& filter,
                          const std::shared_ptr<arrow_data::ArrowKlineCollection>& new_data);

    /// 删除K线数据
    bool delete_kline_data(const std::string& table_name, const ClickHouseQuery& filter);

    /// @}

    // 交易数据操作
    /// @{

    /// 插入交易记录
    bool insert_trade_data(const std::string& table_name,
                          const std::vector<protocol::QIFITrade>& trades);

    /// 查询交易记录
    std::vector<protocol::QIFITrade> query_trade_data(const ClickHouseQuery& query);

    /// 插入订单记录
    bool insert_order_data(const std::string& table_name,
                          const std::vector<protocol::QIFIOrder>& orders);

    /// 查询订单记录
    std::vector<protocol::QIFIOrder> query_order_data(const ClickHouseQuery& query);

    /// @}

    // 账户快照操作
    /// @{

    /// 保存账户快照
    bool save_account_snapshot(const std::string& table_name,
                              const protocol::QIFIAccount& account,
                              const std::string& timestamp);

    /// 查询账户快照
    std::vector<protocol::QIFIAccount> query_account_snapshots(
        const std::string& table_name, const std::string& account_id,
        const std::string& start_time, const std::string& end_time);

    /// @}

    // 实时数据操作
    /// @{

    /// 插入实时行情数据
    bool insert_tick_data(const std::string& table_name,
                         const std::vector<protocol::MIFITick>& ticks);

    /// 查询实时行情数据
    std::vector<protocol::MIFITick> query_tick_data(const ClickHouseQuery& query);

    /// 流式插入数据
    bool start_streaming_insert(const std::string& table_name);
    bool stream_insert_kline(const arrow_data::KlineRecord& record);
    bool stream_insert_tick(const protocol::MIFITick& tick);
    bool finish_streaming_insert();

    /// @}

    // 分析查询
    /// @{

    /// 计算技术指标
    std::map<std::string, std::vector<double>> calculate_technical_indicators(
        const std::string& table_name, const std::string& symbol,
        const std::vector<std::string>& indicators, int period = 20);

    /// 获取统计数据
    std::map<std::string, double> get_statistics(const ClickHouseQuery& query);

    /// 价格分布分析
    std::map<double, int> get_price_distribution(const ClickHouseQuery& query, int buckets = 20);

    /// 成交量分析
    std::map<std::string, std::vector<double>> analyze_volume_patterns(
        const std::string& table_name, const std::string& symbol, int days = 30);

    /// @}

    // 性能优化
    /// @{

    /// 优化表
    bool optimize_table(const std::string& table_name);

    /// 创建物化视图
    bool create_materialized_view(const std::string& view_name, const std::string& select_query);

    /// 分析查询性能
    std::map<std::string, double> analyze_query_performance(const std::string& query);

    /// 获取表统计信息
    std::map<std::string, int64_t> get_table_statistics(const std::string& table_name);

    /// @}

    // 数据导入导出
    /// @{

    /// 从CSV文件导入数据
    bool import_from_csv(const std::string& table_name, const std::string& csv_file);

    /// 导出到CSV文件
    bool export_to_csv(const ClickHouseQuery& query, const std::string& csv_file);

    /// 从Parquet文件导入
    bool import_from_parquet(const std::string& table_name, const std::string& parquet_file);

    /// 导出到Parquet文件
    bool export_to_parquet(const ClickHouseQuery& query, const std::string& parquet_file);

    /// @}

private:
    ClickHouseConfig config_;                  ///< 配置信息
    std::unique_ptr<clickhouse::Client> client_; ///< ClickHouse客户端
    bool connected_ = false;                   ///< 连接状态

    // 流式插入状态
    std::unique_ptr<clickhouse::Block> streaming_block_;
    std::string streaming_table_;
    std::atomic<bool> streaming_active_{false};

    /// 内部辅助方法
    clickhouse::ClientOptions build_client_options() const;
    std::string build_aggregation_query(const std::string& table_name,
                                       const std::string& symbol,
                                       const std::string& start_time,
                                       const std::string& end_time,
                                       AggregationType aggregation_type) const;

    /// 数据转换方法
    clickhouse::Block klines_to_block(const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines) const;
    std::shared_ptr<arrow_data::ArrowKlineCollection> block_to_klines(const clickhouse::Block& block) const;
    clickhouse::Block trades_to_block(const std::vector<protocol::QIFITrade>& trades) const;
    std::vector<protocol::QIFITrade> block_to_trades(const clickhouse::Block& block) const;

    /// 错误处理
    void handle_clickhouse_exception(const std::exception& e, const std::string& operation) const;
    bool validate_connection() const;
};

/// ClickHouse工厂类
class ClickHouseFactory {
public:
    /// 创建默认配置的连接器
    static std::unique_ptr<ClickHouseConnector> create_default_connector();

    /// 创建带配置的连接器
    static std::unique_ptr<ClickHouseConnector> create_connector(const ClickHouseConfig& config);

    /// 创建集群连接器
    static std::unique_ptr<ClickHouseConnector> create_cluster_connector(
        const std::vector<std::string>& hosts, const ClickHouseConfig& base_config);
};

/// 分布式ClickHouse连接器
class DistributedClickHouseConnector {
public:
    /// 构造函数
    explicit DistributedClickHouseConnector(const std::vector<ClickHouseConfig>& configs);

    /// 析构函数
    ~DistributedClickHouseConnector();

    /// 添加节点
    void add_node(const ClickHouseConfig& config);

    /// 移除节点
    void remove_node(const std::string& host);

    /// 分布式插入数据
    bool distributed_insert_kline_data(const std::string& table_name,
                                      const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines);

    /// 分布式查询数据
    std::shared_ptr<arrow_data::ArrowKlineCollection> distributed_query_kline_data(
        const ClickHouseQuery& query);

    /// 负载均衡查询
    std::shared_ptr<arrow_data::ArrowKlineCollection> load_balanced_query(
        const ClickHouseQuery& query);

private:
    std::vector<std::unique_ptr<ClickHouseConnector>> connectors_;
    std::atomic<size_t> round_robin_index_{0};
    mutable std::mutex connectors_mutex_;

    /// 选择健康的连接器
    ClickHouseConnector* select_healthy_connector() const;

    /// 数据分片策略
    std::vector<std::shared_ptr<arrow_data::ArrowKlineCollection>> shard_data(
        const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines, size_t shard_count) const;
};

/// ClickHouse工具函数
namespace clickhouse_utils {
    /// 验证配置
    bool validate_config(const ClickHouseConfig& config);

    /// 生成DDL语句
    std::string generate_kline_table_ddl(const std::string& table_name, const std::string& engine = "MergeTree");
    std::string generate_trade_table_ddl(const std::string& table_name, const std::string& engine = "MergeTree");

    /// 优化查询
    std::string optimize_query(const std::string& query);

    /// 分区管理
    std::vector<std::string> get_table_partitions(ClickHouseConnector& connector,
                                                 const std::string& table_name);
    bool drop_partition(ClickHouseConnector& connector,
                       const std::string& table_name, const std::string& partition);

    /// 监控工具
    class PerformanceMonitor {
    public:
        explicit PerformanceMonitor(ClickHouseConnector& connector);
        ~PerformanceMonitor();

        void start_monitoring();
        void stop_monitoring();

        struct Metrics {
            double query_rate;                 ///< 查询速率 (QPS)
            double insert_rate;                ///< 插入速率 (RPS)
            double avg_query_time;             ///< 平均查询时间 (ms)
            double avg_insert_time;            ///< 平均插入时间 (ms)
            int64_t memory_usage;              ///< 内存使用 (bytes)
            int64_t disk_usage;                ///< 磁盘使用 (bytes)
        };

        Metrics get_metrics() const;

    private:
        ClickHouseConnector& connector_;
        std::atomic<bool> monitoring_;
        std::thread monitor_thread_;
        mutable std::mutex metrics_mutex_;
        Metrics current_metrics_;

        void monitor_loop();
    };

    /// 数据压缩工具
    class DataCompressor {
    public:
        /// 压缩K线数据
        static std::vector<uint8_t> compress_klines(
            const std::shared_ptr<arrow_data::ArrowKlineCollection>& klines);

        /// 解压K线数据
        static std::shared_ptr<arrow_data::ArrowKlineCollection> decompress_klines(
            const std::vector<uint8_t>& compressed_data);

        /// 增量压缩
        static std::vector<uint8_t> delta_compress(const std::vector<double>& values);
        static std::vector<double> delta_decompress(const std::vector<uint8_t>& compressed);
    };
}

/// 实时数据流处理器
class RealTimeStreamProcessor {
public:
    explicit RealTimeStreamProcessor(ClickHouseConnector& connector);
    ~RealTimeStreamProcessor();

    /// 开始流处理
    bool start_processing(const std::string& input_table, const std::string& output_table);

    /// 停止流处理
    void stop_processing();

    /// 添加数据转换函数
    void add_transform_function(std::function<protocol::MIFITick(const protocol::MIFITick&)> transform);

    /// 设置批处理大小
    void set_batch_size(size_t size);

private:
    ClickHouseConnector& connector_;
    std::atomic<bool> processing_;
    std::thread processing_thread_;
    std::vector<std::function<protocol::MIFITick(const protocol::MIFITick&)>> transform_functions_;
    size_t batch_size_ = 1000;

    void process_stream();
};

} // namespace qaultra::connector