#include "testmacros.h"

#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "color/cielabf.h"

using namespace Color;

TEST_SUITE("CIELabf")

TEST_CASE("Default constructor")
{
    CIELabf color;
    CATCH_REQUIRE(color.L() == 0.0F);
    CATCH_REQUIRE(color.a() == 0.0F);
    CATCH_REQUIRE(color.b() == 0.0F);
}

TEST_CASE("Constructor with values")
{
    CIELabf color(50.0F, 20.0F, -30.0F);
    CATCH_REQUIRE(color.L() == 50.0F);
    CATCH_REQUIRE(color.a() == 20.0F);
    CATCH_REQUIRE(color.b() == -30.0F);
}

TEST_CASE("Constructor from array")
{
    std::array<float, 3> arr{10.0F, 5.0F, -5.0F};
    CIELabf color(arr);
    CATCH_REQUIRE(color.L() == 10.0F);
    CATCH_REQUIRE(color.a() == 5.0F);
    CATCH_REQUIRE(color.b() == -5.0F);
}

TEST_CASE("Mutable accessors")
{
    CIELabf color(10.0F, 20.0F, 30.0F);
    color.L() = 50.0F;
    color.a() = -10.0F;
    color.b() = 40.0F;
    CATCH_REQUIRE(color.L() == 50.0F);
    CATCH_REQUIRE(color.a() == -10.0F);
    CATCH_REQUIRE(color.b() == 40.0F);
}

TEST_CASE("Min and Max constants")
{
    CATCH_REQUIRE(CIELabf::Min[0] == 0.0F);
    CATCH_REQUIRE(CIELabf::Min[1] == -128.0F);
    CATCH_REQUIRE(CIELabf::Min[2] == -128.0F);
    CATCH_REQUIRE(CIELabf::Max[0] == 100.0F);
    CATCH_REQUIRE(CIELabf::Max[1] == 127.0F);
    CATCH_REQUIRE(CIELabf::Max[2] == 127.0F);
}

TEST_CASE("mse identical colors")
{
    CIELabf color(50.0F, 20.0F, -30.0F);
    float result = CIELabf::mse(color, color);
    CATCH_REQUIRE(result == 0.0F);
}

TEST_CASE("mse different lightness")
{
    CIELabf c0(50.0F, 0.0F, 0.0F);
    CIELabf c1(60.0F, 0.0F, 0.0F);
    float result = CIELabf::mse(c0, c1);
    // dL = 10/100 = 0.1, da=0, db=0
    // (sqrt(0) + 0.1) / 2.414213562 = 0.1 / 2.414213562
    CATCH_REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.1F / 2.414213562F, 0.0001F));
}

TEST_CASE("mse different chroma")
{
    CIELabf c0(50.0F, 0.0F, 0.0F);
    CIELabf c1(50.0F, 10.0F, 0.0F);
    float result = CIELabf::mse(c0, c1);
    // dL=0, da=10/255, db=0
    // (sqrt((10/255)^2) + 0) / 2.414213562 = (10/255) / 2.414213562
    CATCH_REQUIRE_THAT(result, Catch::Matchers::WithinAbs((10.0F / 255.0F) / 2.414213562F, 0.0001F));
}

TEST_CASE("mse maximum distance")
{
    // Maximum possible distance: L diff = 100, a diff = 255, b diff = 255
    CIELabf c0(0.0F, -128.0F, -128.0F);
    CIELabf c1(100.0F, 127.0F, 127.0F);
    float result = CIELabf::mse(c0, c1);
    // dL = 1, da = 255/255 = 1, db = 255/255 = 1
    // (sqrt(1+1) + 1) / 2.414213562 = (sqrt(2)+1)/(sqrt(2)+1) = 1
    CATCH_REQUIRE_THAT(result, Catch::Matchers::WithinAbs(1.0F, 0.0001F));
}

TEST_CASE("mse in range [0,1]")
{
    CIELabf c0(10.0F, 50.0F, -20.0F);
    CIELabf c1(90.0F, -50.0F, 20.0F);
    float result = CIELabf::mse(c0, c1);
    CATCH_REQUIRE(result >= 0.0F);
    CATCH_REQUIRE(result <= 1.0F);
}

TEST_CASE("ciede2000 identical colors")
{
    CIELabf color(50.0F, 20.0F, -30.0F);
    float result = CIELabf::ciede2000(color, color);
    CATCH_REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.0F, 0.0001F));
}

TEST_CASE("ciede2000 small difference")
{
    CIELabf c0(50.0F, 20.0F, -30.0F);
    CIELabf c1(50.0F, 20.0F, -29.0F);
    float result = CIELabf::ciede2000(c0, c1);
    // Small difference should give small delta E
    CATCH_REQUIRE(result > 0.0F);
    CATCH_REQUIRE(result < 5.0F);
}

TEST_CASE("ciede2000 large difference")
{
    CIELabf c0(0.0F, -100.0F, -100.0F);
    CIELabf c1(100.0F, 100.0F, 100.0F);
    float result = CIELabf::ciede2000(c0, c1);
    // Large difference should give large delta E
    CATCH_REQUIRE(result > 50.0F);
    CATCH_REQUIRE(result < 200.0F);
}

TEST_CASE("ciede2000 symmetric")
{
    CIELabf c0(40.0F, 30.0F, -20.0F);
    CIELabf c1(60.0F, -10.0F, 40.0F);
    float d01 = CIELabf::ciede2000(c0, c1);
    float d10 = CIELabf::ciede2000(c1, c0);
    // CIEDE2000 is not perfectly symmetric, but should be close
    CATCH_REQUIRE_THAT(d01, Catch::Matchers::WithinAbs(d10, 0.5F));
}

TEST_CASE("Equality comparison")
{
    CIELabf c0(50.0F, 20.0F, -30.0F);
    CIELabf c1(50.0F, 20.0F, -30.0F);
    CIELabf c2(50.0F, 21.0F, -30.0F);
    // Same colors should be equal
    CATCH_REQUIRE(c0 == c1);
    // Different colors should not be equal
    CATCH_REQUIRE(!(c0 == c2));
}

TEST_CASE("Component-wise comparison")
{
    CIELabf c0(10.0F, 0.0F, 0.0F);
    CIELabf c1(20.0F, 0.0F, 0.0F);
    // Verify component-wise ordering works
    CATCH_REQUIRE(c0.L() < c1.L());
    CATCH_REQUIRE(c1.L() > c0.L());
}
