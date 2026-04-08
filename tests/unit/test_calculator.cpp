#include <iostream>
#include <cassert>
#include "myproject/calculator.h"

void test_add() {
    assert(myproject::Calculator::add(2.0, 3.0) == 5.0);
    assert(myproject::Calculator::add(-2.0, -3.0) == -5.0);
    std::cout << "test_add passed" << std::endl;
}

void test_subtract() {
    assert(myproject::Calculator::subtract(5.0, 3.0) == 2.0);
    assert(myproject::Calculator::subtract(3.0, 5.0) == -2.0);
    std::cout << "test_subtract passed" << std::endl;
}

void test_multiply() {
    assert(myproject::Calculator::multiply(2.0, 3.0) == 6.0);
    assert(myproject::Calculator::multiply(5.0, 0.0) == 0.0);
    std::cout << "test_multiply passed" << std::endl;
}

void test_divide() {
    assert(myproject::Calculator::divide(6.0, 2.0) == 3.0);
    bool exception_thrown = false;
    try {
        myproject::Calculator::divide(5.0, 0.0);
    } catch (const std::runtime_error&) {
        exception_thrown = true;
    }
    assert(exception_thrown);
    std::cout << "test_divide passed" << std::endl;
}

int main() {
    test_add();
    test_subtract();
    test_multiply();
    test_divide();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}