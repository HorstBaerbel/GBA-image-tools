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
    auto v2 = convertToWidth(v1, 16, 8, 8);
    // 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
    // 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
    // ->
    // 00 01 02 03 04 05 06 07 10 11 12 13 14 15 16 17
    // ...
    std::span s1(v1);
    std::span s2(v2);
    CATCH_REQUIRE_THAT(s2.subspan(0, 8), Catch::Matchers::RangeEquals(s1.subspan(0, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(8, 8), Catch::Matchers::RangeEquals(s1.subspan(16, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(16, 8), Catch::Matchers::RangeEquals(s1.subspan(32, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(24, 8), Catch::Matchers::RangeEquals(s1.subspan(48, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(32, 8), Catch::Matchers::RangeEquals(s1.subspan(64, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(40, 8), Catch::Matchers::RangeEquals(s1.subspan(80, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(48, 8), Catch::Matchers::RangeEquals(s1.subspan(96, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(56, 8), Catch::Matchers::RangeEquals(s1.subspan(112, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 0, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 0, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 8, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 16, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 16, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 32, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 24, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 48, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 32, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 64, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 40, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 80, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 48, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 96, 8)));
    CATCH_REQUIRE_THAT(s2.subspan(64 + 56, 8), Catch::Matchers::RangeEquals(s1.subspan(8 + 112, 8)));
}
