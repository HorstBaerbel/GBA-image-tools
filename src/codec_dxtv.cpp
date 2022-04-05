#include "codec_dxtv.h"

#include "color_ycgco.h"
#include "colorhelpers.h"
#include "correlation.h"
#include "datahelpers.h"
#include "dtc.h"
#include "exception.h"
#include "linefit.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <array>
#include <deque>
#include <optional>
#include <vector>

#include <iostream>

using namespace Color;

struct FrameHeader
{
    uint16_t flags = 0;         // e.g. FRAME_IS_PFRAME
    uint16_t nrOfRefBlocks = 0; // # of reference blocks in frame. rest is verbatim blocks

    std::array<uint8_t, 4> toArray() const
    {
        std::array<uint8_t, 4> result;
        *reinterpret_cast<uint16_t *>(&result[0]) = flags;
        *reinterpret_cast<uint16_t *>(&result[2]) = nrOfRefBlocks;
        return result;
    }
};

constexpr uint8_t FRAME_IS_PFRAME = 0x80;      // 0 for B-frames / key frames, 1 for P-frame / inter-frame compression ("predicted frame")
constexpr uint32_t BLOCK_FROM_PREVIOUS = 0x01; // The block is from from the previous frame
constexpr uint32_t BLOCK_REFERENCE = 0x02;     // The block is a reference into the current or previous frame

// Block flags mean:
// 0 | 0 --> new, full DXT block
// 0 | BLOCK_REFERENCE --> reference into current frame
// BLOCK_FROM_PREVIOUS | BLOCK_REFERENCE --> reference into previous frame
// BLOCK_FROM_PREVIOUS | 0 --> keep previous frame block

/// @brief 4x4 RGB verbatim block
using CodeBookEntry = std::array<YCgCoRd, 16>;

/// @brief List of code book entries representing the image
using CodeBook = std::vector<CodeBookEntry>;

struct DXTBlock
{
    uint16_t color0;
    uint16_t color1;
    uint32_t indices;

    auto toArray() const -> std::array<uint8_t, 8>
    {
        std::array<uint8_t, 8> result;
        *reinterpret_cast<uint16_t *>(&result[0]) = color0;
        *reinterpret_cast<uint16_t *>(&result[2]) = color1;
        *reinterpret_cast<uint32_t *>(&result[4]) = indices;
        return result;
    }

    /// @brief DXT-encodes one 4x4 block
    /// This is basically the "range fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
    static auto encode(const std::array<YCgCoRd, 16> &colors) -> DXTBlock
    {
        // calculate line fit through RGB color space
        auto originAndAxis = lineFit(colors);
        // calculate signed distance from origin
        std::vector<double> distanceFromOrigin(16);
        std::transform(colors.cbegin(), colors.cend(), distanceFromOrigin.begin(), [origin = originAndAxis.first, axis = originAndAxis.second](const auto &color)
                       { return color.dot(axis); });
        // get the distance of endpoints c0 and c1 on line
        auto minMaxDistance = std::minmax_element(distanceFromOrigin.cbegin(), distanceFromOrigin.cend());
        auto indexC0 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.first);
        auto indexC1 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.second);
        // get colors c0 and c1 on line and round to grid
        auto c0 = colors[indexC0];
        auto c1 = colors[indexC1];
        YCgCoRd endpoints[4] = {c0, c1, {}, {}};
        /*if (toPixel(endpoints[0]) > toPixel(endpoints[1]))
        {
            std::swap(c0, c1);
            std::swap(endpoints[0], endpoints[1]);
        }*/
        // calculate intermediate colors c2 and c3 (rounded like in decoder)
        endpoints[2] = YCgCoRd::roundToRGB555((c0.cwiseProduct(YCgCoRd(2, 2, 2)) + c1).cwiseQuotient(YCgCoRd(3, 3, 3)));
        endpoints[3] = YCgCoRd::roundToRGB555((c0 + c1.cwiseProduct(YCgCoRd(2, 2, 2))).cwiseQuotient(YCgCoRd(3, 3, 3)));
        // calculate minimum distance for all colors to endpoints
        std::array<uint32_t, 16> bestIndices = {0};
        for (uint32_t ci = 0; ci < 16; ++ci)
        {
            // calculate minimum distance for each index for this color
            double bestColorDistance = std::numeric_limits<double>::max();
            for (uint32_t ei = 0; ei < 4; ++ei)
            {
                auto indexDistance = YCgCoRd::distance(colors[ci], endpoints[ei]);
                // check if result improved
                if (bestColorDistance > indexDistance)
                {
                    bestColorDistance = indexDistance;
                    bestIndices[ci] = ei;
                }
            }
        }
        // reverse index data
        uint32_t indices = 0;
        for (auto iIt = bestIndices.crbegin(); iIt != bestIndices.crend(); ++iIt)
        {
            indices = (indices << 2) | *iIt;
        }
        return {toBGR555(c0.toRGB555()), toBGR555(c1.toRGB555()), indices};
    }

    static auto decode(const DXTBlock &block) -> std::array<uint16_t, 16>
    {
        std::array<uint16_t, 4> colors;
        auto c0 = YCgCoRd::fromRGB555(block.color0);
        auto c1 = YCgCoRd::fromRGB555(block.color1);
        colors[0] = block.color0;
        colors[1] = block.color1;
        colors[2] = YCgCoRd((c0.cwiseProduct(YCgCoRd(2, 2, 2)) + c1).cwiseQuotient(YCgCoRd(3, 3, 3))).toRGB555();
        colors[3] = YCgCoRd((c0 + c1.cwiseProduct(YCgCoRd(2, 2, 2))).cwiseQuotient(YCgCoRd(3, 3, 3))).toRGB555();
        uint32_t shift = 0;
        std::array<uint16_t, 16> result;
        for (uint32_t i = 0; i < result.size(); ++i, shift += 2)
        {
            result[i] = colors[(block.indices >> shift) & 0x3];
        }
        return result;
    }
};

// Convert an image to a codebook
auto toCodebook(const std::vector<uint16_t> &image, uint32_t width, uint32_t height) -> CodeBook
{
    CodeBook result(width / 4 * height / 4);
    auto cbIt = result.begin();
    for (uint32_t y = 0; y < height; y += 4)
    {
        for (uint32_t x = 0; x < width; x += 4)
        {
            // get block colors for all 16 pixels
            auto pixel = image.data() + y * width + x;
            auto eIt = cbIt->begin();
            for (int y = 0; y < 4; y++)
            {
                *eIt++ = YCgCoRd::fromRGB555(pixel[0]);
                *eIt++ = YCgCoRd::fromRGB555(pixel[1]);
                *eIt++ = YCgCoRd::fromRGB555(pixel[2]);
                *eIt++ = YCgCoRd::fromRGB555(pixel[3]);
                pixel += width;
            }
            ++cbIt;
        }
    }
    return result;
}

/// @brief Copy 4x4 image block to from src to dst
auto copyImageBlock(std::vector<uint16_t> &dstImage, const std::array<uint16_t, 16> &srcBlock, uint32_t width, uint32_t height, int32_t blockIndex) -> void
{
    auto blockPixelOffset = ((blockIndex / (width / 4)) * width * 4) + ((blockIndex % (width / 4)) * 4);
    auto dstPixel = dstImage.data() + blockPixelOffset;
    auto srcPixel = srcBlock.data();
    for (uint32_t y = 0; y < 4; ++y)
    {
        for (uint32_t x = 0; x < 4; ++x)
        {
            dstPixel[x] = *srcPixel++;
        }
        dstPixel += width;
    }
}

/// @brief Copy 4x4 image block to from src to dst
auto copyImageBlock(std::vector<uint16_t> &dstImage, const std::vector<uint16_t> &srcImage, uint32_t width, uint32_t height, int32_t blockIndex) -> void
{
    auto blockPixelOffset = ((blockIndex / (width / 4)) * width * 4) + ((blockIndex % (width / 4)) * 4);
    auto dstPixel = dstImage.data() + blockPixelOffset;
    auto srcPixel = srcImage.data() + blockPixelOffset;
    for (uint32_t y = 0; y < 4; ++y)
    {
        for (uint32_t x = 0; x < 4; ++x)
        {
            dstPixel[x] = srcPixel[x];
        }
        srcPixel += width;
        dstPixel += width;
    }
}

/// @brief Search how many following blocks would use and entry as a reference
/// @return Returns number of blocks matching
auto findFollowingMatches(const CodeBook &codebook, const CodeBookEntry &entry, double maxAllowedError, int32_t blockIndex, int32_t distanceMax) -> int32_t
{
    if (codebook.empty())
    {
        return 0;
    }
    // calculate start and end of search
    auto minIndex = blockIndex + 1;
    minIndex = minIndex < 0 ? 0 : minIndex;
    minIndex = minIndex >= codebook.size() ? codebook.size() - 1 : minIndex;
    auto maxIndex = blockIndex + distanceMax;
    maxIndex = maxIndex < 0 ? 0 : maxIndex;
    maxIndex = maxIndex >= codebook.size() ? codebook.size() - 1 : maxIndex;
    // searched entries must be >= 1
    if ((maxIndex - minIndex) < 1)
    {
        return 0;
    }
    assert(maxIndex - minIndex >= 1);
    // calculate codebook errors and check if below allowed error
    auto start = std::next(codebook.cbegin(), minIndex);
    auto end = std::next(codebook.cbegin(), maxIndex);
    return static_cast<int32_t>(std::count_if(start, end, [entry, maxAllowedError](const auto &b)
                                              { return YCgCoRd::distance(entry, b) < maxAllowedError; }));
}

/// @brief Search for entry in codebook with minimum error
/// @return Returns (error, entry index) if usable entry found or empty optional, if not
auto findBestMatchingBlock(const CodeBook &codebook, int32_t nrOfBlocks, const CodeBookEntry &entry, double maxAllowedError, int32_t blockIndex, int32_t distanceMin, int32_t distanceMax) -> std::optional<std::pair<double, int32_t>>
{
    if (codebook.empty())
    {
        return std::optional<std::pair<double, int32_t>>();
    }
    // calculate start and end of search
    auto minIndex = blockIndex + distanceMin;
    minIndex = minIndex < 0 ? 0 : minIndex;
    minIndex = minIndex >= nrOfBlocks ? nrOfBlocks - 1 : minIndex;
    auto maxIndex = blockIndex + distanceMax;
    maxIndex = maxIndex < 0 ? 0 : maxIndex;
    maxIndex = maxIndex >= nrOfBlocks ? nrOfBlocks - 1 : maxIndex;
    // searched entries must be >= 1
    if ((maxIndex - minIndex) < 1)
    {
        return std::optional<std::pair<double, int32_t>>();
    }
    assert(maxIndex - minIndex >= 1);
    // calculate codebook errors
    std::deque<std::pair<double, int32_t>> errors;
    auto start = std::next(codebook.cbegin(), minIndex);
    auto end = std::next(codebook.cbegin(), maxIndex);
    std::transform(start, end, std::front_inserter(errors), [entry, index = minIndex](const auto &b) mutable
                   { return std::make_pair(YCgCoRd::distance(entry, b), index++); });
    // stable sort by error
    std::stable_sort(errors.begin(), errors.end(), [](const auto &a, const auto &b)
                     { return a.first < b.first; });
    // find codebook that has minimum error
    auto bestErrorIt = std::min_element(errors.cbegin(), errors.cend());
    return (bestErrorIt->first < maxAllowedError) ? std::optional<std::pair<double, int32_t>>({bestErrorIt->first, bestErrorIt->second}) : std::optional<std::pair<double, int32_t>>();
}

int32_t currentRefBlock = 0;
int32_t previousRefPos = 0;
int32_t previousRefNeg = 0;
int32_t keepBlock = 0;
int32_t repeatBlock = 0;
int32_t matchBlock = 0;

auto DXTV::encodeDXTV(const std::vector<uint16_t> &image, const std::vector<uint16_t> &previousImage, uint32_t width, uint32_t height, bool keyFrame, double maxBlockError) -> std::pair<std::vector<uint8_t>, std::vector<uint16_t>>
{
    static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
    REQUIRE(width % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXTV compression");
    REQUIRE(height % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXTV compression");
    REQUIRE((width / 4 * height / 4) % 8 == 0, std::runtime_error, "Number of 4x4 block must be a multiple of 8 for DXTV compression");
    REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "Max. block error must be in [0.01,1]");
    // divide max block error to get into internal range
    maxBlockError /= 1000;
    // convert frames to codebooks
    auto currentCodeBook = toCodebook(image, width, height);
    CodeBook previousCodeBook = keyFrame ? CodeBook() : toCodebook(previousImage, width, height);
    // compress frame
    std::vector<uint16_t> decompressedFrame(width * height);
    uint16_t blockFlags = 0;        // flags for current 8 blocks
    std::vector<uint8_t> flags;     // block flags store flags for 8 blocks (8*2 bits = 2 bytes)
    std::vector<uint8_t> refBlocks; // stores block references
    std::vector<uint8_t> dxtBlocks; // stores verbatim DXT blocks
    FrameHeader frameHeader;
    frameHeader.flags = keyFrame ? 0 : FRAME_IS_PFRAME;
    // loop through source images blocks
    currentRefBlock = 0;
    previousRefPos = 0;
    previousRefNeg = 0;
    keepBlock = 0;
    repeatBlock = 0;
    matchBlock = 0;
    for (int32_t blockIndex = 0; blockIndex < currentCodeBook.size();)
    {
        auto &entry = currentCodeBook[blockIndex];
        // compare entry to existing codebook entries in list
        blockFlags >>= 2;
        if (keyFrame)
        {
            // for B-frames / key frames, search the last -1 to -256 entries of the current frame
            auto currentMatch = findBestMatchingBlock(currentCodeBook, blockIndex + 1, entry, maxBlockError, blockIndex, -256, -1);
            if (currentMatch.has_value())
            {
                currentRefBlock++;
                repeatBlock += (blockIndex - currentMatch.value().second) == 1 ? 1 : 0;

                // if we've found a usable codebook entry, use the relative index to it (-1, as it is never 0)
                int32_t offset = blockIndex - currentMatch.value().second - 1;
                assert(offset >= 0 && offset <= 255);
                refBlocks.push_back(static_cast<uint8_t>(offset));
                blockFlags |= (BLOCK_REFERENCE << 14);
                // replace current entry by referenced entry
                entry = currentCodeBook[currentMatch.value().second];
                // store decompressed block to image
                copyImageBlock(decompressedFrame, image, width, height, currentMatch.value().second);
            }
            else
            {
                // else insert the codebook entry itself
                auto block = DXTBlock::encode(entry);
                auto rawBlock = block.toArray();
                std::copy(rawBlock.cbegin(), rawBlock.cend(), std::back_inserter(dxtBlocks));
                // store decompressed block to image
                copyImageBlock(decompressedFrame, DXTBlock::decode(block), width, height, blockIndex);
            }
        }
        else
        {
            // P-frame / inter-frame, search the last -15 to 240 entries of the previous frame
            auto previousMatch = findBestMatchingBlock(previousCodeBook, previousCodeBook.size(), entry, maxBlockError, blockIndex, -15, 240);
            // search the last -1 to -256 entries of the current frame
            auto currentMatch = findBestMatchingBlock(currentCodeBook, blockIndex + 1, entry, maxBlockError, blockIndex, -256, -1);
            // select previous or current match
            if (previousMatch.has_value() && (!currentMatch.has_value() || currentMatch.value() > previousMatch.value()))
            {
                previousRefPos += (blockIndex - previousMatch.value().second) < 0 ? 1 : 0;
                previousRefNeg += (blockIndex - previousMatch.value().second) > 0 ? 1 : 0;
                keepBlock += (blockIndex - previousMatch.value().second) == 0 ? 1 : 0;

                if (blockIndex == previousMatch.value().second)
                {
                    // the block should be kept. only set flags accordingly
                    blockFlags |= (BLOCK_FROM_PREVIOUS << 14);
                }
                else
                {
                    // if we've found a usable codebook entry, store the index [-15,240]->[0,255]
                    int32_t offset = blockIndex - previousMatch.value().second + 15;
                    assert(offset >= 0 && offset <= 255);
                    refBlocks.push_back(static_cast<uint8_t>(offset));
                    blockFlags |= ((BLOCK_FROM_PREVIOUS | BLOCK_REFERENCE) << 14);
                }
                // replace current entry by referenced entry
                entry = previousCodeBook[previousMatch.value().second];
                // store decompressed block to image
                copyImageBlock(decompressedFrame, previousImage, width, height, previousMatch.value().second);
            }
            else if (currentMatch.has_value() && (!previousMatch.has_value() || previousMatch.value() > currentMatch.value()))
            {
                currentRefBlock++;
                repeatBlock += (blockIndex - currentMatch.value().second) == 1 ? 1 : 0;

                // if we've found a usable codebook entry, use the relative index to it (-1, as it is never 0)
                int32_t offset = blockIndex - currentMatch.value().second - 1;
                assert(offset >= 0 && offset <= 255);
                refBlocks.push_back(static_cast<uint8_t>(offset));
                blockFlags |= (BLOCK_REFERENCE << 14);
                // replace current entry by referenced entry
                entry = currentCodeBook[currentMatch.value().second];
                // store decompressed block to image
                copyImageBlock(decompressedFrame, image, width, height, currentMatch.value().second);
            }
            else
            {
                // else insert the codebook entry itself
                auto block = DXTBlock::encode(entry);
                auto rawBlock = block.toArray();
                std::copy(rawBlock.cbegin(), rawBlock.cend(), std::back_inserter(dxtBlocks));
                // store decompressed block to image
                copyImageBlock(decompressedFrame, DXTBlock::decode(block), width, height, blockIndex);
            }
        }
        // store and clear block flags every 16 blocks
        blockIndex++;
        if ((blockIndex % 8) == 0)
        {
            flags.push_back((blockFlags >> 0) & 0xFF);
            flags.push_back((blockFlags >> 8) & 0xFF);
            blockFlags = 0;
        }
    }
    // std::cout << "Curr: " << currentRefBlock << ", match: " << matchBlock << ", prev -/+: " << previousRefNeg << "/" << previousRefPos << ", repeat: " << repeatBlock << ", keep: " << keepBlock << std::endl;
    //  add frame header to compressedData
    std::vector<uint8_t> compressedData;
    frameHeader.nrOfRefBlocks = static_cast<uint16_t>(refBlocks.size());
    auto headerData = frameHeader.toArray();
    assert((headerData.size() % 4) == 0);
    std::copy(headerData.cbegin(), headerData.cend(), std::back_inserter(compressedData));
    // expand block flags to multiple of 4 and copy to compressedData
    flags = fillUpToMultipleOf(flags, 4);
    assert((flags.size() % 4) == 0);
    std::copy(flags.cbegin(), flags.cend(), std::back_inserter(compressedData));
    // if we have reference blocks, expand to multiple of 4 and copy to compressedData
    if (!refBlocks.empty())
    {
        refBlocks = fillUpToMultipleOf(refBlocks, 4);
        assert((refBlocks.size() % 4) == 0);
        std::copy(refBlocks.cbegin(), refBlocks.cend(), std::back_inserter(compressedData));
    }
    // copy DXT blocks to compressedData. they're always 8 bytes in size
    std::copy(dxtBlocks.cbegin(), dxtBlocks.cend(), std::back_inserter(compressedData));
    assert((compressedData.size() % 4) == 0);
    return {compressedData, decompressedFrame};
}

auto DXTV::decodeDXTV(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint16_t>
{
    return {};
}