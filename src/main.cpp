#include <iostream>
#include "myproject/calculator.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    // Calculator demonstration
    double a = 10.0, b = 5.0;

    std::cout << "Calculator Demo:" << std::endl;
    std::cout << a << " + " << b << " = " << myproject::Calculator::add(a, b) << std::endl;
    std::cout << a << " - " << b << " = " << myproject::Calculator::subtract(a, b) << std::endl;
    std::cout << a << " * " << b << " = " << myproject::Calculator::multiply(a, b) << std::endl;
    std::cout << a << " / " << b << " = " << myproject::Calculator::divide(a, b) << std::endl;

    // Test division by zero
    try {
        myproject::Calculator::divide(a, 0.0);
    } catch (const std::runtime_error& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    return 0;
}