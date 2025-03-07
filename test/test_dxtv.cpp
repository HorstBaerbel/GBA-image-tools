#include "testmacros.h"

#include "codec/blockview.h"
#include "codec/codebook.h"
#include "codec/dxtv.h"
#include "color/psnr.h"
#include "color/rgb888.h"
#include "io/imageio.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct TestFile
{
    std::string fileName;
    float minPsnr555;
    float minPsnr565;
};

static const std::vector<TestFile> TestFiles = {
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
static const std::string DataPathGBA = "../../data/images/240x160/";

static constexpr float BlockQualityDXTV8x8 = 70;
static constexpr float BlockQualityDXTV4x4 = 99;

static constexpr float ImageQualityDXT8x8 = 90;
static constexpr float ImageQualityDXT4x4 = 99;

// #define WRITE_OUTPUT

TEST_SUITE("DXTV")

/// @brief Encode/decode single 8x8 block as DXTV
auto testEncodeBlock(const Image::Data &data, const float quality, const float allowedPsnr, bool swapToBGR) -> void
{
    // input image
    const auto size = data.image.size;
    auto currentCodeBook = DXTV::CodeBook8x8(data.image.data.pixels().convertData<Color::XRGB8888>(), size.width(), size.height(), false);
    auto &inBlock = currentCodeBook.block(0);
    const auto inPixels = inBlock.pixels();
    // output image
    std::vector<Color::XRGB8888> outImage(data.image.data.pixels().size());
    // compress block
    auto [blockSplitFlag, compressedData] = DXTV::encodeBlock<DXTV::MAX_BLOCK_DIM>(currentCodeBook, CodeBook<Color::XRGB8888, DXTV::MAX_BLOCK_DIM>(), inBlock, quality, swapToBGR);
    // uncompress block
    auto dataPtr = reinterpret_cast<const uint16_t *>(compressedData.data());
    auto currPtr = outImage.data();
    const Color::XRGB8888 *prevPtr = nullptr;
    if (blockSplitFlag)
    {
        dataPtr = DXTV::decodeBlock<4>(dataPtr, currPtr, prevPtr, size.width(), swapToBGR);                                               // A - upper-left
        dataPtr = DXTV::decodeBlock<4>(dataPtr, currPtr + 4, prevPtr + 4, size.width(), swapToBGR);                                       // B - upper-right
        dataPtr = DXTV::decodeBlock<4>(dataPtr, currPtr + 4 * size.width(), prevPtr + 4 * size.width(), size.width(), swapToBGR);         // C - lower-left
        dataPtr = DXTV::decodeBlock<4>(dataPtr, currPtr + 4 * size.width() + 4, prevPtr + 4 * size.width() + 4, size.width(), swapToBGR); // D - lower-right
    }
    else
    {
        dataPtr = DXTV::decodeBlock<8>(dataPtr, currPtr, prevPtr, size.width(), swapToBGR);
    }
    // compare input and output
    auto outCodeBook = DXTV::CodeBook8x8(outImage, size.width(), size.height(), false);
    auto outPixels = currentCodeBook.block(0).pixels();
    auto psnr = Color::psnr(inPixels, outPixels);
    std::cout << "DXTV-compressed " << (swapToBGR ? "BGR555 " : "RGB555 ") << DXTV::MAX_BLOCK_DIM << "x" << DXTV::MAX_BLOCK_DIM << " block, psnr: " << std::setprecision(4) << psnr << std::endl;
    CATCH_REQUIRE(psnr >= allowedPsnr);
}

/// @brief Encode/decode single image as DXTV
auto testEncode(const Image::Data &data, const float quality, const float allowedPsnr, bool swapToBGR) -> void
{
    // input image
    const auto size = data.image.size;
    const auto inPixels = data.image.data.pixels().convertData<Color::XRGB8888>();
    // compress image
    const auto [compressedData, frameBuffer] = DXTV::encode(inPixels, {}, size.width(), size.height(), true, quality, false);
    auto outPixels = DXTV::decode(compressedData, {}, size.width(), size.height(), false);
    auto psnr = Color::psnr(inPixels, outPixels);
    std::cout << "DXTV-compressed " << (swapToBGR ? "BGR555 " : "RGB555 ") << "image, psnr: " << std::setprecision(4) << psnr << std::endl;
    auto encoded = data;
    encoded.image.data.pixels() = Image::PixelData(frameBuffer, Color::Format::XRGB8888);
    auto decoded = data;
    decoded.image.data.pixels() = Image::PixelData(outPixels, Color::Format::XRGB8888);
    IO::File::writeImage(encoded, "/tmp", "dxtv_encoded.png");
    IO::File::writeImage(decoded, "/tmp", "dxtv_decoded.png");
    CATCH_REQUIRE(psnr >= allowedPsnr);
}

TEST_CASE("EncodeDecodeBlock")
{
    auto image = IO::File::readImage(DataPathTest + "BigBuckBunny_361_384x256.png");
    testEncodeBlock(image, BlockQualityDXTV8x8, 14.06F, false);
    testEncodeBlock(image, BlockQualityDXTV8x8, 14.06F, true);
    testEncodeBlock(image, BlockQualityDXTV4x4, 22.96F, false);
    testEncodeBlock(image, BlockQualityDXTV4x4, 22.96F, true);
}

TEST_CASE("EncodeDecodeImage")
{
    auto image = IO::File::readImage(DataPathGBA + "BigBuckBunny_361_240x160.png");
    testEncode(image, ImageQualityDXT8x8, 23.02F, false);
    testEncode(image, ImageQualityDXT4x4, 30.46F, false);
}
