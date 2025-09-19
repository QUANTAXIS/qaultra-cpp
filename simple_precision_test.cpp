#include <iostream>
#include <iomanip>

int main() {
    // Test the exact same precision issue
    double original = 10.2;
    float as_float = static_cast<float>(original);
    double back_to_double = static_cast<double>(as_float);

    std::cout << std::fixed << std::setprecision(20) << std::endl;
    std::cout << "Original: " << original << std::endl;
    std::cout << "As float: " << as_float << std::endl;
    std::cout << "Back:     " << back_to_double << std::endl;
    std::cout << "Diff:     " << (back_to_double - original) << std::endl;
    std::cout << "Abs diff: " << std::abs(back_to_double - original) << std::endl;

    // Test tolerances
    std::cout << "\nTolerance tests:" << std::endl;
    std::cout << "1e-9:  " << (std::abs(back_to_double - original) < 1e-9 ? "PASS" : "FAIL") << std::endl;
    std::cout << "1e-8:  " << (std::abs(back_to_double - original) < 1e-8 ? "PASS" : "FAIL") << std::endl;
    std::cout << "1e-7:  " << (std::abs(back_to_double - original) < 1e-7 ? "PASS" : "FAIL") << std::endl;
    std::cout << "1e-6:  " << (std::abs(back_to_double - original) < 1e-6 ? "PASS" : "FAIL") << std::endl;

    return 0;
}