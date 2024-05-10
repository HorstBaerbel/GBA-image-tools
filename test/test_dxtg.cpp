#include "testmacros.h"

#include "codec/dxt.h"
#include "color/distance.h"
#include "color/rgb888.h"
#include "io/imageio.h"

#include <algorithm>
#include <vector>

TEST_SUITE("DXT")

TEST_CASE("EncodeDecode555")
{
    auto image = IO::File::readImage("../../data/squish_240x160.png");
    auto inPixels = image.imageData.pixels().convertData<Color::XRGB8888>();
    // IO::File::writeImage(image, "/tmp", "in.png");
    auto compressedData = DXT::encodeDXT(inPixels, image.size.width(), image.size.height(), false);
    auto outPixels = DXT::decodeDXT(compressedData, image.size.width(), image.size.height(), false);
    auto imageError = Color::distance(inPixels, outPixels);
    auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
    image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
    // IO::File::writeRawImage(image, "/tmp", "out.data");
    IO::File::writeImage(image, "/tmp", "out555.png");
    CATCH_REQUIRE(imageError < 0.0003F);
}

TEST_CASE("EncodeDecode565")
{
    auto image = IO::File::readImage("../../data/squish_240x160.png");
    auto inPixels = image.imageData.pixels().convertData<Color::XRGB8888>();
    // IO::File::writeImage(image, "/tmp", "in.png");
    auto compressedData = DXT::encodeDXT(inPixels, image.size.width(), image.size.height(), true);
    auto outPixels = DXT::decodeDXT(compressedData, image.size.width(), image.size.height(), true);
    auto imageError = Color::distance(inPixels, outPixels);
    auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
    image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
    // IO::File::writeRawImage(image, "/tmp", "out.data");
    IO::File::writeImage(image, "/tmp", "out565.png");
    CATCH_REQUIRE(imageError < 0.0003F);
}
