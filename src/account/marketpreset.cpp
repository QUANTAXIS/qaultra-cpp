#include "../../include/qaultra/account/marketpreset.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace qaultra::account {

// CodePreset 构造函数
CodePreset::CodePreset(const std::string& name_param,
                       int unit_table_param,
                       double price_tick_param,
                       double buy_frozen_coeff_param,
                       double sell_frozen_coeff_param,
                       const std::string& exchange_param,
                       double commission_coeff_peramount_param,
                       double commission_coeff_pervol_param,
                       double commission_coeff_today_peramount_param,
                       double commission_coeff_today_pervol_param)
    : name(name_param)
    , unit_table(unit_table_param)
    , price_tick(price_tick_param)
    , buy_frozen_coeff(buy_frozen_coeff_param)
    , sell_frozen_coeff(sell_frozen_coeff_param)
    , exchange(exchange_param)
    , commission_coeff_peramount(commission_coeff_peramount_param)
    , commission_coeff_pervol(commission_coeff_pervol_param)
    , commission_coeff_today_peramount(commission_coeff_today_peramount_param)
    , commission_coeff_today_pervol(commission_coeff_today_pervol_param)
{
}

void CodePreset::print() const {
    std::cout << "name " << name
              << " / buy_frozen " << buy_frozen_coeff
              << " / sell_frozen " << sell_frozen_coeff << std::endl;
}

nlohmann::json CodePreset::to_json() const {
    return nlohmann::json{
        {"name", name},
        {"unit_table", unit_table},
        {"price_tick", price_tick},
        {"buy_frozen_coeff", buy_frozen_coeff},
        {"sell_frozen_coeff", sell_frozen_coeff},
        {"exchange", exchange},
        {"commission_coeff_peramount", commission_coeff_peramount},
        {"commission_coeff_pervol", commission_coeff_pervol},
        {"commission_coeff_today_peramount", commission_coeff_today_peramount},
        {"commission_coeff_today_pervol", commission_coeff_today_pervol}
    };
}

CodePreset CodePreset::from_json(const nlohmann::json& j) {
    CodePreset preset;
    preset.name = j.at("name").get<std::string>();
    preset.unit_table = j.at("unit_table").get<int>();
    preset.price_tick = j.at("price_tick").get<double>();
    preset.buy_frozen_coeff = j.at("buy_frozen_coeff").get<double>();
    preset.sell_frozen_coeff = j.at("sell_frozen_coeff").get<double>();
    preset.exchange = j.at("exchange").get<std::string>();
    preset.commission_coeff_peramount = j.at("commission_coeff_peramount").get<double>();
    preset.commission_coeff_pervol = j.at("commission_coeff_pervol").get<double>();
    preset.commission_coeff_today_peramount = j.at("commission_coeff_today_peramount").get<double>();
    preset.commission_coeff_today_pervol = j.at("commission_coeff_today_pervol").get<double>();
    return preset;
}

// MarketPreset 实现
MarketPreset MarketPreset::create_default() {
    MarketPreset market_preset;
    market_preset.init_all_presets();
    return market_preset;
}

CodePreset MarketPreset::get(const std::string& code) {
    // 默认股票配置 - 匹配Rust实现
    CodePreset default_preset{
        code,           // name
        1,              // unit_table
        0.01,           // price_tick
        1.0,            // buy_frozen_coeff
        1.0,            // sell_frozen_coeff
        "STOCK",        // exchange
        0.00032,        // commission_coeff_peramount
        0.0,            // commission_coeff_pervol
        0.00032,        // commission_coeff_today_peramount
        0.0             // commission_coeff_today_pervol
    };

    // 处理L8和L9结尾的合约代码
    if (code.length() >= 2 && (code.substr(code.length() - 2) == "L8" ||
                                code.substr(code.length() - 2) == "L9")) {
        std::string base_code = code.substr(0, code.length() - 2);
        // 转换为大写
        std::transform(base_code.begin(), base_code.end(), base_code.begin(), ::toupper);

        auto it = preset_.find(base_code);
        if (it != preset_.end()) {
            return it->second;
        }
    } else {
        // 提取字母部分并转换为大写
        std::string symbol = extract_symbol(code);
        std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

        auto it = preset_.find(symbol);
        if (it != preset_.end()) {
            return it->second;
        }
    }

    return default_preset;
}

void MarketPreset::add_preset(const std::string& code, const CodePreset& preset) {
    preset_[code] = preset;
}

bool MarketPreset::contains(const std::string& code) const {
    return preset_.find(code) != preset_.end();
}

std::vector<std::string> MarketPreset::get_all_codes() const {
    std::vector<std::string> codes;
    codes.reserve(preset_.size());
    for (const auto& pair : preset_) {
        codes.push_back(pair.first);
    }
    return codes;
}

std::vector<CodePreset> MarketPreset::get_by_exchange(const std::string& exchange) const {
    std::vector<CodePreset> result;
    for (const auto& pair : preset_) {
        if (pair.second.exchange == exchange) {
            result.push_back(pair.second);
        }
    }
    return result;
}

nlohmann::json MarketPreset::to_json() const {
    nlohmann::json j;
    for (const auto& pair : preset_) {
        j["preset"][pair.first] = pair.second.to_json();
    }
    return j;
}

MarketPreset MarketPreset::from_json(const nlohmann::json& j) {
    MarketPreset preset;
    if (j.contains("preset")) {
        for (const auto& item : j["preset"].items()) {
            preset.preset_[item.key()] = CodePreset::from_json(item.value());
        }
    }
    return preset;
}

std::string MarketPreset::extract_symbol(const std::string& code) const {
    std::string symbol;
    for (char c : code) {
        if (std::isalpha(c)) {
            symbol += c;
        }
    }
    return symbol;
}

void MarketPreset::init_all_presets() {
    // 上海期货交易所 (SHFE) - 完全匹配Rust实现
    preset_["AG"] = CodePreset{"白银", 15, 1.0, 0.1, 0.1, "SHFE", 5e-05, 0.0, 5e-05, 0.0};
    preset_["AL"] = CodePreset{"铝", 5, 5.0, 0.1, 0.1, "SHFE", 0.0, 3.0, 0.0, 0.0};
    preset_["AU"] = CodePreset{"黄金", 1000, 0.02, 0.08, 0.08, "SHFE", 0.0, 10.0, 0.0, 0.0};
    preset_["BU"] = CodePreset{"石油沥青", 10, 2.0, 0.15, 0.15, "SHFE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["CU"] = CodePreset{"铜", 5, 10.0, 0.1, 0.1, "SHFE", 5e-05, 0.0, 0.0, 0.0};
    preset_["FU"] = CodePreset{"燃料油", 10, 1.0, 0.15, 0.15, "SHFE", 5e-05, 0.0, 0.0, 0.0};
    preset_["HC"] = CodePreset{"热轧卷板", 10, 1.0, 0.09, 0.09, "SHFE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["NI"] = CodePreset{"镍", 1, 10.0, 0.1, 0.1, "SHFE", 0.0, 6.0, 0.0, 6.0};
    preset_["PB"] = CodePreset{"铅", 5, 5.0, 0.1, 0.1, "SHFE", 4e-05, 0.0, 0.0, 0.0};
    preset_["RB"] = CodePreset{"螺纹钢", 10, 1.0, 0.09, 0.09, "SHFE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["RU"] = CodePreset{"天然橡胶", 10, 5.0, 0.09, 0.09, "SHFE", 4.5e-05, 0.0, 4.5e-05, 0.0};
    preset_["SN"] = CodePreset{"锡", 1, 10.0, 0.1, 0.1, "SHFE", 0.0, 1.0, 0.0, 0.0};
    preset_["SP"] = CodePreset{"漂针浆", 10, 2.0, 0.08, 0.08, "SHFE", 5e-05, 0.0, 0.0, 0.0};
    preset_["WR"] = CodePreset{"线材", 10, 1.0, 0.09, 0.09, "SHFE", 4e-05, 0.0, 0.0, 0.0};
    preset_["ZN"] = CodePreset{"锌", 5, 5.0, 0.1, 0.1, "SHFE", 0.0, 3.0, 0.0, 0.0};
    preset_["SS"] = CodePreset{"不锈钢", 5, 5.0, 0.08, 0.08, "SHFE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["AO"] = CodePreset{"ao", 20, 1.0, 0.2, 0.2, "SHFE", 0.000101, 0.0, 0.0, 0.0};
    preset_["BR"] = CodePreset{"br", 5, 1.0, 0.2, 0.2, "SHFE", 0.000101, 0.0, 0.000101, 0.0};

    // 大连商品交易所 (DCE) - 完全匹配Rust实现
    preset_["A"] = CodePreset{"豆一", 10, 1.0, 0.05, 0.05, "DCE", 0.0, 2.0, 0.0, 2.0};
    preset_["B"] = CodePreset{"豆二", 10, 1.0, 0.05, 0.05, "DCE", 0.0, 1.0, 0.0, 1.0};
    preset_["BB"] = CodePreset{"细木工板", 500, 0.05, 0.2, 0.2, "DCE", 0.0001, 0.0, 5e-05, 0.0};
    preset_["C"] = CodePreset{"黄玉米", 10, 1.0, 0.05, 0.05, "DCE", 0.0, 1.2, 0.0, 0.0};
    preset_["CS"] = CodePreset{"玉米淀粉", 10, 1.0, 0.05, 0.05, "DCE", 0.0, 1.5, 0.0, 0.0};
    preset_["EG"] = CodePreset{"乙二醇", 10, 1.0, 0.06, 0.06, "DCE", 0.0, 4.0, 0.0, 0.0};
    preset_["FB"] = CodePreset{"中密度纤维板", 500, 0.05, 0.2, 0.2, "DCE", 0.0001, 0.0, 5e-05, 0.0};
    preset_["I"] = CodePreset{"铁矿石", 100, 0.5, 0.08, 0.08, "DCE", 6e-05, 0.0, 6e-05, 0.0};
    preset_["J"] = CodePreset{"冶金焦炭", 100, 0.5, 0.08, 0.08, "DCE", 0.00018, 0.0, 0.00018, 0.0};
    preset_["JD"] = CodePreset{"鲜鸡蛋", 10, 1.0, 0.07, 0.07, "DCE", 0.00015, 0.0, 0.00015, 0.0};
    preset_["JM"] = CodePreset{"焦煤", 60, 0.5, 0.08, 0.08, "DCE", 0.00018, 0.0, 0.00018, 0.0};
    preset_["L"] = CodePreset{"线型低密度聚乙烯", 5, 5.0, 0.05, 0.05, "DCE", 0.0, 2.0, 0.0, 0.0};
    preset_["M"] = CodePreset{"豆粕", 10, 1.0, 0.05, 0.05, "DCE", 0.0, 1.5, 0.0, 0.0};
    preset_["P"] = CodePreset{"棕榈油", 10, 2.0, 0.08, 0.08, "DCE", 0.0, 2.5, 0.0, 0.0};
    preset_["PP"] = CodePreset{"聚丙烯", 5, 1.0, 0.05, 0.05, "DCE", 6e-05, 0.0, 3e-05, 0.0};
    preset_["V"] = CodePreset{"聚氯乙烯", 5, 5.0, 0.05, 0.05, "DCE", 0.0, 2.0, 0.0, 0.0};
    preset_["Y"] = CodePreset{"豆油", 10, 2.0, 0.05, 0.05, "DCE", 0.0, 2.5, 0.0, 0.0};
    preset_["EB"] = CodePreset{"苯乙烯", 5, 1.0, 0.05, 0.05, "DCE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["RR"] = CodePreset{"粳米", 10, 1.0, 0.05, 0.05, "DCE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["PG"] = CodePreset{"液化石油气", 20, 1.0, 0.05, 0.05, "DCE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["LH"] = CodePreset{"生猪", 16, 1.0, 0.2, 0.2, "DCE", 0.000201, 0.0, 0.000201, 0.0};

    // 郑州商品交易所 (CZCE) - 完全匹配Rust实现
    preset_["AP"] = CodePreset{"鲜苹果", 10, 1.0, 0.08, 0.08, "CZCE", 0.0, 5.0, 0.0, 5.0};
    preset_["CF"] = CodePreset{"一号棉花", 5, 5.0, 0.05, 0.05, "CZCE", 0.0, 4.3, 0.0, 0.0};
    preset_["CY"] = CodePreset{"棉纱", 5, 5.0, 0.05, 0.05, "CZCE", 0.0, 4.0, 0.0, 0.0};
    preset_["FG"] = CodePreset{"玻璃", 20, 1.0, 0.05, 0.05, "CZCE", 0.0, 3.0, 0.0, 6.0};
    preset_["JR"] = CodePreset{"粳稻", 20, 1.0, 0.05, 0.05, "CZCE", 0.0, 3.0, 0.0, 3.0};
    preset_["LR"] = CodePreset{"晚籼稻", 20, 1.0, 0.05, 0.05, "CZCE", 0.0, 3.0, 0.0, 3.0};
    preset_["MA"] = CodePreset{"甲醇MA", 10, 1.0, 0.07, 0.07, "CZCE", 0.0, 2.0, 0.0, 6.0};
    preset_["OI"] = CodePreset{"菜籽油", 10, 1.0, 0.05, 0.05, "CZCE", 0.0, 2.0, 0.0, 0.0};
    preset_["PM"] = CodePreset{"普通小麦", 50, 1.0, 0.05, 0.05, "CZCE", 0.0, 5.0, 0.0, 5.0};
    preset_["RI"] = CodePreset{"早籼", 20, 1.0, 0.05, 0.05, "CZCE", 0.0, 2.5, 0.0, 2.5};
    preset_["RM"] = CodePreset{"菜籽粕", 10, 1.0, 0.06, 0.06, "CZCE", 0.0, 1.5, 0.0, 0.0};
    preset_["RS"] = CodePreset{"油菜籽", 10, 1.0, 0.2, 0.2, "CZCE", 0.0, 2.0, 0.0, 2.0};
    preset_["SF"] = CodePreset{"硅铁", 5, 2.0, 0.07, 0.07, "CZCE", 0.0, 3.0, 0.0, 9.0};
    preset_["SM"] = CodePreset{"锰硅", 5, 2.0, 0.07, 0.07, "CZCE", 0.0, 3.0, 0.0, 6.0};
    preset_["SR"] = CodePreset{"白砂糖", 10, 1.0, 0.05, 0.05, "CZCE", 0.0, 3.0, 0.0, 0.0};
    preset_["TA"] = CodePreset{"精对苯二甲酸", 5, 2.0, 0.06, 0.06, "CZCE", 0.0, 3.0, 0.0, 0.0};
    preset_["WH"] = CodePreset{"优质强筋小麦", 20, 1.0, 0.2, 0.2, "CZCE", 0.0, 2.5, 0.0, 0.0};
    preset_["ZC"] = CodePreset{"动力煤ZC", 100, 0.2, 0.06, 0.06, "CZCE", 0.0, 4.0, 0.0, 4.0};
    preset_["SA"] = CodePreset{"纯碱", 20, 1.0, 0.05, 0.05, "CZCE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["CJ"] = CodePreset{"红枣", 5, 5.0, 0.07, 0.07, "CZCE", 0.0, 3.0, 0.0, 3.0};
    preset_["UR"] = CodePreset{"尿素", 20, 1.0, 0.05, 0.05, "CZCE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["PF"] = CodePreset{"短纤", 5, 1.0, 0.2, 0.2, "CZCE", 0.000001, 3.0, 0.000001, 3.0};
    preset_["PK"] = CodePreset{"花生仁", 5, 1.0, 0.2, 0.2, "CZCE", 0.000001, 4.0, 0.0, 4.0};
    preset_["PX"] = CodePreset{"对二甲苯", 5, 1.0, 0.12, 0.12, "CZCE", 0.000101, 0.0, 0.000101, 0.0};
    preset_["SH"] = CodePreset{"烧碱", 30, 1.0, 0.12, 0.12, "CZCE", 0.000101, 0.0, 0.000101, 0.0};

    // 中国金融期货交易所 (CFFEX) - 完全匹配Rust实现
    preset_["IC"] = CodePreset{"中证500指数", 200, 0.2, 0.12, 0.12, "CFFEX", 2.301e-05, 0.0, 0.00023, 0.0};
    preset_["IM"] = CodePreset{"中证1000指数", 200, 0.2, 0.12, 0.12, "CFFEX", 2.301e-05, 0.0, 0.00023, 0.0};
    preset_["IF"] = CodePreset{"沪深300指数", 300, 0.2, 0.1, 0.1, "CFFEX", 2.301e-05, 0.0, 0.00023, 0.0};
    preset_["IH"] = CodePreset{"上证50指数", 300, 0.2, 0.05, 0.05, "CFFEX", 2.301e-05, 0.0, 0.00023, 0.0};
    preset_["T"] = CodePreset{"10年期国债", 10000, 0.005, 0.03, 0.03, "CFFEX", 0.0, 3.0, 0.0, 3.0};
    preset_["TF"] = CodePreset{"5年期国债", 10000, 0.005, 0.02, 0.02, "CFFEX", 0.0, 3.0, 0.0, 3.0};
    preset_["TS"] = CodePreset{"2年期国债", 20000, 0.002, 0.01, 0.01, "CFFEX", 0.0, 3.0, 0.0, 3.0};
    preset_["TL"] = CodePreset{"30年期国债", 10000, 0.01, 0.05, 0.05, "CFFEX", 0.0, 3.0, 0.0, 3.0};

    // 上海国际能源交易中心 (INE) - 完全匹配Rust实现
    preset_["SC"] = CodePreset{"原油", 1000, 0.1, 0.1, 0.1, "INE", 0.0, 20.0, 0.0, 0.0};
    preset_["NR"] = CodePreset{"20号胶", 10, 5.0, 0.09, 0.09, "INE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["LU"] = CodePreset{"低硫燃油", 10, 1.0, 0.08, 0.08, "INE", 0.0001, 0.0, 0.0001, 0.0};
    preset_["BC"] = CodePreset{"国际铜", 5, 1.0, 0.2, 0.2, "INE", 0.000011, 0.01, 0.000011, 0.01};
    preset_["EC"] = CodePreset{"ec", 50, 1.0, 0.22, 0.22, "INE", 0.000601, 0.0, 0.000601, 0.0};

    // 广州期货交易所 (GFEX) - 完全匹配Rust实现
    preset_["SI"] = CodePreset{"工业硅", 5, 1.0, 0.2, 0.2, "GFEX", 0.000001, 0.0, 0.0, 0.0};
    preset_["LC"] = CodePreset{"碳酸锂", 1, 1.0, 0.2, 0.2, "GFEX", 0.000081, 0.0, 0.000081, 0.0};

    // 数字货币 - 完全匹配Rust实现
    preset_["BTCUSDT"] = CodePreset{"BTC/USDT", 1, 0.01, 1.0, 1.0, "binance", 0.001, 0.0, 0.001, 0.0};
}

} // namespace qaultra::account