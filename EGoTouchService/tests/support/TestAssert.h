#pragma once

#include <iostream>
#include <string_view>

#define REQUIRE_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                      << " expected true: " #expr << std::endl; \
            return false; \
        } \
    } while (false)

#define REQUIRE_EQ(actual, expected) \
    do { \
        const auto actualValue = (actual); \
        const auto expectedValue = (expected); \
        if (!(actualValue == expectedValue)) { \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                      << " expected " #actual " == " #expected \
                      << " (actual=" << actualValue << ", expected=" << expectedValue << ")" \
                      << std::endl; \
            return false; \
        } \
    } while (false)

inline int RunTest(bool (*testFn)(), std::string_view name) {
    if (!testFn()) {
        std::cerr << "[FAIL] " << name << std::endl;
        return 1;
    }
    std::cout << "[PASS] " << name << std::endl;
    return 0;
}
