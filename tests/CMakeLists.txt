# CMakeLists.txt for tests
# Add your test configurations here

add_executable(test_calculator unit/test_calculator.cpp ../src/calculator.cpp)
target_include_directories(test_calculator PRIVATE ../include)
target_link_libraries(test_calculator)  # No special libs needed

add_test(NAME test_calculator COMMAND test_calculator)