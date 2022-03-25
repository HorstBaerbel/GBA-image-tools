#include "codec_dxtv.h"

#include "color_ycgco.h"
#include "colorhelpers.h"
#include "datahelpers.h"
#include "exception.h"
#include "linefit.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <array>
#include <deque>
#include <vector>

#include <iostream>

using namespace Color;

constexpr double MaxKeyFrameBlockError = 0.0001; // Maximum error allowed for key frame block references [0,1]

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

constexpr uint8_t FRAME_IS_PFRAME = 0x80; // 0 for key frames, 1 for inter-frame compression ("predicted frame")

constexpr uint32_t BLOCK_KEEP = 0x01;         // If bit is 1 the current block is kept (copied from previous frame) and no reference or code block entry is sent
constexpr uint32_t BLOCK_IS_REFERENCE = 0x02; // If bit is 1 the current block is a reference, else it is a new, full code book entry

/// @brief Reference to code book entry for intra-frame compression. References the current codebook / frame
struct BlockReferenceIntraFrame
{
    uint8_t index; // negative relative index of code book entry / frame block to use [0,255]->[1-256]
};

/// @brief Reference to code book entry for inter-frame compression / P-frames. References the current or previous codebook / frame
struct BlockReferenceInterFrame
{
    uint8_t previousFrame : 1; // if 1 this references the previous code book / frame block, if 0 the current one
    uint8_t index : 7;         // negative relative index of code book entry / frame block to use [0,127]->[1-128]
};

/// @brief 4x4 RGB verbatim block
using CodeBookEntry = std::array<YCgCoRd, 16>;

/// @brief List of code book entries representing the image
using CodeBook = std::vector<CodeBookEntry>;

struct DXTBlock
{
    uint16_t color0;
    uint16_t color1;
    uint32_t indices;

    std::array<uint8_t, 8> toArray() const
    {
        std::array<uint8_t, 8> result;
        *reinterpret_cast<uint16_t *>(&result[0]) = color0;
        *reinterpret_cast<uint16_t *>(&result[2]) = color1;
        *reinterpret_cast<uint32_t *>(&result[4]) = indices;
        return result;
    }
};

// DTX-encodes one 4x4 block
// This is basically the "range fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
DXTBlock encodeBlock(const std::array<YCgCoRd, 16> &colors)
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

/// @brief Search for entry in codebook with minimum error
/// @return Returns (error, entry index) if usable entry found or empty optional, if not
auto findBestMatchingBlock(const CodeBook &codebook, const CodeBookEntry &entry, double maxAllowedError, int32_t currentIndex, int32_t distanceMin, int32_t distanceMax) -> std::optional<std::pair<double, int32_t>>
{
    if (codebook.empty())
    {
        return std::optional<std::pair<double, int32_t>>();
    }
    // caculate start and end of search
    auto maxIndex = currentIndex + distanceMin;
    maxIndex = maxIndex < 0 ? 0 : maxIndex;
    maxIndex = maxIndex >= codebook.size() ? codebook.size() - 1 : maxIndex;
    auto minIndex = currentIndex + distanceMax;
    minIndex = minIndex < 0 ? 0 : minIndex;
    minIndex = minIndex >= codebook.size() ? codebook.size() - 1 : minIndex;
    // searched entries must be >= 1
    if ((maxIndex - minIndex) < 1)
    {
        return std::optional<std::pair<double, int32_t>>();
    }
    assert(maxIndex - minIndex >= 1);
    // calculate codebook errors in reverse (increasing distance from current position)
    std::deque<std::pair<double, int32_t>> errors;
    auto start = std::next(codebook.cbegin(), minIndex);
    auto end = std::next(codebook.cbegin(), maxIndex);
    std::transform(start, end, std::front_inserter(errors), [entry, index = minIndex](const auto &b) mutable
                   { return std::make_pair(YCgCoRd::distance(entry, b), index++); });
    // stable sort by error
    std::stable_sort(errors.begin(), errors.end(), [](const auto &a, const auto &b)
                     { return a.first < b.first; });
    // find first codebook that is below max error
    auto bestErrorIt = std::find_if(errors.cbegin(), errors.cend(), [maxAllowedError](const auto &a)
                                    { return a.first < maxAllowedError; });
    return (bestErrorIt != errors.cend()) ? std::optional<std::pair<double, int32_t>>({bestErrorIt->first, bestErrorIt->second}) : std::optional<std::pair<double, int32_t>>();
}

auto DXTV::encodeDXTV(const std::vector<uint16_t> &image, uint32_t width, uint32_t height, bool keyFrame, double maxBlockError) -> std::vector<uint8_t>
{
    static_assert(sizeof(BlockReferenceInterFrame) == 1, "Size of intra-frame reference block must be 8 bit");
    static_assert(sizeof(BlockReferenceIntraFrame) == 1, "Size of inter-frame reference block must be 8 bit");
    static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
    REQUIRE(width % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXTV compression");
    REQUIRE(height % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXTV compression");
    REQUIRE((width / 4 * height / 4) % 8 == 0, std::runtime_error, "Number of 4x4 block must be a multiple of 8 for DXTV compression");
    // set up some variables
    uint32_t blockIndex = 0;        // 4x4 block index in frame
    uint16_t blockFlags = 0;        // flags for current 8 blocks
    std::vector<uint8_t> flags;     // block flags store flags for 8 blocks (8*2 bits = 2 bytes)
    std::vector<uint8_t> refBlocks; // stores block references
    std::vector<uint8_t> dxtBlocks; // stores verbatim DXT blocks
    CodeBook codebook;              // code book storing all codebook entries (when finished this equals the frame in RGB format)
    std::array<YCgCoRd, 16> colors; // current set of colors in 4x4 block
    FrameHeader frameHeader;
    frameHeader.flags = keyFrame ? 0 : FRAME_IS_PFRAME;
    // loop through source images blocks
    for (uint32_t y = 0; y < height; y += 4)
    {
        for (uint32_t x = 0; x < width; x += 4)
        {
            // get block colors for all 16 pixels
            auto pixel = image.data() + y * width + x;
            auto cIt = colors.begin();
            for (int y = 0; y < 4; y++)
            {
                *cIt++ = YCgCoRd::fromRGB555(pixel[0]);
                *cIt++ = YCgCoRd::fromRGB555(pixel[1]);
                *cIt++ = YCgCoRd::fromRGB555(pixel[2]);
                *cIt++ = YCgCoRd::fromRGB555(pixel[3]);
                pixel += width;
            }
            // compare codebook to existing codebooks in list
            if (keyFrame)
            {
                blockFlags >>= 2;
                // for key frames, search the last -1 to -256 entries of the current codebook
                auto bestMatch = findBestMatchingBlock(codebook, colors, MaxKeyFrameBlockError, blockIndex, -1, -256);
                if (bestMatch.has_value())
                {
                    // if we've found a usable codebook entry, use the relative index to it (-1, as it is never 0)
                    int32_t offset = blockIndex - bestMatch.value().second - 1;
                    assert(offset >= 0 && offset <= 255);
                    refBlocks.push_back(static_cast<uint8_t>(offset));
                    blockFlags |= (BLOCK_IS_REFERENCE << 14);
                    // insert referenced codebook entry into codebook
                    codebook.push_back(codebook[bestMatch.value().second]);
                }
                else
                {
                    // else insert the codebook entry itself
                    auto block = encodeBlock(colors).toArray();
                    std::copy(block.cbegin(), block.cend(), std::back_inserter(dxtBlocks));
                    // insert new codebook entry into codebook
                    codebook.push_back(colors);
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
    }
    // add frame header to result
    std::vector<uint8_t> result;
    frameHeader.nrOfRefBlocks = static_cast<uint16_t>(refBlocks.size());
    auto headerData = frameHeader.toArray();
    assert((headerData.size() % 4) == 0);
    std::copy(headerData.cbegin(), headerData.cend(), std::back_inserter(result));
    // expand block flags to multiple of 4 and copy to result
    flags = fillUpToMultipleOf(flags, 4);
    assert((flags.size() % 4) == 0);
    std::copy(flags.cbegin(), flags.cend(), std::back_inserter(result));
    // if we have reference blocks, expand to multiple of 4 and copy to result
    if (!refBlocks.empty())
    {
        refBlocks = fillUpToMultipleOf(refBlocks, 4);
        assert((refBlocks.size() % 4) == 0);
        std::copy(refBlocks.cbegin(), refBlocks.cend(), std::back_inserter(result));
    }
    // copy DXT blocks to result. they're always 8 bytes in size
    std::copy(dxtBlocks.cbegin(), dxtBlocks.cend(), std::back_inserter(result));
    assert((result.size() % 4) == 0);
    return result;
}

auto DXTV::decodeDXTV(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint8_t>
{
    return {};
}