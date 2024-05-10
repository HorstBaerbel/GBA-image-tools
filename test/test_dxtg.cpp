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
    auto compressedData = DXT::encodeDXT(inPixels, image.size.width(), image.size.height());
    auto outPixels = DXT::decodeDXT(compressedData, image.size.width(), image.size.height());
    auto error = Color::distance(inPixels, outPixels);
    auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
    image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
    IO::File::writeRawImage(image, "/tmp", "out.data");
    IO::File::writeImage(image, "/tmp", "out.png");
    // CATCH_REQUIRE(error < 0.1F);
}
