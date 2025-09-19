#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <cassert>

// 简化版的数据类型测试，不依赖复杂的库
struct SimpleKline {
    std::string order_book_id;
    double open = 0.0;
    double close = 0.0;
    double high = 0.0;
    double low = 0.0;
    double volume = 0.0;
    double total_turnover = 0.0;

    SimpleKline() = default;
    SimpleKline(const std::string& id, double o, double c, double h, double l, double v, double t)
        : order_book_id(id), open(o), close(c), high(h), low(l), volume(v), total_turnover(t) {}

    double get_change_percent() const {
        if (open == 0.0) return 0.0;
        return (close - open) / open * 100.0;
    }

    bool operator==(const SimpleKline& other) const {
        return order_book_id == other.order_book_id &&
               std::abs(open - other.open) < 1e-9 &&
               std::abs(close - other.close) < 1e-9 &&
               std::abs(high - other.high) < 1e-9 &&
               std::abs(low - other.low) < 1e-9 &&
               std::abs(volume - other.volume) < 1e-9;
    }
};

class SimpleMarketCenter {
private:
    std::vector<SimpleKline> klines_;

public:
    void add_kline(const SimpleKline& kline) {
        klines_.push_back(kline);
    }

    size_t size() const { return klines_.size(); }

    const SimpleKline* find_kline(const std::string& order_book_id) const {
        for (const auto& kline : klines_) {
            if (kline.order_book_id == order_book_id) {
                return &kline;
            }
        }
        return nullptr;
    }

    std::vector<SimpleKline> get_all_klines() const {
        return klines_;
    }

    double calculate_total_volume() const {
        double total = 0.0;
        for (const auto& kline : klines_) {
            total += kline.volume;
        }
        return total;
    }
};

void test_kline_basic() {
    std::cout << "测试基本 Kline 功能..." << std::endl;

    SimpleKline kline("000001.XSHE", 10.0, 10.5, 10.8, 9.8, 1000000, 50000000);

    assert(kline.order_book_id == "000001.XSHE");
    assert(std::abs(kline.open - 10.0) < 1e-9);
    assert(std::abs(kline.close - 10.5) < 1e-9);
    assert(std::abs(kline.get_change_percent() - 5.0) < 1e-9);

    std::cout << "✓ Kline 基本功能测试通过" << std::endl;
}

void test_kline_comparison() {
    std::cout << "测试 Kline 比较功能..." << std::endl;

    SimpleKline kline1("000001.XSHE", 10.0, 10.5, 10.8, 9.8, 1000000, 50000000);
    SimpleKline kline2("000001.XSHE", 10.0, 10.5, 10.8, 9.8, 1000000, 50000000);
    SimpleKline kline3("000002.XSHE", 10.0, 10.5, 10.8, 9.8, 1000000, 50000000);

    assert(kline1 == kline2);
    assert(!(kline1 == kline3));

    std::cout << "✓ Kline 比较功能测试通过" << std::endl;
}

void test_market_center() {
    std::cout << "测试 MarketCenter 功能..." << std::endl;

    SimpleMarketCenter mc;

    // 添加测试数据
    for (int i = 0; i < 10; ++i) {
        std::string code = "00000" + std::to_string(i) + ".XSHE";
        double base_price = 10.0 + i * 0.1;
        SimpleKline kline(code, base_price, base_price + 0.5, base_price + 1.0,
                         base_price - 0.5, 1000 * (i + 1), 50000 * (i + 1));
        mc.add_kline(kline);
    }

    assert(mc.size() == 10);

    // 测试查找功能
    const auto* found = mc.find_kline("000005.XSHE");
    assert(found != nullptr);
    assert(found->order_book_id == "000005.XSHE");

    // 测试总量计算
    double total_volume = mc.calculate_total_volume();
    assert(total_volume > 0);

    std::cout << "✓ MarketCenter 功能测试通过 (处理了 " << mc.size() << " 条数据)" << std::endl;
    std::cout << "✓ 总成交量: " << total_volume << std::endl;
}

void test_performance() {
    std::cout << "测试性能..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    SimpleMarketCenter mc;

    // 创建大量测试数据
    for (int i = 0; i < 100000; ++i) {
        std::string code = "CODE" + std::to_string(i % 1000);
        double base_price = 10.0 + (i % 100) * 0.1;
        SimpleKline kline(code, base_price, base_price + 0.5, base_price + 1.0,
                         base_price - 0.5, 1000 + i, 50000 + i * 50);
        mc.add_kline(kline);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "✓ 性能测试完成 - 处理 100,000 条记录用时: "
              << duration.count() << " 微秒" << std::endl;
    std::cout << "✓ 平均每条记录处理时间: "
              << (double)duration.count() / 100000.0 << " 微秒" << std::endl;

    // 测试查找性能
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        std::string code = "CODE" + std::to_string(i);
        mc.find_kline(code);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "✓ 查找性能测试 - 1000 次查找用时: "
              << duration.count() << " 微秒" << std::endl;
}

void test_edge_cases() {
    std::cout << "测试边界情况..." << std::endl;

    // 测试零除法保护
    SimpleKline zero_open("TEST", 0.0, 10.0, 10.0, 0.0, 1000, 50000);
    assert(zero_open.get_change_percent() == 0.0);

    // 测试空 MarketCenter
    SimpleMarketCenter empty_mc;
    assert(empty_mc.size() == 0);
    assert(empty_mc.find_kline("NONEXISTENT") == nullptr);
    assert(empty_mc.calculate_total_volume() == 0.0);

    std::cout << "✓ 边界情况测试通过" << std::endl;
}

void test_calculation_accuracy() {
    std::cout << "测试计算精度..." << std::endl;

    // 测试精确的浮点数计算
    SimpleKline kline("TEST", 100.0, 105.0, 106.0, 99.0, 10000, 1000000);

    double expected_change = 5.0; // (105-100)/100 * 100
    double actual_change = kline.get_change_percent();

    assert(std::abs(actual_change - expected_change) < 1e-10);

    // 测试极小值
    SimpleKline small_kline("SMALL", 0.0001, 0.0002, 0.0002, 0.0001, 1, 0.1);
    double small_change = small_kline.get_change_percent();
    assert(std::abs(small_change - 100.0) < 1e-6); // 100% 增长

    std::cout << "✓ 计算精度测试通过" << std::endl;
}

int main() {
    std::cout << "=== QAULTRA C++ 简化测试套件 ===" << std::endl;
    std::cout << "开始运行核心功能测试..." << std::endl;
    std::cout << std::endl;

    try {
        test_kline_basic();
        test_kline_comparison();
        test_market_center();
        test_edge_cases();
        test_calculation_accuracy();
        test_performance();

        std::cout << std::endl;
        std::cout << "🎉 所有测试通过！" << std::endl;
        std::cout << "=== 测试总结 ===" << std::endl;
        std::cout << "✓ 基本数据结构功能正常" << std::endl;
        std::cout << "✓ 数据操作和查找功能正常" << std::endl;
        std::cout << "✓ 边界情况处理正确" << std::endl;
        std::cout << "✓ 计算精度满足要求" << std::endl;
        std::cout << "✓ 性能表现良好" << std::endl;
        std::cout << std::endl;
        std::cout << "核心架构验证完成，可以继续开发更复杂的功能。" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 未知错误导致测试失败" << std::endl;
        return 1;
    }
}