#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/grayf.h"
#include "color/distance.h"

using ColorType = Color::Grayf;

TEST_SUITE("Grayf")

TEST_CASE("DefaultConstruction")
{
    ColorType c0;
    CATCH_REQUIRE(c0.raw() == ColorType::pixel_type());
    CATCH_REQUIRE(c0.I() == ColorType::pixel_type());
}

TEST_CASE("Construction")
{
    ColorType c1(0.5F);
    CATCH_REQUIRE(c1.raw() == 0.5F);
    CATCH_REQUIRE(c1.I() == 0.5F);
}

TEST_CASE("Assignment")
{
    ColorType c1(0.1F);
    ColorType c2(0.2F);
    c2 = c1;
    CATCH_REQUIRE(c2.raw() == 0.1F);
    CATCH_REQUIRE(c2.I() == 0.1F);
    c1.I() = 5;
    CATCH_REQUIRE(c1.raw() == 5);
    CATCH_REQUIRE(c1.I() == 5);
}

TEST_CASE("Access")
{
    ColorType c1(0.1F);
    ColorType c2(0.2F);
    c2 = c1;
    CATCH_REQUIRE(c2[0] == c1.I());
    c1[0] = 5;
    CATCH_REQUIRE(c1.I() == 5);
}

TEST_CASE("Distance")
{
    ColorType c0(ColorType::Min[0]);
    ColorType c1(ColorType::Max[0]);
    auto d1 = ColorType::distance(c0, c1);
    CATCH_REQUIRE(d1 == 1.0F);
    auto d2 = ColorType::distance(c1, c0);
    CATCH_REQUIRE(d1 == d2);
    auto d3 = ColorType::distance(c0, c0);
    CATCH_REQUIRE(d3 == 0.0F);
}

TEST_CASE("RoundToGrid")
{
    ColorType c0(ColorType::Min[0]);
    CATCH_REQUIRE(c0 == ColorType::roundTo(c0, std::array<float, 1>({31})));
    auto c2 = ColorType::roundTo(ColorType(0.1), std::array<float, 1>({31}));
    CATCH_REQUIRE_THAT(c2.I(), Catch::Matchers::WithinAbs(0.0967, 0.0001));
    ColorType c4(ColorType::Max[0]);
    CATCH_REQUIRE(c4 == ColorType::roundTo(c4, std::array<float, 1>({31})));
}
