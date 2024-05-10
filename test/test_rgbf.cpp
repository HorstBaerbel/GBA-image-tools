#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/rgbf.h"
#include "color/distance.h"

using ColorType = Color::RGBf;

TEST_SUITE("RGBf")

TEST_CASE("DefaultConstruction")
{
    ColorType c0;
    CATCH_REQUIRE(c0.R() == 0);
    CATCH_REQUIRE(c0.G() == 0);
    CATCH_REQUIRE(c0.B() == 0);
    CATCH_REQUIRE(c0 == ColorType::pixel_type(0, 0, 0));
}

TEST_CASE("Construction")
{
    ColorType c1(1, 2, 3);
    CATCH_REQUIRE(c1.R() == 1);
    CATCH_REQUIRE(c1.G() == 2);
    CATCH_REQUIRE(c1.B() == 3);
    CATCH_REQUIRE(c1.x() == 1);
    CATCH_REQUIRE(c1.y() == 2);
    CATCH_REQUIRE(c1.z() == 3);
    CATCH_REQUIRE(c1 == ColorType::pixel_type(1, 2, 3));
}

TEST_CASE("Assignment")
{
    ColorType c1(26, 43, 60);
    ColorType c2(1, 2, 3);
    c2 = c1;
    CATCH_REQUIRE(c2.R() == c1.R());
    CATCH_REQUIRE(c2.G() == c1.G());
    CATCH_REQUIRE(c2.B() == c1.B());
    c1.R() = 5;
    c1.G() = 7;
    c1.B() = 9;
    CATCH_REQUIRE(c1.R() == 5);
    CATCH_REQUIRE(c1.G() == 7);
    CATCH_REQUIRE(c1.B() == 9);
}

TEST_CASE("Access")
{
    ColorType c1(26, 43, 60);
    ColorType c2(1, 2, 3);
    c2 = c1;
    CATCH_REQUIRE(c2[0] == c1.R());
    CATCH_REQUIRE(c2[1] == c1.G());
    CATCH_REQUIRE(c2[2] == c1.B());
    c1[0] = 5;
    c1[1] = 7;
    c1[2] = 9;
    CATCH_REQUIRE(c1.R() == 5);
    CATCH_REQUIRE(c1.G() == 7);
    CATCH_REQUIRE(c1.B() == 9);
}

TEST_CASE("SwapRB")
{
    ColorType c1(15, 7, 22);
    auto c2 = c1.swappedRB();
    CATCH_REQUIRE(c2.R() == 22);
    CATCH_REQUIRE(c2.G() == 7);
    CATCH_REQUIRE(c2.B() == 15);
}

TEST_CASE("Distance")
{
    ColorType c0(ColorType::Min[0], ColorType::Min[1], ColorType::Min[2]);
    ColorType c1(ColorType::Max[0], ColorType::Max[1], ColorType::Max[2]);
    ColorType c2(ColorType::Max[0], ColorType::Max[1], ColorType::Max[2]);
    ColorType c3(ColorType::Min[0], ColorType::Min[1], ColorType::Min[2]);
    auto d1 = ColorType::distance(c0, c1);
    CATCH_REQUIRE(d1 == 1.0F);
    auto d2 = ColorType::distance(c1, c0);
    CATCH_REQUIRE(d1 == d2);
    auto d3 = ColorType::distance(c1, c2);
    CATCH_REQUIRE(d3 == 0.0F);
    auto d4 = ColorType::distance(c2, c1);
    CATCH_REQUIRE(d3 == d4);
    auto d5 = ColorType::distance(c0, c3);
    CATCH_REQUIRE(d5 == 0.0F);
    auto d6 = ColorType::distance(c3, c0);
    CATCH_REQUIRE(d5 == d6);
}

TEST_CASE("RoundToGrid")
{
    ColorType c0(ColorType::Min[0], ColorType::Min[1], ColorType::Min[2]);
    CATCH_REQUIRE(c0 == ColorType::roundTo(c0, std::array<int, 3>({31, 31, 31})));
    auto c2 = ColorType::roundTo(ColorType(0.1, 0.5, 0.9), std::array<int, 3>({31, 31, 31}));
    CATCH_REQUIRE_THAT(c2.R(), Catch::Matchers::WithinAbs(0.0967, 0.0001));
    CATCH_REQUIRE_THAT(c2.G(), Catch::Matchers::WithinAbs(0.5161, 0.0001));
    CATCH_REQUIRE_THAT(c2.B(), Catch::Matchers::WithinAbs(0.9032, 0.0001));
    ColorType c4(ColorType::Max[0], ColorType::Max[1], ColorType::Max[2]);
    CATCH_REQUIRE(c4 == ColorType::roundTo(c4, std::array<int, 3>({31, 31, 31})));
}
