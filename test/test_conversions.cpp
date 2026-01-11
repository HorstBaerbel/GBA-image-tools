#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/cielabf.h"
#include "color/conversions.h"
#include "color/gamma.h"
#include "color/grayf.h"
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
        B c;
        if constexpr (!std::is_same<A, B>::value && std::is_same<A, Color::CIELabf>::value)
        {
            c = Color::convertTo<B>(Color::srgbToLinear(a[i]));
        }
        else
        {
            c = Color::convertTo<B>(a[i]);
        }
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
        if constexpr (!std::is_same<A, B>::value)
        {
            if constexpr (std::is_same<B, Color::CIELabf>::value)
            {
                auto c = Color::convertTo<B>(Color::srgbToLinear(a[i]));
                CATCH_REQUIRE_THAT(static_cast<float>(c[0]), Catch::Matchers::WithinAbs(b[i][0], 0.005));
                CATCH_REQUIRE_THAT(static_cast<float>(c[1]), Catch::Matchers::WithinAbs(b[i][1], 0.005));
                CATCH_REQUIRE_THAT(static_cast<float>(c[2]), Catch::Matchers::WithinAbs(b[i][2], 0.005));
            }
            else if constexpr (std::is_same<A, Color::CIELabf>::value)
            {
                if constexpr (std::is_same<B, Color::RGBf>::value)
                {
                    auto c = Color::linearToSrgb(Color::convertTo<Color::RGBf>(a[i]));
                    CATCH_REQUIRE_THAT(static_cast<float>(c[0]), Catch::Matchers::WithinAbs(b[i][0], 0.005));
                    CATCH_REQUIRE_THAT(static_cast<float>(c[1]), Catch::Matchers::WithinAbs(b[i][1], 0.005));
                    CATCH_REQUIRE_THAT(static_cast<float>(c[2]), Catch::Matchers::WithinAbs(b[i][2], 0.005));
                }
                else
                {
                    auto c = Color::convertTo<B>(Color::linearToSrgb(Color::convertTo<Color::RGBf>(a[i])));
                    CATCH_REQUIRE_THAT(static_cast<float>(c[0]), Catch::Matchers::WithinAbs(b[i][0], 0.005));
                    CATCH_REQUIRE_THAT(static_cast<float>(c[1]), Catch::Matchers::WithinAbs(b[i][1], 0.005));
                    CATCH_REQUIRE_THAT(static_cast<float>(c[2]), Catch::Matchers::WithinAbs(b[i][2], 0.005));
                }
            }
            else
            {
                auto c = Color::convertTo<B>(a[i]);
                CATCH_REQUIRE_THAT(static_cast<float>(c[0]), Catch::Matchers::WithinAbs(b[i][0], 0.0005));
                CATCH_REQUIRE_THAT(static_cast<float>(c[1]), Catch::Matchers::WithinAbs(b[i][1], 0.0005));
                CATCH_REQUIRE_THAT(static_cast<float>(c[2]), Catch::Matchers::WithinAbs(b[i][2], 0.0005));
            }
        }
        else
        {
            CATCH_REQUIRE_THAT(static_cast<float>(a[i][0]), Catch::Matchers::WithinAbs(b[i][0], 0.0005));
            CATCH_REQUIRE_THAT(static_cast<float>(a[i][1]), Catch::Matchers::WithinAbs(b[i][1], 0.0005));
            CATCH_REQUIRE_THAT(static_cast<float>(a[i][2]), Catch::Matchers::WithinAbs(b[i][2], 0.0005));
        }
    }
}

// Calculate: https://coliru.stacked-crooked.com/a/c7f8e31d0e6f1cb7

TEST_CASE("Grayf")
{
    std::array<Color::Grayf, 3> c = {
        Color::Grayf(0),
        Color::Grayf(1),
        Color::Grayf(0.5)};
    // RGB565
    std::array<Color::RGB565, 3> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(16, 32, 16)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 3> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(16, 16, 16)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 3> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(128, 128, 128)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 3> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(128, 128, 128)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 3> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.5, 0, 0)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 3> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, -0.003, 0.0006), Color::CIELabf(53.388, -0.002, 0.0)};
    compare(c, c6);
    // RGBf
    std::array<Color::RGBf, 3> c7 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(0.5, 0.5, 0.5)};
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
        Color::RGB565(8, 32, 16)};
    // XRGB1555
    std::array<Color::XRGB1555, 6> c1 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(8, 15, 16)};
    compare(c, c1);
    // RGB888
    std::array<Color::RGB888, 6> c2 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(66, 130, 132)};
    compare(c, c2);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c3 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(66, 130, 132)};
    compare(c, c3);
    // RGBf
    std::array<Color::RGBf, 6> c4 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(0.258, 0.508, 0.516)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.448, 0.121, -0.258)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0, 0), Color::CIELabf(53.24, 80.09, 67.2), Color::CIELabf(87.73, -86.185, 83.18), Color::CIELabf(32.3, 79.19, -107.86), Color::CIELabf(50.269, -19.77, -7.44)};
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
        Color::XRGB1555(8, 16, 16)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(8, 33, 16)};
    compare(c, c1);
    // RGB888
    std::array<Color::RGB888, 6> c2 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(66, 132, 132)};
    compare(c, c2);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c3 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(66, 132, 132)};
    compare(c, c3);
    // RGBf
    std::array<Color::RGBf, 6> c4 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(0.258, 0.516, 0.516)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.452, 0.129, -0.258)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0, 0), Color::CIELabf(53.24, 80.09, 67.2), Color::CIELabf(87.73, -86.185, 83.18), Color::CIELabf(32.3, 79.19, -107.86), Color::CIELabf(50.924, -20.878, -6.467)};
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
        Color::RGB888(64, 128, 128)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(8, 32, 16)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(8, 16, 16)};
    compare(c, c2);
    // RGBf
    std::array<Color::RGBf, 6> c3 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(64.0 / 255, 128.0 / 255, 128.0 / 255)};
    compare(c, c3);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c4 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.439, 0.125, -0.251)};
    compare(c, c4);
    // CIELabf
    std::array<Color::CIELabf, 6> c5 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0, 0), Color::CIELabf(53.24, 80.09, 67.2), Color::CIELabf(87.73, -86.185, 83.18), Color::CIELabf(32.3, 79.19, -107.86), Color::CIELabf(49.6, -20.42, -6.33)};
    compare(c, c5);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c6 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(64, 128, 128)};
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
        Color::XRGB8888(64, 128, 128)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(8, 32, 16)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(8, 16, 16)};
    compare(c, c2);
    // RGBf
    std::array<Color::RGBf, 6> c3 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(64.0 / 255, 128.0 / 255, 128.0 / 255)};
    compare(c, c3);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c4 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.439, 0.125, -0.251)};
    compare(c, c4);
    // CIELabf
    std::array<Color::CIELabf, 6> c5 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0, 0), Color::CIELabf(53.24, 80.09, 67.2), Color::CIELabf(87.73, -86.185, 83.18), Color::CIELabf(32.3, 79.19, -107.86), Color::CIELabf(49.6, -20.42, -6.33)};
    compare(c, c5);
    // RGB888
    std::array<Color::RGB888, 6> c6 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(64, 128, 128)};
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
        Color::RGBf(0.25, 0.5, 0.5)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(8, 32, 16)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(8, 16, 16)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 6> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(64, 128, 128)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(64, 128, 128)};
    compare(c, c4);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c5 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.4375, 0.125, -0.25)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0, 0), Color::CIELabf(53.24, 80.09, 67.2), Color::CIELabf(87.73, -86.185, 83.18), Color::CIELabf(32.3, 79.19, -107.86), Color::CIELabf(49.42, -20.36, -6.31)};
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
        Color::YCgCoRf(0.4375, 0.125, -0.25)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(8, 32, 16)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(8, 16, 16)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 6> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(64, 128, 128)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(64, 128, 128)};
    compare(c, c4);
    // RGBf
    std::array<Color::RGBf, 6> c5 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(0.25, 0.5, 0.5)};
    compare(c, c5);
    // CIELabf
    std::array<Color::CIELabf, 6> c6 = {Color::CIELabf(0, 0, 0), Color::CIELabf(100, 0, 0), Color::CIELabf(53.24, 80.09, 67.2), Color::CIELabf(87.73, -86.185, 83.18), Color::CIELabf(32.3, 79.19, -107.86), Color::CIELabf(49.42, -20.36, -6.31)};
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
        Color::CIELabf(100, 0, 0),
        Color::CIELabf(53.24, 80.09, 67.2),
        Color::CIELabf(87.73, -86.185, 83.18),
        Color::CIELabf(32.3, 79.19, -107.86),
        Color::CIELabf(49.6, -20.42, -6.33)};
    // RGB565
    std::array<Color::RGB565, 6> c1 = {Color::RGB565(0, 0, 0), Color::RGB565(31, 63, 31), Color::RGB565(31, 0, 0), Color::RGB565(0, 63, 0), Color::RGB565(0, 0, 31), Color::RGB565(8, 32, 16)};
    compare(c, c1);
    // XRGB1555
    std::array<Color::XRGB1555, 6> c2 = {Color::XRGB1555(0, 0, 0), Color::XRGB1555(31, 31, 31), Color::XRGB1555(31, 0, 0), Color::XRGB1555(0, 31, 0), Color::XRGB1555(0, 0, 31), Color::XRGB1555(8, 16, 16)};
    compare(c, c2);
    // RGB888
    std::array<Color::RGB888, 6> c3 = {Color::RGB888(0, 0, 0), Color::RGB888(255, 255, 255), Color::RGB888(255, 0, 0), Color::RGB888(0, 255, 0), Color::RGB888(0, 0, 255), Color::RGB888(64, 128, 128)};
    compare(c, c3);
    // XRGB8888
    std::array<Color::XRGB8888, 6> c4 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(64, 128, 128)};
    compare(c, c4);
    // RGBf
    std::array<Color::RGBf, 6> c5 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(64.0 / 255, 128.0 / 255, 128.0 / 255)};
    compare(c, c5);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c6 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.4375, 0.125, -0.25)};
    compare(c, c6);
    // grayscale
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::CIELabf(0, 0, 0)), Catch::Matchers::WithinAbs(0.0, 0.0001));
    // CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::CIELabf(50, 0, 0)), Catch::Matchers::WithinAbs(0.5, 0.001));
    CATCH_REQUIRE_THAT(Color::convertTo<Color::Grayf>(Color::CIELabf(100, 0, 0)), Catch::Matchers::WithinAbs(1.0, 0.0001));
}
