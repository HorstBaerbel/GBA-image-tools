#include "testmacros.h"

#include "color/colorformat.h"

using namespace Color;

TEST_SUITE("Color format")

TEST_CASE("addColorAtIndex0")
{
    // 1 bpp
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 1) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 2) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 3) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 4) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 5) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 6) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 7) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 8) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted1, 9) == 2);
    // 2 bpp
    CATCH_REQUIRE(bytesPerImage(Format::Paletted2, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted2, 1) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted2, 2) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted2, 3) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted2, 4) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted2, 5) == 2);
    // 4 bpp
    CATCH_REQUIRE(bytesPerImage(Format::Paletted4, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted4, 1) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted4, 2) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted4, 3) == 2);
    // 8 bpp
    CATCH_REQUIRE(bytesPerImage(Format::Paletted8, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted8, 1) == 1);
    CATCH_REQUIRE(bytesPerImage(Format::Paletted8, 2) == 2);
    // 15 bpp
    const auto bytes15 = formatInfo(Format::XRGB1555).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::XRGB1555, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::XRGB1555, 1) == bytes15);
    CATCH_REQUIRE(bytesPerImage(Format::XRGB1555, 2) == 2 * bytes15);
    CATCH_REQUIRE(bytesPerImage(Format::XBGR1555, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::XBGR1555, 1) == bytes15);
    CATCH_REQUIRE(bytesPerImage(Format::XBGR1555, 2) == 2 * bytes15);
    // 16 bpp
    const auto bytes16 = formatInfo(Format::RGB565).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::RGB565, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::RGB565, 1) == bytes16);
    CATCH_REQUIRE(bytesPerImage(Format::RGB565, 2) == 2 * bytes16);
    CATCH_REQUIRE(bytesPerImage(Format::BGR565, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::BGR565, 1) == bytes16);
    CATCH_REQUIRE(bytesPerImage(Format::BGR565, 2) == 2 * bytes16);
    // 24 bpp
    const auto bytes24 = formatInfo(Format::RGB888).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::RGB888, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::RGB888, 1) == bytes24);
    CATCH_REQUIRE(bytesPerImage(Format::RGB888, 2) == 2 * bytes24);
    CATCH_REQUIRE(bytesPerImage(Format::BGR888, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::BGR888, 1) == bytes24);
    CATCH_REQUIRE(bytesPerImage(Format::BGR888, 2) == 2 * bytes24);
    // 32 bpp
    const auto bytes32 = formatInfo(Format::XRGB8888).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::XRGB8888, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::XRGB8888, 1) == bytes32);
    CATCH_REQUIRE(bytesPerImage(Format::XRGB8888, 2) == 2 * bytes32);
    CATCH_REQUIRE(bytesPerImage(Format::XBGR8888, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::XBGR8888, 1) == bytes32);
    CATCH_REQUIRE(bytesPerImage(Format::XBGR8888, 2) == 2 * bytes32);
    // RGBf
    const auto bytesRGBf = formatInfo(Format::RGBf).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::RGBf, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::RGBf, 1) == bytesRGBf);
    CATCH_REQUIRE(bytesPerImage(Format::RGBf, 2) == 2 * bytesRGBf);
    // CIELabf
    const auto bytesCIELabf = formatInfo(Format::CIELabf).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::CIELabf, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::CIELabf, 1) == bytesCIELabf);
    CATCH_REQUIRE(bytesPerImage(Format::CIELabf, 2) == 2 * bytesCIELabf);
    // YCgCoRf
    const auto bytesYCgCoRf = formatInfo(Format::YCgCoRf).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::YCgCoRf, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::YCgCoRf, 1) == bytesYCgCoRf);
    CATCH_REQUIRE(bytesPerImage(Format::YCgCoRf, 2) == 2 * bytesYCgCoRf);
    // Grayf
    const auto bytesGrayf = formatInfo(Format::Grayf).bytesPerPixel;
    CATCH_REQUIRE(bytesPerImage(Format::Grayf, 0) == 0);
    CATCH_REQUIRE(bytesPerImage(Format::Grayf, 1) == bytesGrayf);
    CATCH_REQUIRE(bytesPerImage(Format::Grayf, 2) == 2 * bytesGrayf);
}
