#include "testmacros.h"

#include "color/colorhelpers.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace ColorHelpers;

TEST_SUITE("Color helpers")

TEST_CASE("addColorAtIndex0")
{
    std::vector<Color::XRGB8888> v1;
    v1 = addColorAtIndex0(v1, Color::XRGB8888(1));
    CATCH_REQUIRE(v1.size() == 1);
    CATCH_REQUIRE(v1.front() == Color::XRGB8888(1));
    v1 = addColorAtIndex0(v1, Color::XRGB8888(2));
    CATCH_REQUIRE(v1.size() == 2);
    CATCH_REQUIRE(v1.front() == Color::XRGB8888(2));
    CATCH_REQUIRE(v1.back() == Color::XRGB8888(1));
}

TEST_CASE("swapColors")
{
    std::vector<Color::XRGB8888> v1 = {Color::XRGB8888(1), Color::XRGB8888(2), Color::XRGB8888(3)};
    std::vector<uint8_t> i1 = {1, 2, 0};
    auto v2 = swapColors(v1, i1);
    CATCH_REQUIRE(v2.size() == v1.size());
    CATCH_REQUIRE(v2[0] == v1[1]);
    CATCH_REQUIRE(v2[1] == v1[2]);
    CATCH_REQUIRE(v2[2] == v1[0]);
}
