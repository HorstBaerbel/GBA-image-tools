#include "testmacros.h"

#include "image/imagedata.h"

#include <algorithm>
#include <vector>

TEST_SUITE("Image data")

TEST_CASE("DefaultConstruction")
{
    Image::ImageData i0;
    CATCH_REQUIRE(i0.colorMap().empty());
    CATCH_REQUIRE(i0.colorMap().format() == Color::Format::Unknown);
    CATCH_REQUIRE(i0.colorMap().isGrayscale() == false);
    CATCH_REQUIRE(i0.colorMap().isIndexed() == false);
    CATCH_REQUIRE(i0.colorMap().isRaw() == false);
    CATCH_REQUIRE(i0.colorMap().isTruecolor() == false);
    CATCH_REQUIRE(i0.colorMap().size() == 0);
    CATCH_REQUIRE_THROWS(i0.colorMap().data<Color::XRGB8888>());
    CATCH_REQUIRE_THROWS(i0.colorMap().convertData<Color::XRGB8888>());
    CATCH_REQUIRE_THROWS(i0.colorMap().convertDataToRaw());
    CATCH_REQUIRE(i0.pixels().empty());
    CATCH_REQUIRE(i0.pixels().format() == Color::Format::Unknown);
    CATCH_REQUIRE(i0.pixels().isIndexed() == false);
    CATCH_REQUIRE(i0.pixels().isRaw() == false);
    CATCH_REQUIRE(i0.pixels().isTruecolor() == false);
    CATCH_REQUIRE(i0.pixels().size() == 0);
    CATCH_REQUIRE_THROWS(i0.pixels().data<Color::XRGB8888>());
    CATCH_REQUIRE_THROWS(i0.pixels().convertData<Color::XRGB8888>());
    CATCH_REQUIRE_THROWS(i0.pixels().convertDataToRaw());
}

TEST_CASE("ConstructionIndexed")
{
    std::vector<uint8_t> x0{0, 1, 2, 1};
    std::vector<Color::XRGB8888> m0{Color::XRGB8888(1, 1, 1), Color::XRGB8888(2, 2, 2), Color::XRGB8888(3, 3, 3)};
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::Unknown, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::Grayf, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::CIELabf, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::RGB565, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::RGB888, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::RGBf, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::XRGB1555, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::XRGB8888, m0));
    CATCH_REQUIRE_THROWS(Image::ImageData(x0, Color::Format::YCgCoRf, m0));
    Image::ImageData i1(x0, Color::Format::Paletted1, m0);
    Image::ImageData i2(x0, Color::Format::Paletted2, m0);
    Image::ImageData i4(x0, Color::Format::Paletted4, m0);
    Image::ImageData i8(x0, Color::Format::Paletted8, m0);
    CATCH_REQUIRE(i8.colorMap().empty() == false);
    CATCH_REQUIRE(i8.colorMap().format() == Color::Format::XRGB8888);
    CATCH_REQUIRE(i8.colorMap().isGrayscale() == false);
    CATCH_REQUIRE(i8.colorMap().isIndexed() == false);
    CATCH_REQUIRE(i8.colorMap().isRaw() == false);
    CATCH_REQUIRE(i8.colorMap().isTruecolor() == true);
    CATCH_REQUIRE(i8.colorMap().size() == 3);
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::Grayf>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::CIELabf>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::RGB565>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::RGB888>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::RGBf>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::XRGB1555>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<uint8_t>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::YCgCoRf>());
    CATCH_REQUIRE(i8.colorMap().data<Color::XRGB8888>() == m0);
    CATCH_REQUIRE(i8.colorMap().convertData<Color::XRGB8888>() == m0);
    CATCH_REQUIRE(i8.colorMap().convertDataToRaw() == std::vector<uint8_t>({1, 1, 1, 0, 2, 2, 2, 0, 3, 3, 3, 0}));
    CATCH_REQUIRE(i8.pixels().empty() == false);
    CATCH_REQUIRE(i8.pixels().format() == Color::Format::Paletted8);
    CATCH_REQUIRE(i8.pixels().isGrayscale() == false);
    CATCH_REQUIRE(i8.pixels().isIndexed() == true);
    CATCH_REQUIRE(i8.pixels().isRaw() == false);
    CATCH_REQUIRE(i8.pixels().isTruecolor() == false);
    CATCH_REQUIRE(i8.pixels().size() == 4);
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::Grayf>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::CIELabf>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::RGB565>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::RGB888>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::RGBf>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::XRGB1555>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::XRGB8888>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::YCgCoRf>());
    CATCH_REQUIRE(i8.pixels().data<uint8_t>() == x0);
    CATCH_REQUIRE(i8.pixels().convertData<uint8_t>() == x0);
    CATCH_REQUIRE(i8.pixels().convertDataToRaw() == x0);
}

TEST_CASE("ConstructionTruecolor")
{
    std::vector<Color::RGB565> c0{Color::RGB565(1, 1, 1), Color::RGB565(2, 2, 2), Color::RGB565(3, 3, 3)};
    std::vector<Color::XRGB1555> c1{Color::XRGB1555(1, 1, 1), Color::XRGB1555(2, 2, 2), Color::XRGB1555(3, 3, 3)};
    std::vector<Color::RGB888> c7{Color::RGB888(1, 1, 1), Color::RGB888(2, 2, 2), Color::RGB888(3, 3, 3)};
    std::vector<Color::XRGB8888> c8{Color::XRGB8888(1, 1, 1), Color::XRGB8888(2, 2, 2), Color::XRGB8888(3, 3, 3)};
    std::vector<Color::RGBf> c3{Color::RGBf(1, 1, 1), Color::RGBf(2, 2, 2), Color::RGBf(3, 3, 3)};
    std::vector<Color::CIELabf> c4{Color::CIELabf(1, 1, 1), Color::CIELabf(2, 2, 2), Color::CIELabf(3, 3, 3)};
    std::vector<Color::YCgCoRf> c5{Color::YCgCoRf(1, 1, 1), Color::YCgCoRf(2, 2, 2), Color::YCgCoRf(3, 3, 3)};
    Image::ImageData i8(c8);
    CATCH_REQUIRE(i8.colorMap().empty() == true);
    CATCH_REQUIRE(i8.colorMap().format() == Color::Format::Unknown);
    CATCH_REQUIRE(i8.colorMap().isGrayscale() == false);
    CATCH_REQUIRE(i8.colorMap().isIndexed() == false);
    CATCH_REQUIRE(i8.colorMap().isRaw() == false);
    CATCH_REQUIRE(i8.colorMap().isTruecolor() == false);
    CATCH_REQUIRE(i8.colorMap().size() == 0);
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::Grayf>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::CIELabf>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::RGB565>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::RGB888>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::RGBf>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::XRGB8888>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::XRGB1555>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<Color::YCgCoRf>());
    CATCH_REQUIRE_THROWS(i8.colorMap().data<uint8_t>());
    CATCH_REQUIRE_THROWS(i8.colorMap().convertData<Color::XRGB8888>());
    CATCH_REQUIRE_THROWS(i8.colorMap().convertDataToRaw());
    CATCH_REQUIRE(i8.pixels().empty() == false);
    CATCH_REQUIRE(i8.pixels().format() == Color::Format::XRGB8888);
    CATCH_REQUIRE(i8.pixels().isGrayscale() == false);
    CATCH_REQUIRE(i8.pixels().isIndexed() == false);
    CATCH_REQUIRE(i8.pixels().isRaw() == false);
    CATCH_REQUIRE(i8.pixels().isTruecolor() == true);
    CATCH_REQUIRE(i8.pixels().size() == 3);
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::Grayf>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::CIELabf>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::RGB565>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::RGB888>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::RGBf>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::XRGB1555>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<Color::YCgCoRf>());
    CATCH_REQUIRE_THROWS(i8.pixels().data<uint8_t>());
    CATCH_REQUIRE(i8.pixels().data<Color::XRGB8888>() == c8);
    CATCH_REQUIRE(i8.pixels().convertData<Color::XRGB8888>() == c8);
    CATCH_REQUIRE(i8.pixels().convertDataToRaw() == std::vector<uint8_t>({1, 1, 1, 0, 2, 2, 2, 0, 3, 3, 3, 0}));
}
