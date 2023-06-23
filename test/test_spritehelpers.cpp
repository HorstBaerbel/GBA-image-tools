#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include "testmacros.h"

#include "processing/spritehelpers.h"

#include <algorithm>
#include <random>
#include <span>
#include <vector>

TEST_SUITE("Sprite helpers")

CATCH_TEST_CASE("convertToWidth", TEST_SUITE_TAG)
{
    std::vector<Color::XRGB8888> v0;
    CATCH_REQUIRE_THROWS(convertToWidth(v0, 5, 8, 8));
    CATCH_REQUIRE_THROWS(convertToWidth(v0, 8, 7, 8));
    CATCH_REQUIRE_THROWS(convertToWidth(v0, 8, 8, 8));
    v0.resize(64);
    CATCH_REQUIRE_THROWS(convertToWidth(v0, 8, 8, 6));
    std::vector<Color::XRGB8888> v1;
    std::generate_n(std::back_inserter(v1), 128, [i = 0]() mutable
                    { return i; });
    CATCH_REQUIRE(v1 == convertToWidth(v1, 16, 8, 16));
    auto v2 = convertToWidth(v1, 16, 8, 8);
    // 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
    // 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
    // ->
    // 00 01 02 03 04 05 06 07 10 11 12 13 14 15 16 17
    // ...
    std::span s1(v1);
    std::span s2(v2);
    for (std::size_t i = 0; i < 56; i += 8)
    {
        CATCH_REQUIRE_THAT(s2.subspan(i, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 64, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i + 8, 8)));
    }
    CATCH_REQUIRE(v2 == convertToTiles(v1, 16, 8, 8, 8));
}

CATCH_TEST_CASE("convertToTiles", TEST_SUITE_TAG)
{
    std::vector<Color::XRGB8888> v0;
    CATCH_REQUIRE_THROWS(convertToTiles(v0, 5, 8));
    CATCH_REQUIRE_THROWS(convertToTiles(v0, 8, 7));
    CATCH_REQUIRE_THROWS(convertToTiles(v0, 8, 8));
    CATCH_REQUIRE_THROWS(convertToTiles(v0, 8, 8, 6, 8));
    CATCH_REQUIRE_THROWS(convertToTiles(v0, 8, 8, 8, 4));
    v0.resize(64);
    CATCH_REQUIRE_THROWS(convertToTiles(v0, 8, 8, 6));
    std::vector<Color::XRGB8888> v1;
    std::generate_n(std::back_inserter(v1), 256, [i = 0]() mutable
                    { return i; });
    CATCH_REQUIRE(v1 == convertToTiles(v1, 16, 16, 16, 16));
    auto v2 = convertToTiles(v1, 16, 16, 8);
    // 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
    // 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
    // ->
    // 00 01 02 03 04 05 06 07 10 11 12 13 14 15 16 17
    // ...
    std::span s1(v1);
    std::span s2(v2);
    for (std::size_t i = 0; i < 112; i += 8)
    {
        CATCH_REQUIRE_THAT(s2.subspan(i, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 64, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i + 8, 8)));
    }
    CATCH_REQUIRE(v2 == convertToWidth(v1, 16, 16, 8));
}

CATCH_TEST_CASE("convertToSprites", TEST_SUITE_TAG)
{
    std::vector<Color::XRGB8888> v0;
    CATCH_REQUIRE_THROWS(convertToSprites(v0, 5, 8, 8, 8));
    CATCH_REQUIRE_THROWS(convertToSprites(v0, 8, 7, 8, 8));
    CATCH_REQUIRE_THROWS(convertToSprites(v0, 8, 8, 6, 8));
    CATCH_REQUIRE_THROWS(convertToSprites(v0, 8, 8, 8, 4));
    CATCH_REQUIRE_THROWS(convertToSprites(v0, 8, 8, 8, 8));
    v0.resize(64);
    CATCH_REQUIRE_THROWS(convertToSprites(v0, 8, 8, 6, 5));
    std::vector<Color::XRGB8888> v1;
    std::generate_n(std::back_inserter(v1), 512, [i = 0]() mutable
                    { return i; });
    auto v2 = convertToSprites(v1, 32, 16, 16, 16);
    // 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
    // 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
    // ->
    // 00 01 02 03 04 05 06 07 10 11 12 13 14 15 16 17
    // ...
    std::span s1(v1);
    std::span s2(v2);
    for (std::size_t i = 0; i < 64; i += 8)
    {
        CATCH_REQUIRE_THAT(s2.subspan(i + 0 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(i + 0, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 1 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(i + 8, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 2 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i + 0, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 3 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i + 8, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 4 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(i + 16, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 5 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(i + 24, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 6 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i + 16, 8)));
        CATCH_REQUIRE_THAT(s2.subspan(i + 7 * 64, 8), Catch::Matchers::RangeEquals(s1.subspan(2 * i + 24, 8)));
    }
    CATCH_REQUIRE(v2 == convertToTiles(convertToWidth(v1, 32, 16, 16), 16, 32, 8));
}
