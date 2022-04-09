#include "codec_dxtv.h"

#include "colorblock.h"
#include "datahelpers.h"
#include "dxtblock.h"
#include "exception.h"
#include "linefit.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <array>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include <iostream>

using namespace Color;

struct FrameHeader
{
    uint16_t flags = 0;         // e.g. FRAME_IS_PFRAME
    uint16_t nrOfBlocks = 0;    // # of blocks or any size in frame (determines the size of flags block)
    uint16_t nrOfRefBlocks = 0; // # of reference blocks in frame. rest is verbatim blocks (determines the size of references block)
    uint16_t dummy = 0;

    std::array<uint8_t, 8> toArray() const
    {
        std::array<uint8_t, 8> result;
        *reinterpret_cast<uint16_t *>(&result[0]) = flags;
        *reinterpret_cast<uint16_t *>(&result[2]) = nrOfBlocks;
        *reinterpret_cast<uint16_t *>(&result[4]) = nrOfRefBlocks;
        *reinterpret_cast<uint16_t *>(&result[6]) = dummy;
        return result;
    }
};

// The image is split into 16x4 pixel blocks which can be futher split into 8x4 and 4x4 blocks.
//
// Every 16x4 (block size 0) block has two flags:
// Bit 0: Verbatim DXT block (0) or reference to other block (1)
// Bit 1: Block handled entirely (0) or block split into 8x8 (1)
//
// Every 8x4 block (block size 1) has two flags:
// Bit 0: Verbatim DXT block (0) or reference to other block (1)
// Bit 1: Block handled entirely (0) or block split into 4x4 (1)
//
// Every 4x4 block (block size 2) has two flags:
// Bit 0: Verbatim DXT block (0) or reference to other block (1)
// Bit 1: unused
//
// Block flags are sent depth first. So if a 16x4 block is split into 2 8x4 children AB, its first 8x4 child A is sent first.
// If that child is split, its 2 4x4 children CD are sent first, then child B and so on. This makes sure no flag bits are wasted.
//
// References for 16x4 blocks are stored as 1 bit current (0) / previous (1) frame + 7 Bit index.
//     Range [-128,-1] is used for references to the current frame.
//     Range [-63,64] is used for references to the previous frame.
// References for 8x4 blocks are stored as 1 bit current (0) / previous (1) frame + 7 Bit index.
//     Range [-128,-1] is used for references to the current frame.
//     Range [-63,64] is used for references to the previous frame.
// References for 4x4 blocks are stored as 1 bit current (0) / previous (1) frame + 7 Bit index.
//     Range [-128,-1] is used for references to the current frame.
//     Range [-63,64] is used for references to the previous frame.

constexpr uint16_t FRAME_IS_PFRAME = 0x80; // 0 for B-frames / key frames, 1 for P-frame / inter-frame compression ("predicted frame")
constexpr uint16_t FRAME_KEEP = 0x40;      // 1 for frames that are considered a direct copy of the previous frame and can be kept

constexpr bool BLOCK_IS_FULL = false;     // The block is a full block
constexpr bool BLOCK_IS_SPLIT = true;     // The block is split into smaller sub-blocks
constexpr bool BLOCK_IS_DXT = false;      // The block is a verbatim DXT block
constexpr bool BLOCK_IS_REF = true;       // The block is a reference into the current or previous frame
constexpr uint8_t BLOCK_FROM_CURR = 0;    // The block is from from the current frame
constexpr uint8_t BLOCK_FROM_PREV = 0x80; // The block is from from the previous frame

constexpr std::pair<int32_t, int32_t> CurrRefOffset = {-128, -1}; // Block search offsets for current frame for 16, 8, 4
constexpr std::pair<int32_t, int32_t> PrevRefOffset = {-63, 64};  // Block search offsets for previous frame for 16, 8, 4

/// @brief Calculate perceived pixel difference between blocks
template <std::size_t W, std::size_t H>
static auto distance(const BlockView<YCgCoRd, W, H> &a, const BlockView<YCgCoRd, W, H> &b) -> double
{
    double dist = 0.0;
    for (auto aIt = a.cbegin(), bIt = b.cbegin(); aIt != a.cend() && bIt != b.cend(); ++aIt, ++bIt)
    {
        dist += YCgCoRd::distance(*aIt, *bIt);
    }
    return dist / (W * H);
}

/// @brief Calculate perceived pixel difference between blocks
template <std::size_t W, std::size_t H>
static auto distanceBelowThreshold(const BlockView<YCgCoRd, W, H> &a, const BlockView<YCgCoRd, W, H> &b, double threshold) -> std::pair<bool, double>
{
    bool belowThreshold = true;
    double dist = 0.0;
    for (auto aIt = a.cbegin(), bIt = b.cbegin(); aIt != a.cend() && bIt != b.cend(); ++aIt, ++bIt)
    {
        auto colorDist = YCgCoRd::distance(*aIt, *bIt);
        belowThreshold = belowThreshold ? colorDist < threshold : belowThreshold;
        dist += colorDist;
    }
    return {belowThreshold, dist / (W * H)};
}

/// @brief List of code book entries representing the image
class CodeBook
{
public:
    using value_type = YCgCoRd;
    using state_type = bool;
    static constexpr std::size_t BlockMaxWidth = 16;
    static constexpr std::size_t BlockMinWidth = 4;
    static constexpr std::size_t BlockHeight = 4;
    static constexpr std::size_t BlockLevels = std::log2(BlockMaxWidth) - std::log2(BlockMinWidth);
    using block_type0 = BlockView<YCgCoRd, BlockMaxWidth, BlockHeight, BlockMinWidth>;
    using block_type1 = BlockView<YCgCoRd, BlockMaxWidth / 2, BlockHeight, BlockMinWidth>;
    using block_type2 = BlockView<YCgCoRd, BlockMaxWidth / 4, BlockHeight, BlockMinWidth>;

    CodeBook() = default;

    /// @brief Construct a codebook from image data
    CodeBook(const std::vector<uint16_t> &image, uint32_t width, uint32_t height)
        : m_width(width), m_height(height)
    {
        std::transform(image.cbegin(), image.cend(), std::back_inserter(m_colors), [](const auto &pixel)
                       { return YCgCoRd::fromRGB555(pixel); });
        for (uint32_t y = 0; y < m_height; y += BlockHeight)
        {
            for (uint32_t x = 0; x < m_width; x += BlockMaxWidth)
            {
                m_blocks0.emplace_back(block_type0(m_colors.data(), m_width, m_height, x, y));
            }
            for (uint32_t x = 0; x < m_width; x += BlockMaxWidth / 2)
            {
                m_blocks1.emplace_back(block_type1(m_colors.data(), m_width, m_height, x, y));
            }
            for (uint32_t x = 0; x < m_width; x += BlockMaxWidth / 4)
            {
                m_blocks2.emplace_back(block_type2(m_colors.data(), m_width, m_height, x, y));
            }
        }
    }

    /// @brief Block iterator to start of blocks
    template <std::size_t BLOCK_LEVEL>
    auto begin() noexcept
    {
        if constexpr (BLOCK_LEVEL == 0)
        {
            return m_blocks0.begin();
        }
        else if constexpr (BLOCK_LEVEL == 1)
        {
            return m_blocks1.begin();
        }
        else
        {
            return m_blocks2.begin();
        }
    }

    /// @brief Block iterator past end of blocks
    template <std::size_t BLOCK_LEVEL>
    auto end() noexcept
    {
        if constexpr (BLOCK_LEVEL == 0)
        {
            return m_blocks0.end();
        }
        else if constexpr (BLOCK_LEVEL == 1)
        {
            return m_blocks1.end();
        }
        else
        {
            return m_blocks2.end();
        }
    }

    /// @brief Block iterator to start of blocks
    template <std::size_t BLOCK_LEVEL>
    auto cbegin() const noexcept
    {
        if constexpr (BLOCK_LEVEL == 0)
        {
            return m_blocks0.cbegin();
        }
        else if constexpr (BLOCK_LEVEL == 1)
        {
            return m_blocks1.cbegin();
        }
        else
        {
            return m_blocks2.cbegin();
        }
    }

    /// @brief Block iterator past end of blocks
    template <std::size_t BLOCK_LEVEL>
    auto cend() const noexcept
    {
        if constexpr (BLOCK_LEVEL == 0)
        {
            return m_blocks0.cend();
        }
        else if constexpr (BLOCK_LEVEL == 1)
        {
            return m_blocks1.cend();
        }
        else
        {
            return m_blocks2.cend();
        }
    }

    /// @brief Check if codebook has blocks
    template <std::size_t BLOCK_LEVEL>
    auto empty() const -> bool
    {
        if constexpr (BLOCK_LEVEL == 0)
        {
            return m_blocks0.empty();
        }
        else if constexpr (BLOCK_LEVEL == 1)
        {
            return m_blocks1.empty();
        }
        else
        {
            return m_blocks2.empty();
        }
    }

    /// @brief Get number codebook blocks at specific level
    template <std::size_t BLOCK_LEVEL>
    auto size() const
    {
        if constexpr (BLOCK_LEVEL == 0)
        {
            return m_blocks0.size();
        }
        else if constexpr (BLOCK_LEVEL == 1)
        {
            return m_blocks1.size();
        }
        else
        {
            return m_blocks2.size();
        }
    }

    /// @brief Convert a codebook to image data
    auto toImage() const -> std::vector<uint16_t>
    {
        std::vector<uint16_t> image;
        std::transform(m_colors.cbegin(), m_colors.cend(), std::back_inserter(image), [](const auto &color)
                       { return color.toRGB555(); });
        return image;
    }

    /// @brief Calculate perceived pixel difference between codebooks
    auto distance(const CodeBook &b) -> double
    {
        double sum = 0;
        auto aIt = m_colors.cbegin();
        auto bIt = b.m_colors.cbegin();
        while (aIt != m_colors.cend() && bIt != b.m_colors.cend())
        {
            sum += YCgCoRd::distance(*aIt++, *bIt++);
        }
        return sum / m_blocks2.size();
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::vector<YCgCoRd> m_colors;
    std::vector<block_type0> m_blocks0;
    std::vector<block_type1> m_blocks1;
    std::vector<block_type2> m_blocks2;
};

/// @brief Search for entry in codebook with minimum error
/// @return Returns (error, entry index) if usable entry found or empty optional, if not
template <std::size_t BLOCK_WIDTH>
auto findBestMatchingBlock(const CodeBook &codeBook, int32_t nrOfBlocks, const BlockView<CodeBook::value_type, BLOCK_WIDTH, CodeBook::BlockHeight> &block, double maxAllowedError, int32_t offsetMin, int32_t offsetMax) -> std::optional<std::pair<double, BlockView<CodeBook::value_type, BLOCK_WIDTH, CodeBook::BlockHeight>>>
{
    using return_type = std::pair<double, BlockView<CodeBook::value_type, BLOCK_WIDTH, CodeBook::BlockHeight>>;
    constexpr std::size_t BLOCK_LEVEL = log2(CodeBook::BlockMaxWidth) - log2(BLOCK_WIDTH);
    if (codeBook.empty<BLOCK_LEVEL>())
    {
        return std::optional<return_type>();
    }
    // calculate start and end of search
    int32_t minIndex = block.index() + offsetMin;
    minIndex = minIndex < 0 ? 0 : minIndex;
    minIndex = minIndex >= nrOfBlocks ? nrOfBlocks - 1 : minIndex;
    int32_t maxIndex = block.index() + offsetMax;
    maxIndex = maxIndex < 0 ? 0 : maxIndex;
    maxIndex = maxIndex >= nrOfBlocks ? nrOfBlocks - 1 : maxIndex;
    // searched entries must be >= 1
    if ((maxIndex - minIndex) < 1)
    {
        return std::optional<return_type>();
    }
    // calculate codebook errors
    std::vector<std::pair<double, int32_t>> errors(maxIndex - minIndex);
    auto start = std::next(codeBook.cbegin<BLOCK_LEVEL>(), minIndex);
    auto end = std::next(codeBook.cbegin<BLOCK_LEVEL>(), maxIndex);
    std::transform(start, end, errors.begin(), [block, index = minIndex, maxAllowedError](const auto &b) mutable
                   { auto dist = distanceBelowThreshold(block, b, maxAllowedError); return std::make_pair(dist.first ? dist.second : UINT_MAX, index++); });
    // find codebook that has minimum error
    auto bestErrorIt = std::min_element(errors.cbegin(), errors.cend());
    return (bestErrorIt->first < maxAllowedError) ? std::optional<return_type>({bestErrorIt->first, *std::next(codeBook.cbegin<BLOCK_LEVEL>(), bestErrorIt->second)}) : std::optional<return_type>();
}

/// @brief Store state of compression of one frame
struct CompressionState
{
    std::vector<bool> flags;        // block flags store flags for blocks (2 bit per block of any size)
    std::vector<uint8_t> refBlocks; // stores block references
    std::vector<uint8_t> dxtBlocks; // stores verbatim DXT blocks
    uint32_t minBlocksEncoded = 0;  // how many blocks of CodeBook::BlockMinWidth have been encoded yet
};

template <std::size_t BLOCK_WIDTH>
auto encodeBlock(CodeBook &currentCodeBook, const CodeBook &previousCodeBook, BlockView<CodeBook::value_type, BLOCK_WIDTH, CodeBook::BlockHeight> &block, CompressionState &state, double maxAllowedError) -> void
{
    constexpr std::size_t BLOCK_LEVEL = std::log2(CodeBook::BlockMaxWidth) - std::log2(BLOCK_WIDTH);
    // Try to reference block from the previous code book (if available) within error
    std::optional<std::pair<double, BlockView<CodeBook::value_type, BLOCK_WIDTH, CodeBook::BlockHeight>>> previousRef;
    if (!previousCodeBook.empty<BLOCK_LEVEL>())
    {
        previousRef = findBestMatchingBlock(previousCodeBook, previousCodeBook.size<BLOCK_LEVEL>(), block, maxAllowedError, PrevRefOffset.first, PrevRefOffset.second);
    }
    // Try to reference block from the current code book within error
    auto currentRef = findBestMatchingBlock(currentCodeBook, (state.minBlocksEncoded * CodeBook::BlockMinWidth) / BLOCK_WIDTH, block, maxAllowedError, CurrRefOffset.first, CurrRefOffset.second);
    // Choose the better one of both block references
    if (previousRef.has_value() && (!currentRef.has_value() || previousRef.value().first <= currentRef.value().first))
    {
        // store reference from previous frame
        state.flags.push_back(BLOCK_IS_REF);
        state.flags.push_back(BLOCK_IS_FULL);
        // get referenced block
        const auto &srcBlock = previousRef.value().second;
        // bring offset from [-63, 64] into range [0, 127]
        int32_t offset = (static_cast<int32_t>(block.index()) - static_cast<int32_t>(srcBlock.index())) - PrevRefOffset.first; // first is correct if range crosses 0
        REQUIRE(offset >= 0 && offset <= (PrevRefOffset.second - PrevRefOffset.first), std::runtime_error, "Previous frame block offset not in range");
        state.refBlocks.push_back(BLOCK_FROM_PREV | offset);
        state.minBlocksEncoded++;
        // store block colors in current frame
        block.copyColorsFrom(srcBlock);
    }
    else if (currentRef.has_value() && (!previousRef.has_value() || currentRef.value().first <= previousRef.value().first))
    {
        // store reference from current frame
        state.flags.push_back(BLOCK_IS_REF);
        state.flags.push_back(BLOCK_IS_FULL);
        // get referenced block
        const auto &srcBlock = currentRef.value().second;
        // bring offset from [1, 128] into range [0, 127]
        int32_t offset = (static_cast<int32_t>(block.index()) - static_cast<int32_t>(srcBlock.index())) + CurrRefOffset.second; // second is correct if range < 0
        REQUIRE(offset >= 0 && offset <= (CurrRefOffset.second - CurrRefOffset.first), std::runtime_error, "Current frame block offset not in range");
        state.refBlocks.push_back(BLOCK_FROM_CURR | offset);
        state.minBlocksEncoded++;
        // store block colors in current frame
        block.copyColorsFrom(srcBlock);
    }
    else
    {
        // No good references found. DXT-encode full block
        auto rawBlock = block.colors();
        auto encodedBlock = DXTBlock<BLOCK_WIDTH, CodeBook::BlockHeight>::encode(rawBlock);
        // check if encoded block is below allowed error or we need to store it due to maximum split level
        if (auto decodedBlock = DXTBlock<BLOCK_WIDTH, CodeBook::BlockHeight>::decode(encodedBlock); BLOCK_WIDTH <= CodeBook::BlockMinWidth || YCgCoRd::distanceBelowThreshold(rawBlock, decodedBlock, maxAllowedError).first)
        {
            // Threshold ok or we can't split. Store full DXT block
            state.flags.push_back(BLOCK_IS_DXT);
            state.flags.push_back(BLOCK_IS_FULL);
            auto encodedData = encodedBlock.toArray();
            std::copy(encodedData.cbegin(), encodedData.cend(), std::back_inserter(state.dxtBlocks));
            state.minBlocksEncoded++;
            // store decoded block colors in current frame
            block.copyColorsFrom(decodedBlock);
        }
        else if constexpr (BLOCK_WIDTH > CodeBook::BlockMinWidth)
        {
            // Split block and recurse
            encodeBlock(currentCodeBook, previousCodeBook, block.block(0), state, maxAllowedError);
            encodeBlock(currentCodeBook, previousCodeBook, block.block(1), state, maxAllowedError);
        }
    }
}

#ifdef STATISTICS
int32_t currentRefBlock = 0;
int32_t previousRefPos = 0;
int32_t previousRefNeg = 0;
int32_t keepBlock = 0;
int32_t repeatBlock = 0;
int32_t matchBlock = 0;
#endif
int32_t duplicateFrames = 0;
int32_t keyFrames = 0;

auto DXTV::encodeDXTV(const std::vector<uint16_t> &image, const std::vector<uint16_t> &previousImage, uint32_t width, uint32_t height, bool keyFrame, double maxBlockError) -> std::pair<std::vector<uint8_t>, std::vector<uint16_t>>
{
    static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
    REQUIRE(width % CodeBook::BlockMaxWidth == 0, std::runtime_error, "Image width must be a multiple of 16 for DXTV compression");
    REQUIRE(height % CodeBook::BlockMaxWidth == 0, std::runtime_error, "Image height must be a multiple of 16 for DXTV compression");
    REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "Max. block error must be in [0.01,1]");
    // divide max block error to get into internal range
    maxBlockError /= 1000;
    // convert frames to codebooks
    auto currentCodeBook = CodeBook(image, width, height);
    const CodeBook previousCodeBook = previousImage.empty() || keyFrame ? CodeBook() : CodeBook(previousImage, width, height);
    // calculate perceived frame distance
    const double frameDistance = previousCodeBook.empty<0>() ? INT_MAX : currentCodeBook.distance(previousCodeBook);
    // check if the new frame can be considered a verbatim copy
    if (!keyFrame && frameDistance < 0.001)
    {
        std::cout << "Duplicate frame " << duplicateFrames++ << std::endl;
        // frame is a duplicate. pass header only
        FrameHeader frameHeader;
        frameHeader.flags = FRAME_KEEP;
        std::vector<uint8_t> compressedData;
        auto headerData = frameHeader.toArray();
        assert((headerData.size() % 4) == 0);
        std::copy(headerData.cbegin(), headerData.cend(), std::back_inserter(compressedData));
        return {compressedData, previousImage};
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
#ifdef STATISTICS
    currentRefBlock = 0;
    previousRefPos = 0;
    previousRefNeg = 0;
    keepBlock = 0;
    repeatBlock = 0;
    matchBlock = 0;
#endif
    // loop through source images blocks
    for (auto cbIt = currentCodeBook.begin<0>(); cbIt != currentCodeBook.end<0>(); ++cbIt)
    {
        encodeBlock(currentCodeBook, previousCodeBook, *cbIt, state, maxBlockError);
    }
    // std::cout << "Curr: " << currentRefBlock << ", match: " << matchBlock << ", prev -/+: " << previousRefNeg << "/" << previousRefPos << ", repeat: " << repeatBlock << ", keep: " << keepBlock << std::endl;
    //  add frame header to compressedData
    std::vector<uint8_t> compressedData;
    FrameHeader frameHeader;
    frameHeader.flags = keyFrame ? 0 : FRAME_IS_PFRAME;
    frameHeader.nrOfBlocks = static_cast<uint16_t>(state.flags.size() / 2);
    frameHeader.nrOfRefBlocks = static_cast<uint16_t>(state.refBlocks.size());
    auto headerData = frameHeader.toArray();
    assert((headerData.size() % 4) == 0);
    std::copy(headerData.cbegin(), headerData.cend(), std::back_inserter(compressedData));
    // expand block flags to multiple of 32 bits
    while ((state.flags.size() % 32) != 0)
    {
        state.flags.push_back(false);
    }
    // convert bits to a little-endian 32-Bit value and copy to compressedData
    uint32_t bitCount = 0;
    uint32_t flags32 = 0;
    for (auto fIt = state.flags.cbegin(); fIt != state.flags.cend(); ++fIt)
    {
        // swap bits around so we end up with the last bit in the highest place
        flags32 = (flags32 >> 1) | (*fIt ? (1 << 31) : 0);
        if (++bitCount >= 32)
        {
            // store little-endian
            compressedData.push_back(static_cast<uint8_t>((flags32 >> 24) & 0xFF));
            compressedData.push_back(static_cast<uint8_t>((flags32 >> 16) & 0xFF));
            compressedData.push_back(static_cast<uint8_t>((flags32 >> 8) & 0xFF));
            compressedData.push_back(static_cast<uint8_t>(flags32 & 0xFF));
            bitCount = 0;
            flags32 = 0;
        }
    }
    // if we have reference blocks, expand to multiple of 4 and copy to compressedData
    if (!state.refBlocks.empty())
    {
        state.refBlocks = fillUpToMultipleOf(state.refBlocks, 4);
        assert((state.refBlocks.size() % 4) == 0);
        std::copy(state.refBlocks.cbegin(), state.refBlocks.cend(), std::back_inserter(compressedData));
    }
    // copy DXT blocks to compressedData
    std::copy(state.dxtBlocks.cbegin(), state.dxtBlocks.cend(), std::back_inserter(compressedData));
    assert((compressedData.size() % 4) == 0);
    // convert current frame / codebook back to store as decompressed frame
    return {compressedData, currentCodeBook.toImage()};
}

auto DXTV::decodeDXTV(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint16_t>
{
    return {};
}