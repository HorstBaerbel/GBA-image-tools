#include "testmacros.h"

#include "processing/datahelpers.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace DataHelpers;

TEST_SUITE("Data helpers")

TEST_CASE("fillUpToMultipleOf")
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

TEST_CASE("combineTo")
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

TEST_CASE("convertTo")
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

TEST_CASE("getStartIndices")
{
    std::vector<std::vector<uint8_t>> v0;
    CATCH_REQUIRE(getStartIndices(v0).empty());
    std::vector<std::vector<uint8_t>> v1 = {{1, 2}, {4, 5, 6}, {1}, {}, {3, 4, 5, 6}};
    auto i1 = getStartIndices(v1);
    CATCH_REQUIRE(i1 == std::vector<decltype(i1)::value_type>({0, 2, 5, 6, 6}));
}

TEST_CASE("divideBy")
{
    std::vector<uint8_t> v0;
    CATCH_REQUIRE(divideBy(v0, uint8_t(4)).empty());
    std::vector<uint32_t> v1 = {1, 2, 3, 4, 5, 6, 7, 8};
    auto v2 = divideBy(v1, uint32_t(4));
    CATCH_REQUIRE(v2 == std::vector<decltype(v2)::value_type>({0, 0, 0, 1, 1, 1, 1, 2}));
}

TEST_CASE("interleave")
{
    std::vector<std::vector<uint8_t>> va = {{0x12, 0x23, 0x34}, {0x45, 0x67}};
    std::vector<std::vector<uint8_t>> vb = {{0x12, 0x23, 0x34}, {0x45, 0x67}};
    CATCH_REQUIRE_THROWS(interleave(va, 4));
    CATCH_REQUIRE_THROWS(interleave(vb, 4));
    std::vector<std::vector<uint8_t>> v1 = {{0x12, 0x23, 0x34}, {0x45, 0x67, 0x89}};
    CATCH_REQUIRE_THROWS(interleave(v1, 0));
    CATCH_REQUIRE_THROWS(interleave(v1, 1));
    CATCH_REQUIRE_THROWS(interleave(v1, 2));
    CATCH_REQUIRE_THROWS(interleave(v1, 3));
    CATCH_REQUIRE_THROWS(interleave(v1, 5));
    CATCH_REQUIRE_THROWS(interleave(v1, 6));
    CATCH_REQUIRE_THROWS(interleave(v1, 7));
    CATCH_REQUIRE_THROWS(interleave(v1, 9));
    CATCH_REQUIRE_THROWS(interleave(v1, 10));
    CATCH_REQUIRE_THROWS(interleave(v1, 11));
    CATCH_REQUIRE_THROWS(interleave(v1, 12));
    CATCH_REQUIRE_THROWS(interleave(v1, 13));
    CATCH_REQUIRE_THROWS(interleave(v1, 14));
    CATCH_REQUIRE_THROWS(interleave(v1, 17));
    auto v2 = interleave(v1, 4);
    CATCH_REQUIRE(v2 == std::vector<decltype(v2)::value_type>({0x52, 0x41, 0x73, 0x62, 0x94, 0x83}));
    auto v3 = interleave(v1, 8);
    CATCH_REQUIRE(v3 == std::vector<decltype(v3)::value_type>({0x12, 0x45, 0x23, 0x67, 0x34, 0x89}));
    CATCH_REQUIRE_THROWS(interleave(v1, 15));
    CATCH_REQUIRE_THROWS(interleave(v1, 16));
    std::vector<std::vector<uint8_t>> v4 = {{0x12, 0x23, 0x34, 0x56}, {0x45, 0x67, 0x89, 0x01}};
    auto v5 = interleave(v4, 15);
    CATCH_REQUIRE(v5 == std::vector<decltype(v5)::value_type>({0x12, 0x23, 0x45, 0x67, 0x34, 0x56, 0x89, 0x01}));
    auto v6 = interleave(v4, 16);
    CATCH_REQUIRE(v5 == v6);
}

template <typename T>
auto generate_n(std::size_t n = 100000) -> std::vector<T>
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    std::vector<T> result;
    std::generate_n(std::back_inserter(result), n, [&dist, &mt]()
                    { return dist(mt); });
    return result;
}

TEST_CASE("deltaEncode")
{
    std::vector<uint8_t> v0 = {1, 2, 56, 44, 7, 10, 0, 0};
    auto v1 = deltaEncode(v0);
    CATCH_REQUIRE(v1 == std::vector<decltype(v1)::value_type>({1, 1, 54, 256 - 12, 256 - 37, 3, 256 - 10, 0}));
    auto v2 = deltaDecode(v1);
    CATCH_REQUIRE(v2 == v0);
    // unsigned
    auto v3 = generate_n<uint8_t>();
    CATCH_REQUIRE(v3 == deltaDecode(deltaEncode(v3)));
    auto v4 = generate_n<uint16_t>();
    CATCH_REQUIRE(v4 == deltaDecode(deltaEncode(v4)));
    auto v5 = generate_n<uint32_t>();
    CATCH_REQUIRE(v5 == deltaDecode(deltaEncode(v5)));
    // signed
    auto v6 = generate_n<int8_t>();
    CATCH_REQUIRE(v6 == deltaDecode(deltaEncode(v6)));
    auto v7 = generate_n<int16_t>();
    CATCH_REQUIRE(v7 == deltaDecode(deltaEncode(v7)));
    auto v8 = generate_n<int32_t>();
    CATCH_REQUIRE(v8 == deltaDecode(deltaEncode(v8)));
}

TEST_CASE("prependValue")
{
    std::vector<uint8_t> v0;
    CATCH_REQUIRE(prependValue(v0, uint8_t(123)) == std::vector<decltype(v0)::value_type>({123}));
    std::vector<uint8_t> v1 = {1, 2};
    CATCH_REQUIRE(prependValue(v1, uint8_t(3)) == std::vector<decltype(v1)::value_type>({3, 1, 2}));
    std::vector<uint8_t> v2 = {5, 6};
    CATCH_REQUIRE(prependValue(v2, uint16_t(0x1234)) == std::vector<decltype(v2)::value_type>({0x34, 0x12, 5, 6}));
    std::vector<uint8_t> v3 = {9, 0};
    CATCH_REQUIRE(prependValue(v3, uint32_t(0x12345678)) == std::vector<decltype(v3)::value_type>({0x78, 0x56, 0x34, 0x12, 9, 0}));
}
