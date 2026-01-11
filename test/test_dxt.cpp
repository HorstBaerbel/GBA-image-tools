#include "testmacros.h"

#include "color/psnr.h"
#include "color/rgb888.h"
#include "image/imageio.h"
#include "image_codec/dxt.h"

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
    {"artificial_384x256.png", 20.17, 20.80},
    {"BigBuckBunny_282_384x256.png", 23.56, 24.18},
    {"BigBuckBunny_361_384x256.png", 20.64, 20.92},
    {"BigBuckBunny_40_384x256.png", 27.42, 28.12},
    {"BigBuckBunny_648_384x256.png", 21.21, 21.47},
    {"BigBuckBunny_664_384x256.png", 22.97, 23.80},
    {"bridge_256x384.png", 18.58, 19.18},
    {"flower_foveon_384x256.png", 21.01, 21.89},
    {"gradient_384x256.png", 24.57, 25.06},
    {"nightshot_iso_100_384x256.png", 19.47, 20.35},
    {"squish_384x384.png", 24.03, 25.03},
    {"TearsOfSteel_1200_384x256.png", 18.34, 19.07},
    {"TearsOfSteel_676_384x256.png", 20.92, 21.46}};

/*
XRGB1555
DXT-compressed RGB555 4x4 block, psnr: 16.58
DXT-compressed RGB555 8x8 block, psnr: 15.06
DXT-compressed RGB555 16x16 block, psnr: 13.34
DXT-compressed RGB555 artificial_384x256.png, psnr: 20.18
DXT-compressed RGB555 BigBuckBunny_282_384x256.png, psnr: 23.57
DXT-compressed RGB555 BigBuckBunny_361_384x256.png, psnr: 20.65
DXT-compressed RGB555 BigBuckBunny_40_384x256.png, psnr: 27.43
DXT-compressed RGB555 BigBuckBunny_648_384x256.png, psnr: 21.22
DXT-compressed RGB555 BigBuckBunny_664_384x256.png, psnr: 22.98
DXT-compressed RGB555 bridge_256x384.png, psnr: 18.59
DXT-compressed RGB555 flower_foveon_384x256.png, psnr: 21.02
DXT-compressed RGB555 gradient_384x256.png, psnr: 24.58
DXT-compressed RGB555 nightshot_iso_100_384x256.png, psnr: 19.48
DXT-compressed RGB555 squish_384x384.png, psnr: 24.04
DXT-compressed RGB555 TearsOfSteel_1200_384x256.png, psnr: 18.35
DXT-compressed RGB555 TearsOfSteel_676_384x256.png, psnr: 20.93

RGB565
DXT-compressed RGB565 4x4 block, psnr: 16.72
DXT-compressed RGB565 8x8 block, psnr: 15.08
DXT-compressed RGB565 16x16 block, psnr: 13.29
DXT-compressed RGB565 artificial_384x256.png, psnr: 20.81
DXT-compressed RGB565 BigBuckBunny_282_384x256.png, psnr: 24.19
DXT-compressed RGB565 BigBuckBunny_361_384x256.png, psnr: 20.93
DXT-compressed RGB565 BigBuckBunny_40_384x256.png, psnr: 28.13
DXT-compressed RGB565 BigBuckBunny_648_384x256.png, psnr: 21.48
DXT-compressed RGB565 BigBuckBunny_664_384x256.png, psnr: 23.81
DXT-compressed RGB565 bridge_256x384.png, psnr: 19.19
DXT-compressed RGB565 flower_foveon_384x256.png, psnr: 21.9
DXT-compressed RGB565 gradient_384x256.png, psnr: 25.05
DXT-compressed RGB565 nightshot_iso_100_384x256.png, psnr: 20.36
DXT-compressed RGB565 squish_384x384.png, psnr: 25.04
DXT-compressed RGB565 TearsOfSteel_1200_384x256.png, psnr: 19.08
DXT-compressed RGB565 TearsOfSteel_676_384x256.png, psnr: 21.47
*/

static const std::string DataPathTest = "data/images/test/";

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
    //  swap to BGR and try again
    compressedData = DXT::encodeBlock<BLOCK_DIM>(inPixels, asRGB565, true);
    outPixels = DXT::decodeBlock<BLOCK_DIM>(compressedData, asRGB565, true);
    auto psnrBGR = Color::psnr(inPixels, outPixels);
    CATCH_REQUIRE(psnrRGB == psnrBGR);
}

TEST_CASE("EncodeDecodeBlock555")
{
    auto image = IO::File::readImage(DataPathTest + "BigBuckBunny_361_384x256.png");
    auto pixels = image.data.pixels().convertData<Color::XRGB8888>();
    testEncodeBlock<4>(pixels, image.info.size.width(), false, 16.57F);
    testEncodeBlock<8>(pixels, image.info.size.width(), false, 15.05F);
    testEncodeBlock<16>(pixels, image.info.size.width(), false, 13.33F);
}

TEST_CASE("EncodeDecodeBlock565")
{
    auto image = IO::File::readImage(DataPathTest + "BigBuckBunny_361_384x256.png");
    auto pixels = image.data.pixels().convertData<Color::XRGB8888>();
    testEncodeBlock<4>(pixels, image.info.size.width(), true, 16.71F);
    testEncodeBlock<8>(pixels, image.info.size.width(), true, 15.07F);
    testEncodeBlock<16>(pixels, image.info.size.width(), true, 13.28F);
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
