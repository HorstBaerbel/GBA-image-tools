#include "gvid.h"

#include "color/ycgcod.h"
#include "color/colorhelpers.h"
#include "exception.h"

#include <Eigen/Core>
#include <Eigen/Dense>

#include <algorithm>
#include <array>
#include <deque>
#include <numeric>
#include <optional>
#include <iostream>

constexpr float MaxKeyFrameBlockError = 1.0F; // Maximum error allowed for key frame block references [0,6]

constexpr uint8_t FRAME_IS_PFRAME = 0x80; // 0 for key frames, 1 for inter-frame compression ("predicted frame")

constexpr uint32_t BLOCK_KEEP = 0x01;         // If bit is 1 the current block is kept (copied from previous frame) and no reference or code block entry is sent
constexpr uint32_t BLOCK_IS_REFERENCE = 0x02; // If bit is 1 the current block is a reference, else it is a new, full code book entry

/// @brief Reference to code book entry for intra-frame compression. References the current codebook / frame
struct BlockReferenceBFrame
{
    uint8_t index; // negative relative index of code book entry / frame block to use [0,255]->[1-256]
};

/// @brief Reference to code book entry for inter-frame compression / P-frames. References the current or previous codebook / frame
struct BlockReferencePFrame
{
    uint8_t previousFrame : 1; // if 1 this references the previous code book / frame block, if 0 the current one
    uint8_t index : 7;         // negative relative index of code book entry / frame block to use [0,127]->[1-128]
};

/// @brief YCgCo 4:2:0 verbatim block
struct BlockCodeBookEntry
{
    unsigned Y0 : 5;
    unsigned Y1 : 5;
    unsigned Y2 : 5;
    unsigned Y3 : 5;
    unsigned Cg : 6;
    unsigned Co : 6;

    operator uint32_t() const
    {
        return (Y0 << 27) | (Y1 << 22) | (Y2 << 17) | (Y3 << 12) | (Cg << 6) | Co;
    }
};

/// @brief YCgCo 4:2:0 block. Layout:
/// y0 Cg Co, y1 Cg Co
/// y2 Cg Co, y3 Cg Co
struct CodeBookEntry
{
    std::array<float, 4> Y = {0};
    float Cg = 0;
    float Co = 0;

    auto toBlock() const -> BlockCodeBookEntry
    {
        BlockCodeBookEntry result;
        result.Y0 = static_cast<uint8_t>(std::round(Y[0] * 31.0F));
        result.Y1 = static_cast<uint8_t>(std::round(Y[1] * 31.0F));
        result.Y2 = static_cast<uint8_t>(std::round(Y[2] * 31.0F));
        result.Y3 = static_cast<uint8_t>(std::round(Y[3] * 31.0F));
        result.Cg = static_cast<uint8_t>(std::round(Cg * 63.0F));
        result.Co = static_cast<uint8_t>(std::round(Co * 63.0F));
        return result;
    }

    auto distanceSqr(const CodeBookEntry &b) const -> float
    {
        float result = 0;
        result += (b.Y[0] - this->Y[0]) * (b.Y[0] - this->Y[0]);
        result += (b.Y[1] - this->Y[1]) * (b.Y[1] - this->Y[1]);
        result += (b.Y[2] - this->Y[2]) * (b.Y[2] - this->Y[2]);
        result += (b.Y[3] - this->Y[3]) * (b.Y[3] - this->Y[3]);
        result += (b.Cg - this->Cg) * (b.Cg - this->Cg);
        result += (b.Co - this->Co) * (b.Co - this->Co);
        return result; // in [0,6]
    }
};

using CodeBook = std::vector<CodeBookEntry>;

/// @brief Search for entry in codebook with minimum error
/// @return Returns (error, entry index) if usable entry found or empty optional, if not
auto findBestMatch(const CodeBook &codebook, const CodeBookEntry &entry, float maxAllowedError, int32_t currentIndex, int32_t distanceMin, int32_t distanceMax) -> std::optional<std::pair<float, int32_t>>
{
    if (codebook.empty())
    {
        return {};
    }
    // cacluate start and end of search
    auto maxIndex = currentIndex + distanceMin;
    maxIndex = maxIndex < 0 ? 0 : maxIndex;
    auto minIndex = currentIndex + distanceMax;
    minIndex = minIndex < 0 ? 0 : minIndex;
    minIndex = minIndex >= codebook.size() ? codebook.size() - 1 : minIndex;
    // searched entries must be >= 1
    if ((maxIndex - minIndex) < 1)
    {
        return {};
    }
    // calculate codebook errors in reverse (increasing distance from current position)
    std::deque<std::pair<float, int32_t>> errors;
    auto start = std::next(codebook.cbegin(), minIndex);
    auto end = std::next(codebook.cbegin(), maxIndex);
    std::transform(start, end, std::front_inserter(errors), [entry, index = minIndex](const auto &b) mutable
                   { return std::make_pair(entry.distanceSqr(b), index++); });
    // stable sort by error
    std::stable_sort(errors.begin(), errors.end(), [](const auto &a, const auto &b)
                     { return a.first < b.first; });
    // find first codebook that is below max error
    auto bestErrorIt = std::find_if(errors.cbegin(), errors.cend(), [maxAllowedError](const auto &a)
                                    { return a.first < maxAllowedError; });
    return (bestErrorIt != errors.cend()) ? std::optional<std::pair<float, int32_t>>({bestErrorIt->first, bestErrorIt->second}) : std::optional<std::pair<float, int32_t>>();
}

auto GVID::encodeGVID(const std::vector<uint8_t> &image, uint32_t width, uint32_t height, bool keyFrame, float maxBlockError) -> std::vector<uint8_t>
{
    static_assert(sizeof(BlockCodeBookEntry) == 4, "Size of code book block must be 32 bit");
    static_assert(sizeof(BlockReferenceBFrame) == 1, "Size of intra-frame reference block must be 8 bit");
    static_assert(sizeof(BlockReferenceBFrame) == 1, "Size of inter-frame reference block must be 8 bit");
    REQUIRE(width % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for GVID compression");
    REQUIRE(height % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for GVID compression");
    REQUIRE(image.size() % 3 == 0, std::runtime_error, "Image data size must be a multiple of 3 for GVID compression");
    const auto pixelsPerScanline = width * 3;
    // set up some variables
    uint32_t blockIndex = 0;               // 4x4 block index in frame
    uint32_t blockFlags = 0;               // flags for current 16 blocks
    std::vector<uint8_t> flags;            // block flags store flags for 16 blocks (16*2 bits = 4 bytes)
    std::vector<uint8_t> blocks;           // blocks store verbatim codebook entries or references
    CodeBook codebook;                     // code book storing all codebook entries (when finished this equals the frame in YCgCo format)
    std::array<Color::YCgCoRd, 16> colors; // current set of colors in 4x4 block
    // loop through source images blocks
    for (uint32_t y = 0; y < height; y += 4)
    {
        for (uint32_t x = 0; x < width; x += 4)
        {
            // get block colors for all 16 pixels
            auto pixels = image.data() + y * pixelsPerScanline + x;
            auto cIt = colors.begin();
            for (int by = 0; by < 4; by++)
            {
                *cIt++ = Color::YCgCoRd::fromRGB888(pixels);
                pixels += 3;
                *cIt++ = Color::YCgCoRd::fromRGB888(pixels);
                pixels += 3;
                *cIt++ = Color::YCgCoRd::fromRGB888(pixels);
                pixels += 3;
                *cIt++ = Color::YCgCoRd::fromRGB888(pixels);
                pixels += pixelsPerScanline - 3 * 3;
            }
            // convert block to codebook
            CodeBookEntry cbe;
            cbe.Y[0] = (colors[0].Y() + colors[1].Y() + colors[4].Y() + colors[5].Y()) / 4.0F;
            cbe.Y[1] = (colors[2].Y() + colors[3].Y() + colors[6].Y() + colors[7].Y()) / 4.0F;
            cbe.Y[2] = (colors[8].Y() + colors[9].Y() + colors[12].Y() + colors[13].Y()) / 4.0F;
            cbe.Y[3] = (colors[10].Y() + colors[11].Y() + colors[14].Y() + colors[15].Y()) / 4.0F;
            cbe.Cg = std::accumulate(colors.cbegin(), colors.cend(), 0.0F, [](auto v, auto c)
                                     { return v + c.Cg(); }) /
                     colors.size();
            cbe.Co = std::accumulate(colors.cbegin(), colors.cend(), 0.0F, [](auto v, auto c)
                                     { return v + c.Co(); }) /
                     colors.size();
            // compare codebook to existing codebooks in list
            if (keyFrame)
            {
                blockFlags >>= 2;
                // for key frames, search the last -1 to -256 entries of the current codebook
                auto bestMatch = findBestMatch(codebook, cbe, MaxKeyFrameBlockError, blockIndex, -1, -256);
                if (bestMatch.has_value())
                {
                    // if we've found a usable codebook entry, use the relative index to it (-1, as it is never 0)
                    int32_t offset = blockIndex - bestMatch.value().second - 1;
                    assert(offset >= 0 && offset <= 255);
                    blocks.push_back(static_cast<uint8_t>(offset));
                    blockFlags |= (BLOCK_IS_REFERENCE << 30);
                    // insert referenced codebook entry into codebook
                    codebook.push_back(codebook[bestMatch.value().second]);
                }
                else
                {
                    // else insert the codebook entry itself
                    auto entry = static_cast<uint32_t>(cbe.toBlock());
                    blocks.push_back((entry >> 24) & 0xFF);
                    blocks.push_back((entry >> 16) & 0xFF);
                    blocks.push_back((entry >> 8) & 0xFF);
                    blocks.push_back((entry >> 0) & 0xFF);
                    // insert new codebook entry into codebook
                    codebook.push_back(cbe);
                }
            }
            // store and clear block flags every 16 blocks
            blockIndex++;
            if (blockIndex % 16 == 0)
            {
                flags.push_back((blockFlags >> 24) & 0xFF);
                flags.push_back((blockFlags >> 16) & 0xFF);
                flags.push_back((blockFlags >> 8) & 0xFF);
                flags.push_back((blockFlags >> 0) & 0xFF);
                blockFlags = 0;
            }
        }
    }
    // combine frame flags, flags and block data
    std::vector<uint8_t> result;
    result.push_back(keyFrame ? 0 : FRAME_IS_PFRAME);
    std::copy(flags.cbegin(), flags.cend(), std::back_inserter(result));
    std::copy(blocks.cbegin(), blocks.cend(), std::back_inserter(result));
    return result;
}

auto decodeGVID(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint8_t>
{
    return {};
}