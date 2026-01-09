#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/conversions.h"
#include "color/grayf.h"
#include "color/cielabf.h"
#include "color/rgb565.h"
#include "color/rgb888.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "color/ycgcorf.h"

#include <array>

TEST_SUITE("Conversions")

template <std::size_t N, typename A, typename B>
auto compare(const std::array<A, 1> &a, const std::array<B, N> &b) -> void
{
    for (std::size_t i = 0; i < N; i++)
    {
        auto c = Color::convertTo<B>(a[i]);
        CATCH_REQUIRE_THAT(static_cast<float>(c[0]), Catch::Matchers::WithinAbs(b[i][0], 0.0005));
        CATCH_REQUIRE_THAT(static_cast<float>(c[1]), Catch::Matchers::WithinAbs(b[i][1], 0.0005));
        CATCH_REQUIRE_THAT(static_cast<float>(c[2]), Catch::Matchers::WithinAbs(b[i][2], 0.0005));
    }
}

template <std::size_t N, typename A, typename B>
auto compare(const std::array<A, N> &a, const std::array<B, N> &b) -> void
{
    for (std::size_t i = 0; i < N; i++)
    {
        auto c = Color::convertTo<B>(a[i]);
        CATCH_REQUIRE_THAT(static_cast<float>(c[0]), Catch::Matchers::WithinAbs(b[i][0], 0.0005));
        CATCH_REQUIRE_THAT(static_cast<float>(c[1]), Catch::Matchers::WithinAbs(b[i][1], 0.0005));
        CATCH_REQUIRE_THAT(static_cast<float>(c[2]), Catch::Matchers::WithinAbs(b[i][2], 0.0005));
    }
}

// Calculate: https://coliru.stacked-crooked.com/a/a6f3ec31d820dada

TEST_CASE("Grayf")
{
    std::array<Color::Grayf, 3> c = {
        Color::Grayf(0),
        Color::Grayf(1),
        Color::Grayf(82.0 / 255)};
    // RGB565
    std::array<Color::RGB565, 3> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(10, 20, 10)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 3> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(10, 10, 10)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 3> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(82, 82, 82)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 3> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(82, 82, 82)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 3> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.321568638, 0, 0)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 3> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0.00840794, 213.9604), Color::CIELabf(63.4723129, 0.00575300632, 213.75572)};
    compare(c, c6);
    // RGBf
    std::array<Color::RGBf, 3> c7 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(82.0 / 255, 82.0 / 255, 82.0 / 255)};
    compare(c, c7);
}

TEST_CASE("RGB565")
{
    std::array<Color::RGB565, 6> c = {
        Color::RGB565(0, 0, 0),
        Color::RGB565(31, 63, 31),
        Color::RGB565(31, 0, 0),
        Color::RGB565(0, 63, 0),
        Color::RGB565(0, 0, 31),
        Color::RGB565(10, 14, 30)};
    // XRGB1555
    std::array<Color::XRGB1555, 6> c1 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(10, 7, 30)};
    compare(c, c1);
    // RGB888
    std::array<Color::RGB888, 6> c2 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(82, 57, 247)};
    compare(c, c2);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c3 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 57, 247)};
    compare(c, c3);
    // RGBf
    std::array<Color::RGBf, 6> c4 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(10.0 / 31, 14.0 / 63, 30.0 / 31)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.433692, -0.422939, -0.645161)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0.00840794, 213.9604), Color::CIELabf(53.23824, 104.5461, 39.99994), Color::CIELabf(87.73554, 119.7787, 136.0166), Color::CIELabf(32.29847, 133.8101, 306.2844), Color::CIELabf(61.42716, 68.55281, 302.5583)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGB565(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGB565(15, 32, 15)), Catch::Matchers::WithinAbs(0.5, 0.01));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGB565(31, 63, 31)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}

TEST_CASE("XRGB1555")
{
    std::array<Color::XRGB1555, 6> c = {
        Color::XRGB1555(0, 0, 0),
        Color::XRGB1555(31, 31, 31),
        Color::XRGB1555(31, 0, 0),
        Color::XRGB1555(0, 31, 0),
        Color::XRGB1555(0, 0, 31),
        Color::XRGB1555(10, 7, 30)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(10, 14, 30)};
    compare(c, c1);
    // RGB888
    std::array<Color::RGB888, 6> c2 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(82, 58, 247)};
    compare(c, c2);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c3 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 58, 247)};
    compare(c, c3);
    // RGBf
    std::array<Color::RGBf, 6> c4 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(10.0 / 31, 7.0 / 31, 30.0 / 31)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.43548, -0.41935, -0.645161)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0.00840794, 213.9604), Color::CIELabf(53.23824, 104.5461, 39.99994), Color::CIELabf(87.73554, 119.7787, 136.0166), Color::CIELabf(32.29847, 133.8101, 306.2844), Color::CIELabf(61.64899, 67.9604, 302.32974)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::XRGB1555(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::XRGB1555(15, 15, 15)), Catch::Matchers::WithinAbs(0.48, 0.01));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::XRGB1555(31, 31, 31)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}

TEST_CASE("RGB888")
{
    std::array<Color::RGB888, 6> c = {
        Color::RGB888(0, 0, 0),
        Color::RGB888(255, 255, 255),
        Color::RGB888(255, 0, 0),
        Color::RGB888(0, 255, 0),
        Color::RGB888(0, 0, 255),
        Color::RGB888(82, 57, 247)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(10, 14, 30)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(10, 7, 30)};
    compare(c, c2);
    // RGBf
    std::array<Color::RGBf, 6> c3 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(82.0 / 255, 57.0 / 255, 247.0 / 255)};
    compare(c, c3);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c4 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.43431, -0.42157, -0.64706)};
    compare(c, c4);
    // CIELabf
    std::array<Color::CIELabf, 6> c5 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0.00840794, 213.9604), Color::CIELabf(53.23824, 104.5461, 39.99994), Color::CIELabf(87.73554, 119.7787, 136.0166), Color::CIELabf(32.29847, 133.8101, 306.2844), Color::CIELabf(61.49511, 68.38783, 302.42209)};
    compare(c, c5);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c6 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 57, 247)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGB888(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGB888(127, 127, 127)), Catch::Matchers::WithinAbs(0.5, 0.002));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGB888(255, 255, 255)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}

TEST_CASE("XRGB8888")
{
    std::array<Color::XRGB8888, 6> c = {
        Color::XRGB8888(0, 0, 0),
        Color::XRGB8888(255, 255, 255),
        Color::XRGB8888(255, 0, 0),
        Color::XRGB8888(0, 255, 0),
        Color::XRGB8888(0, 0, 255),
        Color::XRGB8888(82, 57, 247)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(10, 14, 30)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(10, 7, 30)};
    compare(c, c2);
    // RGBf
    std::array<Color::RGBf, 6> c3 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(82.0 / 255, 57.0 / 255, 247.0 / 255)};
    compare(c, c3);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c4 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.43431, -0.42157, -0.64706)};
    compare(c, c4);
    // CIELabf
    std::array<Color::CIELabf, 6> c5 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0.00840794, 213.9604), Color::CIELabf(53.23824, 104.5461, 39.99994), Color::CIELabf(87.73554, 119.7787, 136.0166), Color::CIELabf(32.29847, 133.8101, 306.2844), Color::CIELabf(61.49511, 68.38783, 302.42209)};
    compare(c, c5);
    // RGB888
    std::array<Color::RGB888, 6> c6 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(82, 57, 247)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::XRGB8888(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::XRGB8888(127, 127, 127)), Catch::Matchers::WithinAbs(0.5, 0.002));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::XRGB8888(255, 255, 255)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}

TEST_CASE("RGBf")
{
    std::array<Color::RGBf, 6> c = {
        Color::RGBf(0, 0, 0),
        Color::RGBf(1, 1, 1),
        Color::RGBf(1, 0, 0),
        Color::RGBf(0, 1, 0),
        Color::RGBf(0, 0, 1),
        Color::RGBf(82.0 / 255, 57.0 / 255, 247.0 / 255)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(10, 14, 30)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(10, 7, 30)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 6> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(82, 57, 247)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 57, 247)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.43431, -0.42157, -0.64706)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0.00840794, 213.9604), Color::CIELabf(53.23824, 104.5461, 39.99994), Color::CIELabf(87.73554, 119.7787, 136.0166), Color::CIELabf(32.29847, 133.8101, 306.2844), Color::CIELabf(61.49511, 68.38783, 302.42209)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGBf(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGBf(0.5F, 0.5F, 0.5F)), Catch::Matchers::WithinAbs(0.5, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::RGBf(1, 1, 1)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}

TEST_CASE("YCgCoRf")
{
    std::array<Color::YCgCoRf, 6> c = {
        Color::YCgCoRf(0, 0, 0),
        Color::YCgCoRf(1, 0, 0),
        Color::YCgCoRf(0.25, -0.5, 1),
        Color::YCgCoRf(0.5, 1, 0),
        Color::YCgCoRf(0.25, -0.5, -1),
        Color::YCgCoRf(0.43431, -0.42157, -0.64706)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(10, 14, 30)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(10, 7, 30)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 6> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(82, 57, 247)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 57, 247)};
    compare(c, c4);
    // RGBf
    std::array<Color::RGBf, 6> c5 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(82.0 / 255, 57.0 / 255, 247.0 / 255)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0.00840794, 213.9604), Color::CIELabf(53.23824, 104.5461, 39.99994), Color::CIELabf(87.73554, 119.7787, 136.0166), Color::CIELabf(32.29847, 133.8101, 306.2844), Color::CIELabf(61.49511, 68.38841, 302.42209)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::YCgCoRf(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::YCgCoRf(0.5F, 0, 0)), Catch::Matchers::WithinAbs(0.5, 0.0001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::YCgCoRf(1, 0, 0)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}

TEST_CASE("CIELabf")
{
    std::array<Color::CIELabf, 6> c = {
        Color::CIELabf(0, 0, 0),
        Color::CIELabf(100, 0, 360),
        Color::CIELabf(53.23824, 104.5461, 39.99994),
        Color::CIELabf(87.73554, 119.7787, 136.0166),
        Color::CIELabf(32.29847, 133.8101, 306.2844),
        Color::CIELabf(61.49511, 68.38783, 302.42209)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(10, 14, 30)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(10, 7, 30)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 6> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(82, 57, 247)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 57, 247)};
    compare(c, c4);
    // RGBf
    std::array<Color::RGBf, 6> c5 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(82.0 / 255, 57.0 / 255, 247.0 / 255)};
    compare(c, c5);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c6 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.43431, -0.42157, -0.64706)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::CIELabf(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    // CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::CIELabf(50, 0, 0)), Catch::Matchers::WithinAbs(0.5, 0.001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::CIELabf(100, 0, 360)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}
