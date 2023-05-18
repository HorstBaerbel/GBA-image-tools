#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/conversions.h"
#include "color/lchf.h"
#include "color/rgb565.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"

TEST_SUITE("Conversions")

CATCH_TEST_CASE("RGB565", TEST_SUITE_TAG)
{
    Color::RGB565 c00;
    Color::RGB565 c01(10, 14, 30);
    Color::RGB565 c02(Color::RGB565::Max);
    // XRGB1555
    CATCH_REQUIRE(Color::convertTo<Color::XRGB1555>(c00) == Color::XRGB1555(Color::XRGB1555::Min));
    CATCH_REQUIRE(Color::convertTo<Color::XRGB1555>(c02) == Color::XRGB1555(Color::XRGB1555::Max));
    auto c11 = Color::convertTo<Color::XRGB1555>(c01);
    CATCH_REQUIRE(c11.R() == 10);
    CATCH_REQUIRE(c11.G() == 7);
    CATCH_REQUIRE(c11.B() == 30);
    // XRGB8888
    CATCH_REQUIRE(Color::convertTo<Color::XRGB8888>(c00) == Color::XRGB8888(Color::XRGB8888::Min));
    CATCH_REQUIRE(Color::convertTo<Color::XRGB8888>(c02) == Color::XRGB8888(Color::XRGB8888::Max));
    auto c21 = Color::convertTo<Color::XRGB8888>(c01);
    CATCH_REQUIRE(static_cast<uint32_t>(c21.R()) == 82);
    CATCH_REQUIRE(static_cast<uint32_t>(c21.G()) == 57);
    CATCH_REQUIRE(static_cast<uint32_t>(c21.B()) == 247);
}
