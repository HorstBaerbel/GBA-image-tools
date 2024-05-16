#include "testmacros.h"

#include "color/psnr.h"
#include "color/xrgb1555.h"

using ColorType = Color::XRGB1555;

TEST_SUITE("XRGB1555")

TEST_CASE("DefaultConstruction")
{
    ColorType c0;
    CATCH_REQUIRE(c0.R() == 0);
    CATCH_REQUIRE(c0.G() == 0);
    CATCH_REQUIRE(c0.B() == 0);
    CATCH_REQUIRE(c0 == 0);
}

TEST_CASE("Construction")
{
    ColorType c1(1, 2, 3);
    CATCH_REQUIRE(c1.R() == 1);
    CATCH_REQUIRE(c1.G() == 2);
    CATCH_REQUIRE(c1.B() == 3);
    CATCH_REQUIRE(c1 == uint16_t(0b0000010001000011)); // raw is XRGB
    CATCH_REQUIRE(((decltype(c1)::pixel_type)c1) == static_cast<uint16_t>(c1));
    ColorType c2(uint16_t(0b0101000010001100));
    CATCH_REQUIRE(c2.R() == 20);
    CATCH_REQUIRE(c2.G() == 4);
    CATCH_REQUIRE(c2.B() == 12);
    CATCH_REQUIRE(c2 == uint16_t(0b0101000010001100)); // raw is XRGB
    ColorType c3(c1);
    CATCH_REQUIRE(c3.R() == c1.R());
    CATCH_REQUIRE(c3.G() == c1.G());
    CATCH_REQUIRE(c3.B() == c1.B());
    ColorType c4(std::array<ColorType::value_type, 3>({1, 2, 3}));
    CATCH_REQUIRE(c4.R() == 1);
    CATCH_REQUIRE(c4.G() == 2);
    CATCH_REQUIRE(c4.B() == 3);
    ColorType c5(ColorType::Max);
    CATCH_REQUIRE(static_cast<ColorType::pixel_type>(c5) == 0x7FFF);
}

TEST_CASE("Assignment")
{
    ColorType c1(15, 7, 22);
    ColorType c2(uint16_t(0x6178));
    c2 = c1;
    CATCH_REQUIRE(c2.R() == c1.R());
    CATCH_REQUIRE(c2.G() == c1.G());
    CATCH_REQUIRE(c2.B() == c1.B());
    c2 = uint16_t(0x1753);
    CATCH_REQUIRE(c2.R() == 5);
    CATCH_REQUIRE(c2.G() == 26);
    CATCH_REQUIRE(c2.B() == 19);
}

TEST_CASE("Access")
{
    ColorType c1(15, 7, 22);
    ColorType c2(uint16_t(0x6178));
    c2 = c1;
    CATCH_REQUIRE(c2[0] == c1.R());
    CATCH_REQUIRE(c2[1] == c1.G());
    CATCH_REQUIRE(c2[2] == c1.B());
    c2 = uint16_t(0x1753);
    CATCH_REQUIRE(c2[0] == 5);
    CATCH_REQUIRE(c2[1] == 26);
    CATCH_REQUIRE(c2[2] == 19);
}

TEST_CASE("SwapRB")
{
    ColorType c1(15, 7, 22);
    auto c2 = c1.swapToBGR();
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
    auto d1 = ColorType::mse(c0, c1);
    CATCH_REQUIRE(d1 == 1.0F);
    auto d2 = ColorType::mse(c1, c0);
    CATCH_REQUIRE(d1 == d2);
    auto d3 = ColorType::mse(c1, c2);
    CATCH_REQUIRE(d3 == 0.0F);
    auto d4 = ColorType::mse(c2, c1);
    CATCH_REQUIRE(d3 == d4);
    auto d5 = ColorType::mse(c0, c3);
    CATCH_REQUIRE(d5 == 0.0F);
    auto d6 = ColorType::mse(c3, c0);
    CATCH_REQUIRE(d5 == d6);
}
