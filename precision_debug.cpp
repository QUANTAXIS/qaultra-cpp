#include <iostream>
#include <iomanip>
#include "qaultra/data/unified_datatype.hpp"

using namespace qaultra::data;

int main() {
    // Test the exact same values as in the test
    Date trading_date(2024, 1, 15);

    // Show what happens when double 10.2 becomes float then double
    double original_close = 10.2;
    float float_close = static_cast<float>(original_close);
    double recovered_close = static_cast<double>(float_close);

    std::cout << std::fixed << std::setprecision(15) << std::endl;
    std::cout << "Original double: " << original_close << std::endl;
    std::cout << "As float:        " << float_close << std::endl;
    std::cout << "Back to double:  " << recovered_close << std::endl;
    std::cout << "Difference:      " << (recovered_close - original_close) << std::endl;

    // Create the StockCnDay exactly as in test
    StockCnDay stock_day(trading_date, "000001.XSHE", 1000, 11.5, 9.5, 10.0, 10.5, 9.8, 10.2, 1000000, 10200000);

    std::cout << "\nStockCnDay results:" << std::endl;
    std::cout << "get_close(): " << stock_day.get_close() << std::endl;
    std::cout << "Expected:    10.2" << std::endl;
    std::cout << "Exact diff:  " << (stock_day.get_close() - 10.2) << std::endl;
    std::cout << "Abs diff:    " << std::abs(stock_day.get_close() - 10.2) << std::endl;

    // Test with tolerance
    double tolerance = 1e-9;
    bool passes_tolerance = std::abs(stock_day.get_close() - 10.2) < tolerance;
    std::cout << "Passes 1e-9 tolerance: " << (passes_tolerance ? "YES" : "NO") << std::endl;

    // Try larger tolerance
    tolerance = 1e-6;
    passes_tolerance = std::abs(stock_day.get_close() - 10.2) < tolerance;
    std::cout << "Passes 1e-6 tolerance: " << (passes_tolerance ? "YES" : "NO") << std::endl;

    return 0;
}