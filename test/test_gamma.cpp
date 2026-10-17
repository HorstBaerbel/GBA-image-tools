#include "testmacros.h"

#include "color/gamma.h"
#include "color/grayf.h"
#include "color/rgbf.h"

#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Color;

TEST_SUITE("Gamma")

TEST_CASE("srgbToLinear black")
{
    RGBf color(0.0F, 0.0F, 0.0F);
    auto linear = srgbToLinear(color);
    CATCH_REQUIRE_THAT(linear[0], Catch::Matchers::WithinAbs(0.0F, 0.0001F));
    CATCH_REQUIRE_THAT(linear[1], Catch::Matchers::WithinAbs(0.0F, 0.0001F));
    CATCH_REQUIRE_THAT(linear[2], Catch::Matchers::WithinAbs(0.0F, 0.0001F));
}

TEST_CASE("srgbToLinear white")
{
    RGBf color(1.0F, 1.0F, 1.0F);
    auto linear = srgbToLinear(color);
    CATCH_REQUIRE_THAT(linear[0], Catch::Matchers::WithinAbs(1.0F, 0.0001F));
    CATCH_REQUIRE_THAT(linear[1], Catch::Matchers::WithinAbs(1.0F, 0.0001F));
    CATCH_REQUIRE_THAT(linear[2], Catch::Matchers::WithinAbs(1.0F, 0.0001F));
}

TEST_CASE("srgbToLinear mid gray")
{
    // 0.5 sRGB should give approximately 0.214 linear
    RGBf color(0.5F, 0.5F, 0.5F);
    auto linear = srgbToLinear(color);
    CATCH_REQUIRE_THAT(linear[0], Catch::Matchers::WithinAbs(0.214F, 0.01F));
    CATCH_REQUIRE_THAT(linear[1], Catch::Matchers::WithinAbs(0.214F, 0.01F));
    CATCH_REQUIRE_THAT(linear[2], Catch::Matchers::WithinAbs(0.214F, 0.01F));
}

TEST_CASE("srgbToLinear small value (linear segment)")
{
    // Values <= 0.04045 use linear segment: c / 12.92
    RGBf color(0.04F, 0.04F, 0.04F);
    auto linear = srgbToLinear(color);
    CATCH_REQUIRE_THAT(linear[0], Catch::Matchers::WithinAbs(0.04F / 12.92F, 1e-6F));
}

TEST_CASE("srgbToLinear boundary value")
{
    // At exactly 0.04045, both formulas should give the same result
    RGBf color(0.04045F, 0.04045F, 0.04045F);
    auto linear = srgbToLinear(color);
    // Linear segment: 0.04045 / 12.92 = 0.003131...
    CATCH_REQUIRE_THAT(linear[0], Catch::Matchers::WithinAbs(0.04045F / 12.92F, 1e-5F));
}

TEST_CASE("srgbToLinear grayf")
{
    Grayf color(0.5F);
    auto linear = srgbToLinear(color);
    // Should produce RGB with all channels equal
    CATCH_REQUIRE_THAT(linear[0], Catch::Matchers::WithinAbs(linear[1], 1e-6F));
    CATCH_REQUIRE_THAT(linear[1], Catch::Matchers::WithinAbs(linear[2], 1e-6F));
    CATCH_REQUIRE_THAT(linear[0], Catch::Matchers::WithinAbs(0.214F, 0.01F));
}

TEST_CASE("linearToSrgb black")
{
    RGBf color(0.0F, 0.0F, 0.0F);
    auto srgb = linearToSrgb(color);
    CATCH_REQUIRE_THAT(srgb[0], Catch::Matchers::WithinAbs(0.0F, 0.0001F));
    CATCH_REQUIRE_THAT(srgb[1], Catch::Matchers::WithinAbs(0.0F, 0.0001F));
    CATCH_REQUIRE_THAT(srgb[2], Catch::Matchers::WithinAbs(0.0F, 0.0001F));
}

TEST_CASE("linearToSrgb white")
{
    RGBf color(1.0F, 1.0F, 1.0F);
    auto srgb = linearToSrgb(color);
    CATCH_REQUIRE_THAT(srgb[0], Catch::Matchers::WithinAbs(1.0F, 0.0001F));
    CATCH_REQUIRE_THAT(srgb[1], Catch::Matchers::WithinAbs(1.0F, 0.0001F));
    CATCH_REQUIRE_THAT(srgb[2], Catch::Matchers::WithinAbs(1.0F, 0.0001F));
}

TEST_CASE("linearToSrgb mid value")
{
    // 0.214 linear should give approximately 0.5 sRGB
    RGBf color(0.214F, 0.214F, 0.214F);
    auto srgb = linearToSrgb(color);
    CATCH_REQUIRE_THAT(srgb[0], Catch::Matchers::WithinAbs(0.5F, 0.01F));
}

TEST_CASE("linearToSrgb small value (linear segment)")
{
    // Values <= 0.0031308 use linear segment: c * 12.92
    RGBf color(0.001F, 0.001F, 0.001F);
    auto srgb = linearToSrgb(color);
    CATCH_REQUIRE_THAT(srgb[0], Catch::Matchers::WithinAbs(0.001F * 12.92F, 1e-6F));
}

TEST_CASE("Roundtrip sRGB -> Linear -> sRGB")
{
    RGBf original(0.3F, 0.6F, 0.9F);
    auto linear = srgbToLinear(original);
    auto roundtrip = linearToSrgb(linear);
    CATCH_REQUIRE_THAT(roundtrip[0], Catch::Matchers::WithinAbs(original[0], 1e-4F));
    CATCH_REQUIRE_THAT(roundtrip[1], Catch::Matchers::WithinAbs(original[1], 1e-4F));
    CATCH_REQUIRE_THAT(roundtrip[2], Catch::Matchers::WithinAbs(original[2], 1e-4F));
}

TEST_CASE("Roundtrip with multiple values")
{
    std::vector<RGBf> colors = {
        RGBf(0.0F, 0.0F, 0.0F),
        RGBf(0.1F, 0.2F, 0.3F),
        RGBf(0.5F, 0.5F, 0.5F),
        RGBf(0.8F, 0.4F, 0.6F),
        RGBf(1.0F, 1.0F, 1.0F)};
    for (const auto &color : colors)
    {
        auto linear = srgbToLinear(color);
        auto roundtrip = linearToSrgb(linear);
        CATCH_REQUIRE_THAT(roundtrip[0], Catch::Matchers::WithinAbs(color[0], 1e-4F));
        CATCH_REQUIRE_THAT(roundtrip[1], Catch::Matchers::WithinAbs(color[1], 1e-4F));
        CATCH_REQUIRE_THAT(roundtrip[2], Catch::Matchers::WithinAbs(color[2], 1e-4F));
    }
}

TEST_CASE("Vector srgbToLinear")
{
    std::vector<RGBf> colors = {
        RGBf(0.0F, 0.0F, 0.0F),
        RGBf(0.5F, 0.5F, 0.5F),
        RGBf(1.0F, 1.0F, 1.0F)};
    auto linear = srgbToLinear(colors);
    CATCH_REQUIRE(linear.size() == 3);
    CATCH_REQUIRE_THAT(linear[0][0], Catch::Matchers::WithinAbs(0.0F, 0.01));
    CATCH_REQUIRE_THAT(linear[1][0], Catch::Matchers::WithinAbs(0.214F, 0.01F));
    CATCH_REQUIRE_THAT(linear[2][0], Catch::Matchers::WithinAbs(1.0F, 0.01));
}

TEST_CASE("Monotonicity srgbToLinear")
{
    // As sRGB value increases, linear value should also increase
    float prevLinear = 0.0F;
    for (float v = 0.0F; v <= 1.0F; v += 0.05F)
    {
        RGBf color(v, v, v);
        auto linear = srgbToLinear(color);
        CATCH_REQUIRE(linear[0] >= prevLinear);
        prevLinear = linear[0];
    }
}

TEST_CASE("Monotonicity linearToSrgb")
{
    // As linear value increases, sRGB value should also increase
    float prevSrgb = 0.0F;
    for (float v = 0.0F; v <= 1.0F; v += 0.05F)
    {
        RGBf color(v, v, v);
        auto srgb = linearToSrgb(color);
        CATCH_REQUIRE(srgb[0] >= prevSrgb);
        prevSrgb = srgb[0];
    }
}
