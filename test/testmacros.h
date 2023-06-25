#pragma once

#include <catch2/catch_test_macros.hpp>
#include <string>

// Define the test suite name. Call before yout first test.
// It will also define the test prefix name + "::"
// e.g. TEST_SUITE("Foo") -> suite name "Foo", all tests prefixed with "Foo::"
#define TEST_SUITE(a)                                           \
    static const inline std::string TEST_SUITE_TAG = "[" a "]"; \
    static const inline std::string TEST_SUITE_PREFIX = a "::";

// Define a Catch2 test case. Will be prefixed with test suite name + "::", e.g.
// TEST_SUITE("Foo"); TEST_CASE("bar"); -> Test name "Foo::bar"
#define TEST_CASE(b) CATCH_TEST_CASE(TEST_SUITE_PREFIX + b, TEST_SUITE_TAG)
