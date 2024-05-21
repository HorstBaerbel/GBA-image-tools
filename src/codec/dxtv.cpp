#include "dxtv.h"

#include "blockview.h"
#include "codebook.h"
#include "color/conversions.h"
#include "color/psnr.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "dxtblock.h"
#include "exception.h"
#include "math/linefit.h"
#include "processing/datahelpers.h"

#include <Eigen/Core>
#include <Eigen/Dense>

#include <array>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

using namespace Color;

struct FrameHeader
{
    uint16_t flags = 0;     // e.g. FRAME_IS_PFRAME
    uint16_t nrOfFlags = 0; // # of blocks > MinDim in frame (determines the size of flags block)

    std::array<uint8_t, 4> toArray() const
    {
        std::array<uint8_t, 4> result;
        *reinterpret_cast<uint16_t *>(&result[0]) = flags;
        *reinterpret_cast<uint16_t *>(&result[2]) = nrOfFlags;
        return result;
    }
};

// The image is split into 16x16 pixel blocks which can be futher split into 8x8 and 4x4 blocks.
//
// Every 16x16 (block size 0) block has one flag:
// Bit 0: Block handled entirely (0) or block split into 8x8 (1)
//
// Every 8x8 block (block size 1) has one flag:
// Bit 0: Block handled entirely (0) or block split into 4x4 (1)
//
// 4x4 block (block size 2) has no flags. If an 8x8 block has been split,
// 4 block references or DXT blocks will be read from data.
//
// Block flags are sent depth first. So if a 16x16 block is split into 4 8x8 children ABCD, its first 8x8 child A is sent first.
// If that child is split, its 4 4x4 children CDEF are sent first, then child B and so on. This makes sure no flag bits are wasted.
//
// DXT and reference blocks differ in their Bit 15. If 0 it is a DXT-block, if 1 a reference block.
// - DXT blocks store verbatim DXT data (2 * uint16_t RGB555 colors and index data depending on blocks size).
//   Due to RGB555 having bit 15 == 0, the correct bit is automatically set.
// - Reference blocks store:
//   Bit 15: Always 1 (see above)
//   Bit 14: Current (0) / previous (1) frame Bit
//   Bit 0-13: Reference index into frame [0,16383]
//             Range [-16384,-1] is used for references to the current frame.
//             Range [-8191,8192] is used for references to the previous frame.

constexpr uint16_t FRAME_IS_PFRAME = 0x80; // 0 for B-frames / key frames, 1 for P-frame / inter-frame compression ("predicted frame")
constexpr uint16_t FRAME_KEEP = 0x40;      // 1 for frames that are considered a direct copy of the previous frame and can be kept

constexpr bool BLOCK_NO_SPLIT = false;             // The block is a full block
constexpr bool BLOCK_IS_SPLIT = true;              // The block is split into smaller sub-blocks
constexpr uint16_t BLOCK_IS_DXT = 0;               // The block is a verbatim DXT block
constexpr uint16_t BLOCK_IS_REF = (1 << 15);       // The block is a reference into the current or previous frame
constexpr uint16_t BLOCK_FROM_CURR = (0 << 14);    // The reference block is from from the current frame
constexpr uint16_t BLOCK_FROM_PREV = (1 << 14);    // The reference block is from from the previous frame
constexpr uint32_t BLOCK_INDEX_MASK = ~(3U << 14); // Mask to get the block index from the reference info

constexpr std::pair<int32_t, int32_t> CurrRefOffset{-16384, -1};  // Block search offsets for current frame for 16, 8, 4
constexpr std::pair<int32_t, int32_t> PrevRefOffset{-8191, 8192}; // Block search offsets for previous frame for 16, 8, 4

/// @brief Calculate perceived pixel difference between blocks
template <std::size_t BLOCK_DIM>
static auto mse(const BlockView<RGBf, BLOCK_DIM> &a, const BlockView<RGBf, BLOCK_DIM> &b) -> float
{
    float dist = 0.0F;
    for (auto aIt = a.cbegin(), bIt = b.cbegin(); aIt != a.cend() && bIt != b.cend(); ++aIt, ++bIt)
    {
        dist += RGBf::mse(*aIt, *bIt);
    }
    return dist / (BLOCK_DIM * BLOCK_DIM);
}

/// @brief Calculate perceived pixel difference between blocks
template <std::size_t BLOCK_DIM>
static auto mseBelowThreshold(const BlockView<RGBf, BLOCK_DIM> &a, const BlockView<RGBf, BLOCK_DIM> &b, float threshold) -> std::pair<bool, float>
{
    bool belowThreshold = true;
    float dist = 0.0F;
    for (auto aIt = a.cbegin(), bIt = b.cbegin(); aIt != a.cend() && bIt != b.cend(); ++aIt, ++bIt)
    {
        auto colorDist = RGBf::mse(*aIt, *bIt);
        if (belowThreshold)
        {
            belowThreshold = colorDist < threshold;
        }
        dist += colorDist;
    }
    return {belowThreshold, dist / (BLOCK_DIM * BLOCK_DIM)};
}

/// @brief Search for entry in codebook with minimum error
/// @return Returns (error, entry index) if usable entry found or empty optional, if not
template <std::size_t BLOCK_DIM>
auto findBestMatchingBlock(const CodeBook<RGBf> &codeBook, const BlockView<RGBf, BLOCK_DIM> &block, float maxAllowedError, int32_t offsetMin, int32_t offsetMax) -> std::optional<std::pair<float, BlockView<RGBf, BLOCK_DIM>>>
{
    using return_type = std::pair<float, BlockView<RGBf, BLOCK_DIM>>;
    if (codeBook.empty<BLOCK_DIM>())
    {
        return std::optional<return_type>();
    }
    // calculate start and end of search
    int32_t minIndex = static_cast<int32_t>(block.index()) + offsetMin;
    minIndex = minIndex < 0 ? 0 : minIndex;
    minIndex = minIndex >= codeBook.size<BLOCK_DIM>() ? codeBook.size<BLOCK_DIM>() - 1 : minIndex;
    int32_t maxIndex = static_cast<int32_t>(block.index()) + offsetMax;
    maxIndex = maxIndex < 0 ? 0 : maxIndex;
    maxIndex = maxIndex >= codeBook.size<BLOCK_DIM>() ? codeBook.size<BLOCK_DIM>() - 1 : maxIndex;
    // searched entries must be >= 1
    if ((maxIndex - minIndex) < 1)
    {
        return std::optional<return_type>();
    }
    // find blocks that are already encoded in codebook and calculate mean squared error compared to block
    std::vector<std::pair<float, int32_t>> candidates;
    auto cIt = std::next(codeBook.cbegin<BLOCK_DIM>(), minIndex);
    auto cEnd = std::next(codeBook.cbegin<BLOCK_DIM>(), maxIndex);
    for (int32_t index = minIndex; cIt != cEnd; ++cIt, ++index)
    {
        if (codeBook.isEncoded(*cIt))
        {
            if (auto dist = mseBelowThreshold(block, *cIt, maxAllowedError); dist.first)
            {
                candidates.push_back({dist.second, index});
            }
        }
    }
    // find block that has minimum error
    auto bestCandiateIt = std::min_element(candidates.cbegin(), candidates.cend(), [](const auto &a, const auto &b)
                                           { return a.first < b.first; });
    return (bestCandiateIt != candidates.cend()) ? std::optional<return_type>({bestCandiateIt->first, *std::next(codeBook.cbegin<BLOCK_DIM>(), bestCandiateIt->second)}) : std::optional<return_type>();
}

struct Statistics
{
    std::array<uint32_t, 3> refBlocksCurr;
    std::array<uint32_t, 3> refBlocksPrev;
    std::array<uint32_t, 3> dxtBlocks;
};

Statistics statistics;

/// @brief Store state of compression of one frame
struct CompressionState
{
    std::vector<bool> flags;   // block flags store flags for blocks (2 bit per block of any size)
    std::vector<uint8_t> data; // stores block references and DXT data
};

template <std::size_t BLOCK_DIM>
auto storeDxtBlock(CodeBook<RGBf> &currentCodeBook, BlockView<RGBf, BLOCK_DIM> &block, const DXTBlock<BLOCK_DIM, BLOCK_DIM> &encodedBlock, const std::array<RGBf, BLOCK_DIM * BLOCK_DIM> &decodedBlock, CompressionState &state) -> void
{
    static constexpr std::size_t BLOCK_LEVEL = std::log2(CodeBook<RGBf>::BlockMaxDim) - std::log2(BLOCK_DIM);
    auto dxtData = encodedBlock.toArray();
    std::copy(dxtData.cbegin(), dxtData.cend(), std::back_inserter(state.data));
    // mark block as encoded
    currentCodeBook.setEncoded<BLOCK_DIM>(block);
    statistics.dxtBlocks[BLOCK_LEVEL]++;
}

template <std::size_t BLOCK_DIM>
auto storeRefBlock(CodeBook<RGBf> &currentCodeBook, BlockView<RGBf, BLOCK_DIM> &block, BlockView<RGBf, BLOCK_DIM> &srcBlock, CompressionState &state, bool fromPrevCodeBook) -> void
{
    static constexpr std::size_t BLOCK_LEVEL = std::log2(CodeBook<RGBf>::BlockMaxDim) - std::log2(BLOCK_DIM);
    // get referenced block
    REQUIRE(srcBlock.index() >= 0 && srcBlock.index() <= BLOCK_INDEX_MASK, std::runtime_error, "Frame reference block index out of range");
    auto index = static_cast<uint16_t>(srcBlock.index());
    if (fromPrevCodeBook)
    {
        index |= BLOCK_IS_REF | BLOCK_FROM_PREV;
        statistics.refBlocksPrev[BLOCK_LEVEL]++;
    }
    else
    {
        index |= BLOCK_IS_REF | BLOCK_FROM_CURR;
        statistics.refBlocksCurr[BLOCK_LEVEL]++;
    }
    state.data.push_back(index & 0xFF);
    state.data.push_back((index >> 8) & 0xFF);
    // mark block as encoded
    currentCodeBook.setEncoded<BLOCK_DIM>(block);
}

template <std::size_t BLOCK_DIM>
auto encodeBlock(CodeBook<RGBf> &currentCodeBook, const CodeBook<RGBf> &previousCodeBook, BlockView<RGBf, BLOCK_DIM> &block, CompressionState &state, float maxAllowedError) -> void
{
    static constexpr std::size_t BLOCK_LEVEL = std::log2(CodeBook<RGBf>::BlockMaxDim) - std::log2(BLOCK_DIM);
    // Try to reference block from the previous code book (if available) within error
    auto previousRef = findBestMatchingBlock(previousCodeBook, block, maxAllowedError, PrevRefOffset.first, PrevRefOffset.second);
    // Try to reference block from the current code book within error
    auto currentRef = findBestMatchingBlock(currentCodeBook, block, maxAllowedError, CurrRefOffset.first, CurrRefOffset.second);
    // Choose the better one of both block references
    const bool prevRefIsBetter = previousRef.has_value() && (!currentRef.has_value() || previousRef.value().first <= currentRef.value().first);
    const bool currRefIsBetter = currentRef.has_value() && (!previousRef.has_value() || currentRef.value().first <= previousRef.value().first);
    if (prevRefIsBetter)
    {
        if constexpr (BLOCK_DIM > CodeBook<RGBf>::BlockMinDim)
        {
            // only store bit for blocks > 4x4
            state.flags.push_back(BLOCK_NO_SPLIT);
        }
        // store reference from previous frame
        storeRefBlock(currentCodeBook, block, previousRef.value().second, state, true);
    }
    else if (currRefIsBetter)
    {
        if constexpr (BLOCK_DIM > CodeBook<RGBf>::BlockMinDim)
        {
            // only store bit for blocks > 4x4
            state.flags.push_back(BLOCK_NO_SPLIT);
        }
        // store reference from current frame
        storeRefBlock(currentCodeBook, block, currentRef.value().second, state, false);
    }
    else
    {
        // No good references found. DXT-encode full block
        auto rawBlock = block.colors();
        auto encodedBlock = DXTBlock<BLOCK_DIM, BLOCK_DIM>::encode(rawBlock);
        auto decodedBlock = DXTBlock<BLOCK_DIM, BLOCK_DIM>::decode(encodedBlock);
        if constexpr (BLOCK_DIM <= CodeBook<RGBf>::BlockMinDim)
        {
            //  We can't split anymore. Store 4x4 DXT block
            storeDxtBlock(currentCodeBook, block, encodedBlock, decodedBlock, state);
        }
        else if constexpr (BLOCK_DIM > CodeBook<RGBf>::BlockMinDim)
        {
            // check if encoded block is below allowed error or we want to split the block
            auto encodedBlockDist = Color::mse(rawBlock, decodedBlock);
            if (encodedBlockDist < maxAllowedError)
            {
                // Threshold ok. Store full DXT block
                state.flags.push_back(BLOCK_NO_SPLIT);
                storeDxtBlock(currentCodeBook, block, encodedBlock, decodedBlock, state);
            }
            else
            {
                // Split block and recurse
                state.flags.push_back(BLOCK_IS_SPLIT);
                encodeBlock(currentCodeBook, previousCodeBook, block.block(0), state, maxAllowedError);
                encodeBlock(currentCodeBook, previousCodeBook, block.block(1), state, maxAllowedError);
                encodeBlock(currentCodeBook, previousCodeBook, block.block(2), state, maxAllowedError);
                encodeBlock(currentCodeBook, previousCodeBook, block.block(3), state, maxAllowedError);
            }
        }
    }
}

auto DXTV::encodeDXTV(const std::vector<XRGB8888> &image, const std::vector<XRGB8888> &previousImage, uint32_t width, uint32_t height, bool keyFrame, float maxBlockError) -> std::pair<std::vector<uint8_t>, std::vector<XRGB8888>>
{
    static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
    REQUIRE(width % CodeBook<RGBf>::BlockMaxDim == 0, std::runtime_error, "Image width must be a multiple of 16 for DXTV compression");
    REQUIRE(height % CodeBook<RGBf>::BlockMaxDim == 0, std::runtime_error, "Image height must be a multiple of 16 for DXTV compression");
    REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "Max. block error must be in [0.01,1]");
    // divide max block error to get into internal range
    maxBlockError /= 1000;
    // convert frames to codebooks
    auto currentCodeBook = CodeBook<RGBf>(image, width, height, false);
    const CodeBook previousCodeBook = previousImage.empty() || keyFrame ? CodeBook<RGBf>() : CodeBook<RGBf>(previousImage, width, height, true);
    // calculate perceived frame distance
    const float frameDistance = previousCodeBook.empty<CodeBook<RGBf>::BlockMaxDim>() ? INT_MAX : currentCodeBook.distance(previousCodeBook);
    // check if the new frame can be considered a verbatim copy
    if (!keyFrame && frameDistance < 0.001)
    {
        // frame is a duplicate. pass header only
        FrameHeader frameHeader;
        frameHeader.flags = FRAME_KEEP;
        std::vector<uint8_t> compressedData;
        auto headerData = frameHeader.toArray();
        assert((headerData.size() % 4) == 0);
        std::copy(headerData.cbegin(), headerData.cend(), std::back_inserter(compressedData));
        return std::make_pair(compressedData, previousImage);
    }
    // if we don't have a keyframe, check for scene change
    /*if (!keyFrame)
    {
        if (frameDistance > 0.03)
        {
            std::cout << "Inserting key frame " << keyFrames++ << std::endl;
            keyFrame = true;
        }
    }*/
    // compress frame
    CompressionState state;
    statistics = Statistics();
    // loop through source images blocks
    for (auto cbIt = currentCodeBook.begin<CodeBook<RGBf>::BlockMaxDim>(); cbIt != currentCodeBook.end<CodeBook<RGBf>::BlockMaxDim>(); ++cbIt)
    {
        encodeBlock(currentCodeBook, previousCodeBook, *cbIt, state, maxBlockError);
    }
    // print statistics
    const auto nrOfMinBlocks = width / CodeBook<RGBf>::BlockMinDim * height / CodeBook<RGBf>::BlockMinDim;
    auto refPercentCurr = static_cast<float>((statistics.refBlocksCurr[0] * 16 + statistics.refBlocksCurr[1] * 4 + statistics.refBlocksCurr[2]) * 100) / nrOfMinBlocks;
    auto refPercentPrev = static_cast<float>((statistics.refBlocksPrev[0] * 16 + statistics.refBlocksPrev[1] * 4 + statistics.refBlocksPrev[2]) * 100) / nrOfMinBlocks;
    auto dxtPercent = static_cast<float>((statistics.dxtBlocks[0] * 16 + statistics.dxtBlocks[1] * 4 + statistics.dxtBlocks[2]) * 100) / nrOfMinBlocks;
    std::cout << "Curr (16/8/4): " << statistics.refBlocksCurr[0] << "/" << statistics.refBlocksCurr[1] << "/" << statistics.refBlocksCurr[2] << " " << std::fixed << std::setprecision(1) << refPercentCurr << "%";
    std::cout << ", Prev (16/8/4): " << statistics.refBlocksPrev[0] << "/" << statistics.refBlocksPrev[1] << "/" << statistics.refBlocksPrev[2] << " " << std::fixed << std::setprecision(1) << refPercentPrev << "%";
    std::cout << ", DXT: " << statistics.dxtBlocks[0] << "/" << statistics.dxtBlocks[1] << "/" << statistics.dxtBlocks[2] << " " << std::fixed << std::setprecision(1) << dxtPercent << "%" << std::endl;
    //  add frame header to compressedData
    std::vector<uint8_t> compressedData;
    FrameHeader frameHeader;
    frameHeader.flags = keyFrame ? 0 : FRAME_IS_PFRAME;
    frameHeader.nrOfFlags = static_cast<uint16_t>(state.flags.size());
    auto headerData = frameHeader.toArray();
    assert((headerData.size() % 4) == 0);
    std::copy(headerData.cbegin(), headerData.cend(), std::back_inserter(compressedData));
    // expand block flags to multiple of 32 bits
    while ((state.flags.size() % 32) != 0)
    {
        state.flags.push_back(false);
    }
    // convert bits to a little-endian 16-Bit value and copy to compressedData
    uint32_t bitCount = 0;
    uint16_t flags16 = 0;
    for (auto fIt = state.flags.cbegin(); fIt != state.flags.cend(); ++fIt)
    {
        // swap bits around so we end up with the last bit in the highest place
        flags16 = (flags16 >> 1) | (*fIt ? (1 << 15) : 0);
        if (++bitCount >= 16)
        {
            // store little-endian
            compressedData.push_back(static_cast<uint8_t>(flags16 & 0xFF));
            compressedData.push_back(static_cast<uint8_t>((flags16 >> 8) & 0xFF));
            bitCount = 0;
            flags16 = 0;
        }
    }
    // copy DXT blocks to compressedData
    std::copy(state.data.cbegin(), state.data.cend(), std::back_inserter(compressedData));
    compressedData = DataHelpers::fillUpToMultipleOf(compressedData, 4);
    assert((compressedData.size() % 4) == 0);
    // convert current frame / codebook back to store as decompressed frame
    return std::make_pair(compressedData, image);
}

auto DXTV::decodeDXTV(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<XRGB8888>
{
    return {};
}