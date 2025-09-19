#pragma once

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace qaultra::util {

/**
 * @brief 简化的UUID生成器 - 用于订单ID生成
 */
class UUIDGenerator {
public:
    /**
     * @brief 生成简化的UUID字符串
     * @return 格式: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
     */
    static std::string generate() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << dis(gen);
        }
        return ss.str();
    }

    /**
     * @brief 生成简化的订单ID (更短格式)
     * @param prefix 前缀，如"ORD"
     * @return 格式: PREFIX_XXXXXXXX
     */
    static std::string generate_order_id(const std::string& prefix = "ORD") {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint32_t> dis;

        std::stringstream ss;
        ss << prefix << "_" << std::hex << std::uppercase << dis(gen);
        return ss.str();
    }

    /**
     * @brief 生成时间戳-随机数组合的ID
     * @param prefix 前缀
     * @return 格式: PREFIX_TIMESTAMP_XXXX
     */
    static std::string generate_time_based_id(const std::string& prefix = "ID") {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint16_t> dis;

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::stringstream ss;
        ss << prefix << "_" << now << "_" << std::hex << std::uppercase << dis(gen);
        return ss.str();
    }
};

} // namespace qaultra::util