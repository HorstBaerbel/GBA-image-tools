#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testmacros.h"

#include "color/conversions.h"
#include "color/lchf.h"
#include "color/rgbf.h"
#include "color/rgb565.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "color/ycgcorf.h"

#include <array>

TEST_SUITE("Conversions")

template <std::size_t N, typename A, typename B>
auto compare(const std::array<A, N> &a, const std::array<B, N> &b) -> void
{
    for (std::size_t i = 0; i < N; i++)
    {
        CATCH_REQUIRE(Color::convertTo<B>(a[i]) == b[i]);
    }
}

template <std::size_t N, typename A>
auto compare(const std::array<A, N> &a, const std::array<Color::RGBf, N> &b) -> void
{
    for (std::size_t i = 0; i < N; i++)
    {
        auto c = Color::convertTo<Color::RGBf>(a[i]);
        CATCH_REQUIRE_THAT(c[0], Catch::Matchers::WithinAbs(b[i][0], 0.0005));
        CATCH_REQUIRE_THAT(c[1], Catch::Matchers::WithinAbs(b[i][1], 0.0005));
        CATCH_REQUIRE_THAT(c[2], Catch::Matchers::WithinAbs(b[i][2], 0.0005));
    }
}

template <std::size_t N, typename A>
auto compare(const std::array<A, N> &a, const std::array<Color::YCgCoRf, N> &b) -> void
{
    for (std::size_t i = 0; i < N; i++)
    {
        auto c = Color::convertTo<Color::YCgCoRf>(a[i]);
        CATCH_REQUIRE_THAT(c[0], Catch::Matchers::WithinAbs(b[i][0], 0.0005));
        CATCH_REQUIRE_THAT(c[1], Catch::Matchers::WithinAbs(b[i][1], 0.0005));
        CATCH_REQUIRE_THAT(c[2], Catch::Matchers::WithinAbs(b[i][2], 0.0005));
    }
}

template <std::size_t N, typename A>
auto compare(const std::array<A, N> &a, const std::array<Color::LChf, N> &b) -> void
{
    for (std::size_t i = 0; i < N; i++)
    {
        auto c = Color::convertTo<Color::LChf>(a[i]);
        CATCH_REQUIRE_THAT(c[0], Catch::Matchers::WithinAbs(b[i][0], 0.0005));
        CATCH_REQUIRE_THAT(c[1], Catch::Matchers::WithinAbs(b[i][1], 0.0005));
        CATCH_REQUIRE_THAT(c[2], Catch::Matchers::WithinAbs(b[i][2], 0.0005));
    }
}

// Cacluate: https://coliru.stacked-crooked.com/a/a6f3ec31d820dada

CATCH_TEST_CASE("RGB565", TEST_SUITE_TAG)
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
    // XRGB8888
    std::array<Color::XRGB8888, 6> c2 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 57, 247)};
    compare(c, c2);
    // RGBf
    std::array<Color::RGBf, 6> c3 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(10.0 / 31, 14.0 / 63, 30.0 / 31)};
    compare(c, c3);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c4 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.433692, -0.422939, -0.645161)};
    compare(c, c4);
    // LChf
    std::array<Color::LChf, 6> c5 = {Color::LChf(0, 0, 0), Color::LChf(100, 0.00840794, 213.9604), Color::LChf(53.23824, 104.5461, 39.99994), Color::LChf(87.73554, 119.7787, 136.0166), Color::LChf(32.29847, 133.8101, 306.2844), Color::LChf(61.42716, 68.55281, 302.5583)};
    compare(c, c5);
}

CATCH_TEST_CASE("XRGB1555", TEST_SUITE_TAG)
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
    // XRGB8888
    std::array<Color::XRGB8888, 6> c2 = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255), Color::XRGB8888(255, 0, 0), Color::XRGB8888(0, 255, 0), Color::XRGB8888(0, 0, 255), Color::XRGB8888(82, 58, 247)};
    compare(c, c2);
    // RGBf
    std::array<Color::RGBf, 6> c3 = {Color::RGBf(0, 0, 0), Color::RGBf(1, 1, 1), Color::RGBf(1, 0, 0), Color::RGBf(0, 1, 0), Color::RGBf(0, 0, 1), Color::RGBf(10.0 / 31, 7.0 / 31, 30.0 / 31)};
    compare(c, c3);
    // YCgCoRf
    std::array<Color::YCgCoRf, 6> c4 = {Color::YCgCoRf(0, 0, 0), Color::YCgCoRf(1, 0, 0), Color::YCgCoRf(0.25, -0.5, 1), Color::YCgCoRf(0.5, 1, 0), Color::YCgCoRf(0.25, -0.5, -1), Color::YCgCoRf(0.43548, -0.41935, -0.645161)};
    compare(c, c4);
    // LChf
    std::array<Color::LChf, 6> c5 = {Color::LChf(0, 0, 0), Color::LChf(100, 0.00840794, 213.9604), Color::LChf(53.23824, 104.5461, 39.99994), Color::LChf(87.73554, 119.7787, 136.0166), Color::LChf(32.29847, 133.8101, 306.2844), Color::LChf(61.64899, 67.9604, 302.32974)};
    compare(c, c5);
}
