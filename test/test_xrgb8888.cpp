#include "testmacros.h"

#include "color/xrgb8888.h"
#include "color/distance.h"

using ColorType = Color::XRGB8888;

TEST_SUITE("XRGB8888")

TEST_CASE("DefaultConstruction")
{
    ColorType c0;
    CATCH_REQUIRE(c0.R() == 0);
    CATCH_REQUIRE(c0.G() == 0);
    CATCH_REQUIRE(c0.B() == 0);
    CATCH_REQUIRE(c0.raw() == 0);
}

TEST_CASE("Construction")
{
    ColorType c1(1, 2, 3);
    CATCH_REQUIRE(c1.R() == 1);
    CATCH_REQUIRE(c1.G() == 2);
    CATCH_REQUIRE(c1.B() == 3);
    CATCH_REQUIRE(c1.raw() == 0x00010203); // raw is XRGB
    CATCH_REQUIRE(((decltype(c1)::pixel_type)c1) == c1.raw());
    ColorType c2(0x00123456);
    CATCH_REQUIRE(c2.R() == 18);
    CATCH_REQUIRE(c2.G() == 52);
    CATCH_REQUIRE(c2.B() == 86);
    CATCH_REQUIRE(c2.raw() == 0x00123456); // raw is XRGB
    ColorType c3(c1);
    CATCH_REQUIRE(c3.R() == c1.R());
    CATCH_REQUIRE(c3.G() == c1.G());
    CATCH_REQUIRE(c3.B() == c1.B());
    ColorType c4(std::array<ColorType::value_type, 3>({1, 2, 3}));
    CATCH_REQUIRE(c4.R() == 1);
    CATCH_REQUIRE(c4.G() == 2);
    CATCH_REQUIRE(c4.B() == 3);
    ColorType c5(ColorType::Max);
    CATCH_REQUIRE(static_cast<ColorType::pixel_type>(c5) == 0xFFFFFF);
}

TEST_CASE("OutOfRangeValuesGetZeroed")
{
    ColorType c1(0x12345678);
    CATCH_REQUIRE(c1.R() == 0x34);
    CATCH_REQUIRE(c1.G() == 0x56);
    CATCH_REQUIRE(c1.B() == 0x78);
    CATCH_REQUIRE(c1.raw() == 0x00345678); // raw is XRGB
}

TEST_CASE("Assignment")
{
    ColorType c1(26, 43, 60);
    ColorType c2(0x001A2B3C);
    c2 = c1;
    CATCH_REQUIRE(c2.R() == c1.R());
    CATCH_REQUIRE(c2.G() == c1.G());
    CATCH_REQUIRE(c2.B() == c1.B());
    c2 = 0x00135790;
    CATCH_REQUIRE(c2.R() == 19);
    CATCH_REQUIRE(c2.G() == 87);
    CATCH_REQUIRE(c2.B() == 144);
    c2.R() = 5;
    c2.G() = 7;
    c2.B() = 9;
    CATCH_REQUIRE(c2.R() == 5);
    CATCH_REQUIRE(c2.G() == 7);
    CATCH_REQUIRE(c2.B() == 9);
}

TEST_CASE("Access")
{
    ColorType c1(26, 43, 60);
    ColorType c2(0x001A2B3C);
    c2 = c1;
    CATCH_REQUIRE(c2[0] == c1.R());
    CATCH_REQUIRE(c2[1] == c1.G());
    CATCH_REQUIRE(c2[2] == c1.B());
    c2 = 0x00135790;
    CATCH_REQUIRE(c2[0] == 19);
    CATCH_REQUIRE(c2[1] == 87);
    CATCH_REQUIRE(c2[2] == 144);
    c2[0] = 5;
    c2[1] = 7;
    c2[2] = 9;
    CATCH_REQUIRE(c2[0] == 5);
    CATCH_REQUIRE(c2[1] == 7);
    CATCH_REQUIRE(c2[2] == 9);
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

TEST_CASE("FromHex")
{
    auto c0 = ColorType::fromHex("000000");
    CATCH_REQUIRE(c0.raw() == 0);
    c0 = ColorType::fromHex("#000000");
    CATCH_REQUIRE(c0.raw() == 0);
    auto c1 = ColorType::fromHex("123456");
    CATCH_REQUIRE(c1.R() == 18);
    CATCH_REQUIRE(c1.G() == 52);
    CATCH_REQUIRE(c1.B() == 86);
    CATCH_REQUIRE(c1.raw() == 0x00123456); // raw is XRGB
    auto c2 = ColorType::fromHex("#103050");
    CATCH_REQUIRE(c2.R() == 16);
    CATCH_REQUIRE(c2.G() == 48);
    CATCH_REQUIRE(c2.B() == 80);
    CATCH_REQUIRE(c2.raw() == 0x00103050); // raw is XRGB
    CATCH_REQUIRE_THROWS(ColorType::fromHex("1103050"));
    CATCH_REQUIRE_THROWS(ColorType::fromHex("#1103050"));
    CATCH_REQUIRE_THROWS(ColorType::fromHex("03050"));
    CATCH_REQUIRE_THROWS(ColorType::fromHex("#03050"));
    CATCH_REQUIRE_THROWS(ColorType::fromHex(""));
    CATCH_REQUIRE_THROWS(ColorType::fromHex("#"));
}

TEST_CASE("ToHex")
{
    ColorType c0;
    CATCH_REQUIRE(c0.toHex() == "000000");
    ColorType c1(1, 2, 3);
    CATCH_REQUIRE(c1.toHex() == "010203");
    ColorType c2(99, 88, 77);
    CATCH_REQUIRE(ColorType::toHex(c2) == "63584D");
    auto c3 = ColorType::fromHex(ColorType::toHex(c2));
    CATCH_REQUIRE(c3.raw() == 0x0063584D);
}
