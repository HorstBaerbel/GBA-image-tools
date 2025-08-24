#include "testmacros.h"

#include "color/psnr.h"
#include "color/rgb888.h"
#include "image_codec/dxt.h"
#include "io/imageio.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct DxtTestFile
{
    std::string fileName;
    float minPsnr555;
    float minPsnr565;
};

static const std::vector<DxtTestFile> DxtTestFiles = {
    {"artificial_384x256.png", 33.27F, 33.50F},
    {"BigBuckBunny_282_384x256.png", 34.89F, 35.27F},
    {"BigBuckBunny_361_384x256.png", 31.65F, 31.84F},
    {"BigBuckBunny_40_384x256.png", 39.40F, 39.73F},
    {"BigBuckBunny_648_384x256.png", 32.55F, 32.72F},
    {"BigBuckBunny_664_384x256.png", 35.48F, 35.97F},
    {"bridge_256x384.png", 31.78F, 31.98F},
    {"flower_foveon_384x256.png", 36.57F, 37.04F},
    {"gradient_384x256.png", 43.28F, 44.86F},
    {"nightshot_iso_100_384x256.png", 34.70F, 35.08F},
    {"squish_384x384.png", 40.10F, 41.32F},
    {"TearsOfSteel_1200_384x256.png", 33.43F, 33.70F},
    {"TearsOfSteel_676_384x256.png", 34.03F, 34.34F}};

/*
XRGB1555
artificial_384x256.png, psnr: 33.28
BigBuckBunny_282_384x256.png, psnr: 34.9
BigBuckBunny_361_384x256.png, psnr: 31.66
BigBuckBunny_40_384x256.png, psnr: 39.41
BigBuckBunny_648_384x256.png, psnr: 32.56
BigBuckBunny_664_384x256.png, psnr: 35.49
bridge_256x384.png, psnr: 31.79
flower_foveon_384x256.png, psnr: 36.58
gradient_384x256.png, psnr: 43.29
nightshot_iso_100_384x256.png, psnr: 34.71
squish_384x384.png, psnr: 40.1
TearsOfSteel_1200_384x256.png, psnr: 33.44
TearsOfSteel_676_384x256.png, psnr: 34.04

RGB565
artificial_384x256.png, psnr: 33.51
BigBuckBunny_282_384x256.png, psnr: 35.28
BigBuckBunny_361_384x256.png, psnr: 31.85
BigBuckBunny_40_384x256.png, psnr: 39.74
BigBuckBunny_648_384x256.png, psnr: 32.73
BigBuckBunny_664_384x256.png, psnr: 35.98
bridge_256x384.png, psnr: 31.99
flower_foveon_384x256.png, psnr: 37.05
gradient_384x256.png, psnr: 44.87
nightshot_iso_100_384x256.png, psnr: 35.09
squish_384x384.png, psnr: 41.32
TearsOfSteel_1200_384x256.png, psnr: 33.71
TearsOfSteel_676_384x256.png, psnr: 34.35
*/

static const std::string DataPathTest = "../../data/images/test/";

// #define WRITE_OUTPUT

TEST_SUITE("DXT")

/// @brief Encode/decode single block as DXT
template <unsigned BLOCK_DIM>
auto testEncodeBlock(const std::vector<Color::XRGB8888> &image, const std::size_t pixelsPerScanline, const bool asRGB565, const float allowedPsnr) -> void
{
    std::vector<Color::XRGB8888> inPixels;
    for (std::size_t y = 0; y < BLOCK_DIM; ++y)
    {
        auto lineStart = std::next(image.cbegin(), y * pixelsPerScanline);
        std::copy(lineStart, std::next(lineStart, BLOCK_DIM), std::back_inserter(inPixels));
    }
    // compress and uncompress block
    auto compressedData = DXT::encodeBlock<BLOCK_DIM>(inPixels, asRGB565, false);
    auto outPixels = DXT::decodeBlock<BLOCK_DIM>(compressedData, asRGB565, false);
    auto psnrRGB = Color::psnr(inPixels, outPixels);
    std::cout << "DXT-compressed " << (asRGB565 ? "RGB565 " : "RGB555 ") << BLOCK_DIM << "x" << BLOCK_DIM << " block, psnr: " << std::setprecision(4) << psnrRGB << std::endl;
    CATCH_REQUIRE(psnrRGB >= allowedPsnr);
    // swap to BGR and try again
    compressedData = DXT::encodeBlock<BLOCK_DIM>(inPixels, asRGB565, true);
    outPixels = DXT::decodeBlock<BLOCK_DIM>(compressedData, asRGB565, true);
    auto psnrBGR = Color::psnr(inPixels, outPixels);
    CATCH_REQUIRE(psnrRGB == psnrBGR);
}

TEST_CASE("EncodeDecodeBlock555")
{
    auto image = IO::File::readImage(DataPathTest + "BigBuckBunny_361_384x256.png");
    auto pixels = image.data.pixels().convertData<Color::XRGB8888>();
    testEncodeBlock<4>(pixels, image.info.size.width(), false, 21.69F);
    testEncodeBlock<8>(pixels, image.info.size.width(), false, 19.49F);
    testEncodeBlock<16>(pixels, image.info.size.width(), false, 15.31F);
}

TEST_CASE("EncodeDecodeBlock565")
{
    auto image = IO::File::readImage(DataPathTest + "BigBuckBunny_361_384x256.png");
    auto pixels = image.data.pixels().convertData<Color::XRGB8888>();
    testEncodeBlock<4>(pixels, image.info.size.width(), true, 22.23F);
    testEncodeBlock<8>(pixels, image.info.size.width(), true, 19.61F);
    testEncodeBlock<16>(pixels, image.info.size.width(), true, 15.07F);
}

TEST_CASE("EncodeDecode555")
{
    for (auto &testFile : DxtTestFiles)
    {
        auto image = IO::File::readImage(DataPathTest + testFile.fileName);
        auto inPixels = image.data.pixels().convertData<Color::XRGB8888>();
        // IO::File::writeImage(image, "/tmp", "in.png");
        auto compressedDataRGB = DXT::encode(inPixels, image.info.size.width(), image.info.size.height(), false);
        auto outPixelsRGB = DXT::decode(compressedDataRGB, image.info.size.width(), image.info.size.height(), false);
        auto psnrRGB = Color::psnr(inPixels, outPixelsRGB);
#ifdef WRITE_OUTPUT
        auto xrgb8888 = Image::PixelData(outPixelsRGB, Color::Format::XRGB8888);
        image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        IO::File::writeImage(image, "/tmp/test555", testFile.fileName);
#endif
        auto compressedDataBGR = DXT::encode(inPixels, image.info.size.width(), image.info.size.height(), false, true);
        auto outPixelsBGR = DXT::decode(compressedDataBGR, image.info.size.width(), image.info.size.height(), false, true);
        auto psnrBGR = Color::psnr(inPixels, outPixelsBGR);
        std::cout << "DXT-compressed RGB555 " << testFile.fileName << ", psnr: " << std::setprecision(4) << psnrRGB << std::endl;
        CATCH_REQUIRE(psnrRGB == psnrBGR);
        CATCH_REQUIRE(psnrRGB >= testFile.minPsnr555);
    }
}

TEST_CASE("EncodeDecode565")
{
    for (auto &testFile : DxtTestFiles)
    {
        auto image = IO::File::readImage(DataPathTest + testFile.fileName);
        auto inPixels = image.data.pixels().convertData<Color::XRGB8888>();
        // IO::File::writeImage(image, "/tmp", "in.png");
        auto compressedDataRGB = DXT::encode(inPixels, image.info.size.width(), image.info.size.height(), true);
        auto outPixelsRGB = DXT::decode(compressedDataRGB, image.info.size.width(), image.info.size.height(), true);
        auto psnrRGB = Color::psnr(inPixels, outPixelsRGB);
#ifdef WRITE_OUTPUT
        auto xrgb8888 = Image::PixelData(outPixelsRGB, Color::Format::XRGB8888);
        image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        IO::File::writeImage(image, "/tmp/test565", testFile.fileName);
#endif
        auto compressedDataBGR = DXT::encode(inPixels, image.info.size.width(), image.info.size.height(), true, true);
        auto outPixelsBGR = DXT::decode(compressedDataBGR, image.info.size.width(), image.info.size.height(), true, true);
        auto psnrBGR = Color::psnr(inPixels, outPixelsBGR);
        std::cout << "DXT-compressed RGB565 " << testFile.fileName << ", psnr: " << std::setprecision(4) << psnrRGB << std::endl;
        CATCH_REQUIRE(psnrRGB == psnrBGR);
        CATCH_REQUIRE(psnrRGB >= testFile.minPsnr565);
    }
}
