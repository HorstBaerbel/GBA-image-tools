#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "processing/datahelpers.h"

#include <array>

TEST_SUITE("Data helpers")

CATCH_TEST_CASE("fillUpToMultipleOf", TEST_SUITE_TAG)
{
    std::vector<uint8_t> v1;
    v1 = fillUpToMultipleOf(v1, 4);
    CATCH_REQUIRE(v1.size() == 0);
    v1.resize(1);
    v1 = fillUpToMultipleOf(v1, 4);
    CATCH_REQUIRE(v1.size() == 4);
    v1 = fillUpToMultipleOf(v1, 4);
    CATCH_REQUIRE(v1.size() == 4);
    v1 = fillUpToMultipleOf(v1, 3);
    CATCH_REQUIRE(v1.size() == 6);
    v1 = fillUpToMultipleOf(v1, 4, uint8_t(123));
    CATCH_REQUIRE(v1.size() == 8);
    CATCH_REQUIRE(v1[6] == 123);
    CATCH_REQUIRE(v1[7] == 123);
    std::vector<uint32_t> v2 = {1, 2, 3};
    v2 = fillUpToMultipleOf(v2, 4, uint32_t(4));
    CATCH_REQUIRE(v2.size() == 4);
    CATCH_REQUIRE(v2[3] == 4);
}

CATCH_TEST_CASE("combineTo", TEST_SUITE_TAG)
{
    std::vector<std::vector<uint8_t>> v1 = {{1, 2, 3}, {4, 5, 6}};
    auto v8 = combineTo<uint8_t>(v1);
    CATCH_REQUIRE(v8 == std::vector<decltype(v8)::value_type>({1, 2, 3, 4, 5, 6}));
    CATCH_REQUIRE_THROWS(combineTo<uint16_t>(v1));
    CATCH_REQUIRE_THROWS(combineTo<uint32_t>(v1));
    std::vector<std::vector<uint8_t>> v2 = {{1, 2, 3, 4}, {5, 6}};
    auto v16 = combineTo<uint16_t>(v2);
    CATCH_REQUIRE(v16 == std::vector<decltype(v16)::value_type>({0x0201, 0x0403, 0x0605}));
    std::vector<std::vector<uint8_t>> v3 = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    v16 = combineTo<uint16_t>(v3);
    CATCH_REQUIRE(v16 == std::vector<decltype(v16)::value_type>({0x0201, 0x0403, 0x0605, 0x0807}));
    auto v32 = combineTo<uint32_t>(v3);
    CATCH_REQUIRE(v32 == std::vector<decltype(v32)::value_type>({0x04030201, 0x08070605}));
}

CATCH_TEST_CASE("convertTo", TEST_SUITE_TAG)
{
    std::vector<uint8_t> v0 = {1, 2, 3, 4, 5};
    CATCH_REQUIRE_THROWS(convertTo<uint16_t>(v0));
    CATCH_REQUIRE_THROWS(convertTo<uint32_t>(v0));
    std::vector<uint8_t> v1 = {1, 2, 3, 4, 5, 6};
    auto v16 = convertTo<uint16_t>(v1);
    CATCH_REQUIRE(v16 == std::vector<decltype(v16)::value_type>({0x0201, 0x0403, 0x0605}));
    CATCH_REQUIRE_THROWS(convertTo<uint32_t>(v1));
    std::vector<uint8_t> v2 = {1, 2, 3, 4, 5, 6, 7, 8};
    v16 = convertTo<uint16_t>(v2);
    CATCH_REQUIRE(v16 == std::vector<decltype(v16)::value_type>({0x0201, 0x0403, 0x0605, 0x0807}));
    auto v32 = convertTo<uint32_t>(v2);
    CATCH_REQUIRE(v32 == std::vector<decltype(v32)::value_type>({0x04030201, 0x08070605}));
}

CATCH_TEST_CASE("getStartIndices", TEST_SUITE_TAG)
{
    std::vector<std::vector<uint8_t>> v0;
    CATCH_REQUIRE(getStartIndices(v0).empty());
    std::vector<std::vector<uint8_t>> v1 = {{1, 2}, {4, 5, 6}, {1}, {}, {3, 4, 5, 6}};
    auto i1 = getStartIndices(v1);
    CATCH_REQUIRE(i1 == std::vector<decltype(i1)::value_type>({0, 2, 5, 6, 6}));
}

CATCH_TEST_CASE("divideBy", TEST_SUITE_TAG)
{
    std::vector<uint8_t> v0;
    CATCH_REQUIRE(divideBy(v0, uint8_t(4)).empty());
    std::vector<uint32_t> v1 = {1, 2, 3, 4, 5, 6, 7, 8};
    auto v2 = divideBy(v1, uint32_t(4));
    CATCH_REQUIRE(v2 == std::vector<decltype(v2)::value_type>({0, 0, 0, 1, 1, 1, 1, 2}));
}
