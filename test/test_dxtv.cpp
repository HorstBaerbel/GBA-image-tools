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

const std::vector<TestFile> TestFiles = {
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

const std::string DataPath = "../../data/images/test/";

const float MaxBlockErrorDXT8x8 = 0.1F;
const float MaxBlockErrorDXT4x4 = 0.001F;

// #define WRITE_OUTPUT

TEST_SUITE("DXTV")

/// @brief Encode/decode single 8x8 block as DXTV
auto testEncodeBlock(const std::vector<Color::XRGB8888> &image, const Image::DataSize &size, const float maxBlockError, const float allowedPsnr, bool swapToBGR) -> void
{
    // input image
    auto currentCodeBook = DXTV::CodeBook8x8(image, size.width(), size.height(), false);
    auto inIt = currentCodeBook.begin();
    auto inPixels = inIt->pixels();
    // output image
    std::vector<Color::XRGB8888> outImage(image.size());
    // compress block
    auto [blockSplitFlag, compressedData] = DXTV::encodeBlock<DXTV::MAX_BLOCK_DIM>(currentCodeBook, CodeBook<Color::XRGB8888, DXTV::MAX_BLOCK_DIM>(), *inIt, maxBlockError, swapToBGR);
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
    auto outIt = currentCodeBook.begin();
    auto outPixels = outIt->pixels();
    auto psnr = Color::psnr(inPixels, outPixels);
    std::cout << "DXTV-compressed " << (swapToBGR ? "BGR555 " : "RGB555 ") << DXTV::MAX_BLOCK_DIM << "x" << DXTV::MAX_BLOCK_DIM << " block, psnr: " << std::setprecision(4) << psnr << std::endl;
    CATCH_REQUIRE(psnr >= allowedPsnr);
}

TEST_CASE("EncodeDecodeBlock555")
{
    auto image = IO::File::readImage(DataPath + "BigBuckBunny_361_384x256.png");
    auto pixels = image.image.data.pixels().convertData<Color::XRGB8888>();
    testEncodeBlock(pixels, image.image.size, MaxBlockErrorDXT8x8, 14.06F, false);
    testEncodeBlock(pixels, image.image.size, MaxBlockErrorDXT8x8, 14.06F, true);
    testEncodeBlock(pixels, image.image.size, MaxBlockErrorDXT4x4, 22.96F, false);
    testEncodeBlock(pixels, image.image.size, MaxBlockErrorDXT4x4, 22.96F, true);
}
