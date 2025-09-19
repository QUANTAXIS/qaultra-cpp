#include <iostream>
#include "qaultra/data/unified_datatype.hpp"

using namespace qaultra::data;

int main() {
    Date trading_date(2024, 1, 15);
    StockCnDay stock_day(trading_date, "000001.XSHE", 1000, 11.5, 9.5, 10.0, 10.5, 9.8, 10.2, 1000000, 10200000);

    std::cout << "Expected: close = 10.2" << std::endl;
    std::cout << "Actual:   close = " << stock_day.get_close() << std::endl;
    std::cout << "Open = " << stock_day.get_open() << std::endl;
    std::cout << "High = " << stock_day.get_high() << std::endl;
    std::cout << "Low = " << stock_day.get_low() << std::endl;

    return 0;
}