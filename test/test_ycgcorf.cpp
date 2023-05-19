#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/ycgcorf.h"
#include "color/distance.h"

using ColorType = Color::YCgCoRf;

TEST_SUITE("YCgCoRf")

CATCH_TEST_CASE("DefaultConstruction", TEST_SUITE_TAG)
{
    ColorType c0;
    CATCH_REQUIRE(c0.Y() == 0);
    CATCH_REQUIRE(c0.Cg() == 0);
    CATCH_REQUIRE(c0.Co() == 0);
    CATCH_REQUIRE(c0.raw() == ColorType::pixel_type());
}

CATCH_TEST_CASE("Construction", TEST_SUITE_TAG)
{
    ColorType c1(1, 2, 3);
    CATCH_REQUIRE(c1.Y() == 1);
    CATCH_REQUIRE(c1.Cg() == 2);
    CATCH_REQUIRE(c1.Co() == 3);
    CATCH_REQUIRE(c1.x() == 1);
    CATCH_REQUIRE(c1.y() == 2);
    CATCH_REQUIRE(c1.z() == 3);
    CATCH_REQUIRE(c1.raw() == ColorType::pixel_type(1, 2, 3));
}

CATCH_TEST_CASE("Assignment", TEST_SUITE_TAG)
{
    ColorType c1(26, 43, 60);
    ColorType c2(1, 2, 3);
    c2 = c1;
    CATCH_REQUIRE(c2.Y() == c1.Y());
    CATCH_REQUIRE(c2.Cg() == c1.Cg());
    CATCH_REQUIRE(c2.Co() == c1.Co());
    c1.Y() = 5;
    c1.Cg() = 7;
    c1.Co() = 9;
    CATCH_REQUIRE(c1.Y() == 5);
    CATCH_REQUIRE(c1.Cg() == 7);
    CATCH_REQUIRE(c1.Co() == 9);
}

CATCH_TEST_CASE("Access", TEST_SUITE_TAG)
{
    ColorType c1(26, 43, 60);
    ColorType c2(1, 2, 3);
    c2 = c1;
    CATCH_REQUIRE(c2[0] == c1.Y());
    CATCH_REQUIRE(c2[1] == c1.Cg());
    CATCH_REQUIRE(c2[2] == c1.Co());
    c1[0] = 5;
    c1[1] = 7;
    c1[2] = 9;
    CATCH_REQUIRE(c1[0] == 5);
    CATCH_REQUIRE(c1[1] == 7);
    CATCH_REQUIRE(c1[2] == 9);
}

CATCH_TEST_CASE("Normalize", TEST_SUITE_TAG)
{
    ColorType c1(0.25, -0.5, 1);
    auto c2 = c1.normalized();
    CATCH_REQUIRE(c2.Y() == 0.25);
    CATCH_REQUIRE(c2.Cg() == 0.25);
    CATCH_REQUIRE(c2.Co() == 1.0);
}

CATCH_TEST_CASE("Distance", TEST_SUITE_TAG)
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

CATCH_TEST_CASE("RoundToGrid", TEST_SUITE_TAG)
{
    ColorType c0(ColorType::Min[0], ColorType::Min[1], ColorType::Min[2]);
    CATCH_REQUIRE(c0 == ColorType::roundTo(c0, std::array<int, 3>({31, 31, 31})));
    auto c2 = ColorType::roundTo(ColorType(0.1, -0.5, 0.9), std::array<int, 3>({31, 31, 31}));
    CATCH_REQUIRE_THAT(c2.Y(), Catch::Matchers::WithinAbs(0.0967, 0.0001));
    CATCH_REQUIRE_THAT(c2.Cg(), Catch::Matchers::WithinAbs(-0.4838, 0.0001));
    CATCH_REQUIRE_THAT(c2.Co(), Catch::Matchers::WithinAbs(0.8709, 0.0001));
    ColorType c4(ColorType::Max[0], ColorType::Max[1], ColorType::Max[2]);
    CATCH_REQUIRE(c4 == ColorType::roundTo(c4, std::array<int, 3>({31, 31, 31})));
}
