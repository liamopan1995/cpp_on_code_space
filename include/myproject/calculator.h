#pragma once

#include <stdexcept>

namespace myproject {

class Calculator {
public:
    static double add(double a, double b);
    static double subtract(double a, double b);
    static double multiply(double a, double b);
    static double divide(double a, double b);
};

} // namespace myproject