#include "testmacros.h"

#include "color/psnr.h"
#include "color/rgb888.h"
#include "image/imageio.h"
#include "video_codec/blockview.h"
#include "video_codec/codebook.h"
#include "video_codec/dxtv.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

static const std::vector<std::string> SequenceFiles = {
    "BigBuckBunny_240x160_15fps-178.png",
    "BigBuckBunny_240x160_15fps-179.png",
    "BigBuckBunny_240x160_15fps-180.png",
    "BigBuckBunny_240x160_15fps-181.png"};

static const std::string DataPathTest = "../../data/images/test/";
static const std::string DataPathGBAImages = "../../data/images/240x160/";
static const std::string DataPathGBAVideos = "../../data/videos/240x160/";

static constexpr float BlockQualityDXTV8x8 = 70;
static constexpr float BlockQualityDXTV4x4 = 99;

static constexpr float ImageQualityDXT8x8 = 90;
static constexpr float ImageQualityDXT4x4 = 99;

// #define WRITE_OUTPUT

TEST_SUITE("DXTV")

/// @brief Encode/decode single 8x8 block as DXTV
auto testEncodeBlock(const Image::Frame &data, const float quality, const float allowedPsnr, bool swapToBGR) -> void
{
    // input image
    const auto size = data.info.size;
    auto currentCodeBook = Video::Dxtv::CodeBook8x8(data.data.pixels().convertData<Color::XRGB8888>(), size.width(), size.height(), false);
    auto &inBlock = currentCodeBook.block(0);
    const auto inPixels = inBlock.pixels();
    // output image
    std::vector<Color::XRGB8888> outImage(data.data.pixels().size());
    // compress block
    auto [blockSplitFlag, compressedData] = Video::Dxtv::encodeBlock<Video::DxtvConstants::BLOCK_MAX_DIM>(currentCodeBook, CodeBook<Color::XRGB8888, Video::DxtvConstants::BLOCK_MAX_DIM>(), inBlock, quality, swapToBGR);
    // uncompress block
    auto dataPtr = reinterpret_cast<const uint16_t *>(compressedData.data());
    auto currPtr = outImage.data();
    const Color::XRGB8888 *prevPtr = nullptr;
    if (blockSplitFlag)
    {
        dataPtr = Video::Dxtv::decodeBlock<4>(dataPtr, currPtr, prevPtr, size.width(), swapToBGR);                                               // A - upper-left
        dataPtr = Video::Dxtv::decodeBlock<4>(dataPtr, currPtr + 4, prevPtr + 4, size.width(), swapToBGR);                                       // B - upper-right
        dataPtr = Video::Dxtv::decodeBlock<4>(dataPtr, currPtr + 4 * size.width(), prevPtr + 4 * size.width(), size.width(), swapToBGR);         // C - lower-left
        dataPtr = Video::Dxtv::decodeBlock<4>(dataPtr, currPtr + 4 * size.width() + 4, prevPtr + 4 * size.width() + 4, size.width(), swapToBGR); // D - lower-right
    }
    else
    {
        dataPtr = Video::Dxtv::decodeBlock<8>(dataPtr, currPtr, prevPtr, size.width(), swapToBGR);
    }
    // compare input and output
    auto outCodeBook = Video::Dxtv::CodeBook8x8(outImage, size.width(), size.height(), false);
    auto outPixels = currentCodeBook.block(0).pixels();
    auto psnr = Color::psnr(inPixels, outPixels);
    std::cout << "DXTV-compressed " << (swapToBGR ? "BGR555 " : "RGB555 ") << Video::DxtvConstants::BLOCK_MAX_DIM << "x" << Video::DxtvConstants::BLOCK_MAX_DIM << " block, psnr: " << std::setprecision(4) << psnr << std::endl;
    CATCH_REQUIRE(psnr >= allowedPsnr);
}

/// @brief Encode/decode single image as DXTV
auto testEncode(const Image::Frame &data, const float quality, const float allowedPsnr, bool swapToBGR) -> void
{
    // input image
    const auto size = data.info.size;
    const auto inPixels = data.data.pixels().convertData<Color::XRGB8888>();
    // compress image
    const auto [compressedData, frameBuffer] = Video::Dxtv::encode(inPixels, {}, size.width(), size.height(), quality, swapToBGR);
    auto outPixels = Video::Dxtv::decode(compressedData, {}, size.width(), size.height(), swapToBGR);
    auto psnr = Color::psnr(inPixels, outPixels);
    std::cout << "DXTV-compressed " << (swapToBGR ? "BGR555 " : "RGB555 ") << "image, psnr: " << std::setprecision(4) << psnr << std::endl;
    /*auto encoded = data;
    encoded.image.data.pixels() = Image::PixelData(frameBuffer, Color::Format::XRGB8888);
    auto decoded = data;
    decoded.image.data.pixels() = Image::PixelData(outPixels, Color::Format::XRGB8888);
    IO::File::writeImage(encoded, "/tmp", "dxtv_encoded.png");
    IO::File::writeImage(decoded, "/tmp", "dxtv_decoded.png");*/
    CATCH_REQUIRE(psnr >= allowedPsnr);
}

TEST_CASE("EncodeDecodeBlock")
{
    auto image = IO::File::readImage(DataPathTest + "BigBuckBunny_361_384x256.png");
    testEncodeBlock(image, BlockQualityDXTV8x8, 15.05, false);
    testEncodeBlock(image, BlockQualityDXTV8x8, 15.05, true);
    testEncodeBlock(image, BlockQualityDXTV4x4, 17.63, false);
    testEncodeBlock(image, BlockQualityDXTV4x4, 17.63, true);
}

TEST_CASE("EncodeDecodeImage")
{
    auto image = IO::File::readImage(DataPathGBAImages + "BigBuckBunny_361_240x160.png");
    testEncode(image, ImageQualityDXT8x8, 16.68, true);
    testEncode(image, ImageQualityDXT4x4, 20.13, true);
}

TEST_CASE("EncodeDecodeVideo")
{
    std::vector<Image::Frame> images;
    for (const auto &file : SequenceFiles)
    {
        images.push_back(IO::File::readImage(DataPathGBAVideos + file));
    }
    constexpr bool swapToBGR = true;
    constexpr float allowedPsnr = 16.36F;
    std::vector<Color::XRGB8888> prevPixels;
    for (const auto &data : images)
    {
        // input image
        const auto size = data.info.size;
        const auto inPixels = data.data.pixels().convertData<Color::XRGB8888>();
        // compress image
        const auto [compressedData, frameBuffer] = Video::Dxtv::encode(inPixels, prevPixels, size.width(), size.height(), ImageQualityDXT8x8, swapToBGR);
        auto outPixels = Video::Dxtv::decode(compressedData, prevPixels, size.width(), size.height(), swapToBGR);
        auto psnr = Color::psnr(inPixels, outPixels);
        std::cout << "DXTV-compressed " << (swapToBGR ? "BGR555 " : "RGB555 ") << "frame, psnr: " << std::setprecision(4) << psnr << std::endl;
        CATCH_REQUIRE(psnr >= allowedPsnr);
        prevPixels = frameBuffer;
    }
}
