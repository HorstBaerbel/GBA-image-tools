#pragma once

#include <catch2/catch_test_macros.hpp>
#include <string>

#define SCAT_NX(A, B) A##B
#define SCAT(A, B) SCAT_NX(A, B)

#define TEST_SUITE(a)                                           \
    static const inline std::string TEST_SUITE_TAG = "[" a "]"; \
    static const inline std::string TEST_SUITE_PREFIX = a "::";
#define TEST_CASE(b) CATCH_TEST_CASE(TEST_SUITE_PREFIX + b, TEST_SUITE_TAG)
