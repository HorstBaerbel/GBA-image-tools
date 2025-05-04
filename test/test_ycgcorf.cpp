#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/psnr.h"
#include "color/ycgcorf.h"

using ColorType = Color::YCgCoRf;

TEST_SUITE("YCgCoRf")

TEST_CASE("DefaultConstruction")
{
    ColorType c0;
    CATCH_REQUIRE(c0.Y() == 0);
    CATCH_REQUIRE(c0.Cg() == 0);
    CATCH_REQUIRE(c0.Co() == 0);
    CATCH_REQUIRE(c0 == ColorType::pixel_type(0, 0, 0));
}

TEST_CASE("Construction")
{
    ColorType c1(1, 2, 3);
    CATCH_REQUIRE(c1.Y() == 1);
    CATCH_REQUIRE(c1.Cg() == 2);
    CATCH_REQUIRE(c1.Co() == 3);
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

TEST_CASE("Access")
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

TEST_CASE("Normalize")
{
    ColorType c1(0.25, -0.5, 1);
    auto c2 = c1.normalized();
    CATCH_REQUIRE(c2.Y() == 0.25);
    CATCH_REQUIRE(c2.Cg() == 0.25);
    CATCH_REQUIRE(c2.Co() == 1.0);
}

TEST_CASE("Distance")
{
    ColorType c0(ColorType::Min[0], ColorType::Min[1], ColorType::Min[2]);
    ColorType c1(ColorType::Max[0], ColorType::Max[1], ColorType::Max[2]);
    ColorType c2(ColorType::Max[0], ColorType::Max[1], ColorType::Max[2]);
    ColorType c3(ColorType::Min[0], ColorType::Min[1], ColorType::Min[2]);
    CATCH_REQUIRE_THAT(ColorType::mse(c0, c1), Catch::Matchers::WithinAbs(1.0, 0.0001));
    CATCH_REQUIRE_THAT(ColorType::mse(c1, c0), Catch::Matchers::WithinAbs(1.0, 0.0001));
    CATCH_REQUIRE(ColorType::mse(c1, c2) == 0.0F);
    CATCH_REQUIRE(ColorType::mse(c2, c1) == 0.0F);
    CATCH_REQUIRE(ColorType::mse(c0, c3) == 0.0F);
    CATCH_REQUIRE(ColorType::mse(c3, c0) == 0.0F);
    ColorType c4(0.5F, 0.0F, 0.0F);
    CATCH_REQUIRE_THAT(ColorType::mse(c0, c4), Catch::Matchers::WithinAbs(0.25, 0.0001));
    CATCH_REQUIRE_THAT(ColorType::mse(c4, c0), Catch::Matchers::WithinAbs(0.25, 0.0001));
    CATCH_REQUIRE_THAT(ColorType::mse(c4, c1), Catch::Matchers::WithinAbs(0.25, 0.0001));
    CATCH_REQUIRE_THAT(ColorType::mse(c1, c4), Catch::Matchers::WithinAbs(0.25, 0.0001));
}

TEST_CASE("RoundToGrid")
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
