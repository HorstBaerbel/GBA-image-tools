#include "dxtv.h"

#include "blockview.h"
#include "codebook.h"
#include "color/conversions.h"
#include "color/psnr.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "dxt.h"
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
    uint16_t frameFlags = 0;            // General frame flags, e.g. FRAME_IS_PFRAME or FRAME_KEEP
    uint16_t blockFlagBytesPerLine = 0; // Flag bytes to read per 8x8 horizontal image line

    std::vector<uint8_t> toVector() const
    {
        std::vector<uint8_t> result(4);
        *reinterpret_cast<uint16_t *>(result.data() + 0) = frameFlags;
        *reinterpret_cast<uint16_t *>(result.data() + 2) = blockFlagBytesPerLine;
        return result;
    }

    static auto fromVector(const std::vector<uint8_t> &data) -> FrameHeader
    {
        REQUIRE(data.size() >= 4, std::runtime_error, "Data size must be >= 4");
        auto frameFlags = *reinterpret_cast<const uint16_t *>(data.data() + 0);
        auto blockFlagBytesPerLine = *reinterpret_cast<const uint16_t *>(data.data() + 2);
        return {frameFlags, blockFlagBytesPerLine};
    }
};

// The image is split into 8x8 pixel blocks which can be futher split into 4x4 blocks.
//
// Every 8x8 block (block size 0) has one flag:
// Bit 0: Block handled entirely (0) or block split into 4x4 (1)
// These bits are sent in the bitstream for each horizontal 8x8 line at the start of the
// data for each 8x8 line. Each group of bits is padded up to next full byte.
// A 240 pixel image stream will send 30 bits + 2 bits of padding = 32 bits at the start of each 8x8 line.
//
// A 4x4 block (block size 1) has no extra flags. If an 8x8 block has been split,
// 4 motion-compensated or 4 DXT blocks will be read from data.
//
// Blocks are sent row-wise. So if a 8x8 block is split into 4 4x4 children ABCD,
// its first 4x4 child A is sent first then child B and so on. The layout in the image is:
// A B
// C D
//
// 8x8 and 4x4 DXT and motion-compensated blocks differ in their highest Bit:
//
// - If 0 it is a DXT-block with a size of 8 or 20 Bytes:
//   DXT blocks store verbatim DXT data (2 * uint16_t RGB555 colors and index data depending on blocks size).
//   So either 2 * 2 + 16 * 2 / 8 = 8 Bytes (4x4 block) or 2 * 2 + 64 * 2 / 8 = 20 Bytes (8x8 block)
//
// - If 1 it is a motion-compensated block with a size of 2 Bytes:
//   Bit 15: Always 1 (see above)
//   Bit 14: Block is reference to current (0) or previous (1) frame
//   Bit 13+12: Currently unused
//   Bit 11-6: y pixel motion of referenced block [-31,32] from top-left corner
//   Bit  5-0: x pixel motion of referenced block [-31,32] from top-left corner
//

constexpr uint32_t MAX_BLOCK_DIM = 8; // Maximum block size is 8x8 pixels

constexpr uint16_t FRAME_IS_PFRAME = 0x80; // 0 for B-frames / key frames with only intra-frame compression, 1 for P-frames with inter-frame compression ("predicted frame")
constexpr uint16_t FRAME_KEEP = 0x40;      // 1 for frames that are considered a direct copy of the previous frame and can be kept

constexpr bool BLOCK_NO_SPLIT = false;             // The block is a full block
constexpr bool BLOCK_IS_SPLIT = true;              // The block is split into smaller sub-blocks
constexpr uint16_t BLOCK_IS_DXT = 0;               // The block is a verbatim DXT block
constexpr uint16_t BLOCK_IS_REF = (1 << 15);       // The block is a motion-compensated block from the current or previous frame
constexpr uint16_t BLOCK_FROM_CURR = (0 << 14);    // The reference block is from from the current frame
constexpr uint16_t BLOCK_FROM_PREV = (1 << 14);    // The reference block is from from the previous frame

constexpr uint32_t BLOCK_MOTION_BITS = 6;
constexpr uint16_t BLOCK_MOTION_MASK = ~((uint16_t(1) << BLOCK_MOTION_BITS) - 1); // Block x pixel motion mask
constexpr uint16_t BLOCK_MOTION_Y_SHIFT = BLOCK_MOTION_BITS;                      // Block y pixel motion shift

constexpr std::pair<int32_t, int32_t> CurrMotionHOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), (1 << BLOCK_MOTION_BITS) / 2}; // Block position search offsets for current frame for 8, 4
constexpr std::pair<int32_t, int32_t> CurrMotionVOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), 0};                            // Block position search offsets for current frame for 8, 4
constexpr std::pair<int32_t, int32_t> PrevMotionHOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), (1 << BLOCK_MOTION_BITS) / 2}; // Block position search offsets for previous frame for 8, 4
constexpr std::pair<int32_t, int32_t> PrevMotionVOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), (1 << BLOCK_MOTION_BITS) / 2}; // Block position search offsets for previous frame for 8, 4

using Codebook = CodeBook<XRGB8888, MAX_BLOCK_DIM>;

/// @brief Store statistic information for compression
struct Statistics
{
    std::array<uint32_t, Codebook::BlockLevels> motionBlocksCurr = {0, 0}; // statistics on how many motions block references came from the current frame
    std::array<uint32_t, Codebook::BlockLevels> motionBlocksPrev = {0, 0}; // statistics on how many motions block references came from the current frame
    std::array<uint32_t, Codebook::BlockLevels> dxtBlocks = {0, 0};        // statistics on how many DXT blocks were stored
};

/// @brief Search for entry in codebook with minimum error
/// @return Returns (error, x offset, y offset) if usable entry found or empty optional, if not
template <std::size_t BLOCK_DIM>
auto findBestMatchingBlockMotion(const Codebook &codeBook, const BlockView<XRGB8888, BLOCK_DIM> &block, float maxAllowedError, bool fromPrevCodeBook) -> std::optional<std::tuple<float, int32_t, int32_t>>
{
    using return_type = std::tuple<float, int32_t, int32_t>;
    if (codeBook.empty<BLOCK_DIM>())
    {
        return std::optional<return_type>();
    }
    const auto offsetH = fromPrevCodeBook ? PrevMotionHOffset : CurrMotionHOffset;
    const auto offsetV = fromPrevCodeBook ? PrevMotionVOffset : CurrMotionVOffset;
    // calculate start and end of motion search
    const int32_t frameWidth = fromPrevCodeBook ? codeBook.width() : block.x();
    const int32_t frameHeight = fromPrevCodeBook ? codeBook.height() : block.y();
    const int32_t blockX = block.x();
    const int32_t blockY = block.y();
    // clamp search range to frame
    const int32_t xStart = blockX + offsetH.first < 0 ? 0 : (blockX + offsetH.first > frameWidth - 1 ? frameWidth - 1 : blockX + offsetH.first);
    const int32_t xEnd = blockX + offsetH.second < 0 ? 0 : (blockX + offsetH.second > frameWidth - 1 - BLOCK_DIM ? frameWidth - 1 - BLOCK_DIM : blockX + offsetH.second);
    const int32_t yStart = blockY + offsetV.first < 0 ? 0 : (blockY + offsetV.first > frameHeight - 1 ? frameHeight - 1 : blockY + offsetV.first);
    const int32_t yEnd = blockY + offsetV.second < 0 ? 0 : (blockY + offsetV.second > frameHeight - 1 - BLOCK_DIM ? frameHeight - 1 - BLOCK_DIM : blockY + offsetV.second);
    // get frame and block pixel data
    auto &framePixels = codeBook.pixels();
    auto blockPixels = block.pixels();
    // search similar blocks
    return_type bestMotion = {std::numeric_limits<float>::max(), 0, 0};
    for (int32_t y = yStart; y <= yEnd; ++y)
    {
        // if we're searching in the current codebook, do not allow searching past the previously encoded block in this line
        auto hEnd = (!fromPrevCodeBook && y == blockY) ? block.x() - BLOCK_DIM : xEnd;
        if (!fromPrevCodeBook && block.x() == 0)
        {
            continue;
        }
        for (int32_t x = xStart; x <= hEnd; ++x)
        {
            auto error = mse(framePixels, codeBook.width(), blockPixels, x, y, BLOCK_DIM, BLOCK_DIM);
            if (error < maxAllowedError && error < std::get<0>(bestMotion))
            {
                bestMotion = {error, x - blockX, y - blockY};
            }
        }
    }
    return std::get<0>(bestMotion) < maxAllowedError ? bestMotion : std::optional<return_type>();
}

template <std::size_t BLOCK_DIM>
auto encodeBlock(Statistics &statistics, Codebook &currentCodeBook, const Codebook &previousCodeBook, BlockView<XRGB8888, BLOCK_DIM> &block, float maxAllowedError, const bool swapToBGR) -> std::pair<bool, std::vector<uint8_t>>
{
    static constexpr std::size_t BLOCK_LEVEL = std::log2(Codebook::BlockMaxDim) - std::log2(BLOCK_DIM);
    bool blockWasSplit = BLOCK_NO_SPLIT;
    std::vector<uint8_t> data;
    // Try to find x/y motion block within error from previous frame
    auto prevRef = findBestMatchingBlockMotion(previousCodeBook, block, maxAllowedError, true);
    // Try to find x/y motion block within error from current frame
    auto currRef = findBestMatchingBlockMotion(currentCodeBook, block, maxAllowedError, false);
    // Choose the better one of both block references
    const bool prevRefIsBetter = prevRef.has_value() && (!currRef.has_value() || std::get<0>(prevRef.value()) <= std::get<0>(currRef.value()));
    const bool currRefIsBetter = currRef.has_value() && (!prevRef.has_value() || std::get<0>(currRef.value()) <= std::get<0>(prevRef.value()));
    if (prevRefIsBetter)
    {
        // check offset range
        auto offsetX = std::get<1>(prevRef.value());
        auto offsetY = std::get<2>(prevRef.value());
        REQUIRE(PrevMotionHOffset.first <= offsetX && offsetX <= PrevMotionHOffset.second, std::runtime_error, "Reference block x offset out of range for previous frame");
        REQUIRE(PrevMotionVOffset.first <= offsetY && offsetY <= PrevMotionVOffset.second, std::runtime_error, "Reference block y offset out of range for previous frame");
        uint16_t refData = BLOCK_IS_REF;
        refData |= BLOCK_FROM_PREV;
        // convert offsets to unsigned value
        offsetX += ((1 << BLOCK_MOTION_BITS) / 2 - 1);
        offsetY += ((1 << BLOCK_MOTION_BITS) / 2 - 1);
        refData |= (offsetY & BLOCK_MOTION_MASK) << BLOCK_MOTION_Y_SHIFT;
        refData |= (offsetX & BLOCK_MOTION_MASK);
        // store reference to previous frame
        data.push_back(refData & 0xFF);
        data.push_back((refData >> 8) & 0xFF);
        currentCodeBook.setEncoded<BLOCK_DIM>(block);
        statistics.motionBlocksPrev[BLOCK_LEVEL]++;
    }
    else if (currRefIsBetter)
    {
        // check offset range
        auto offsetX = std::get<1>(currRef.value());
        auto offsetY = std::get<2>(currRef.value());
        REQUIRE(CurrMotionHOffset.first <= offsetX && offsetX <= CurrMotionHOffset.second, std::runtime_error, "Reference block x offset out of range for current frame");
        REQUIRE(CurrMotionVOffset.first <= offsetY && offsetY <= CurrMotionVOffset.second, std::runtime_error, "Reference block y offset out of range for current frame");
        uint16_t refData = BLOCK_IS_REF;
        refData |= BLOCK_FROM_CURR;
        // convert offsets to unsigned value
        offsetX += ((1 << BLOCK_MOTION_BITS) / 2 - 1);
        offsetY += ((1 << BLOCK_MOTION_BITS) / 2 - 1);
        refData |= (offsetY & BLOCK_MOTION_MASK) << BLOCK_MOTION_Y_SHIFT;
        refData |= (offsetX & BLOCK_MOTION_MASK);
        // store reference to current frame
        data.push_back(refData & 0xFF);
        data.push_back((refData >> 8) & 0xFF);
        currentCodeBook.setEncoded<BLOCK_DIM>(block);
        statistics.motionBlocksCurr[BLOCK_LEVEL]++;
    }
    else
    {
        // No good references found. DXT-encode full block
        auto rawBlock = block.pixels();
        auto encodedBlock = DXT::encodeBlock<BLOCK_DIM>(rawBlock, false, swapToBGR);
        auto decodedBlock = DXT::decodeBlock<BLOCK_DIM>(encodedBlock, false, swapToBGR);
        if constexpr (BLOCK_DIM <= Codebook::BlockMinDim)
        {
            // We can't split anymore. Store 4x4 DXT block
            data = encodedBlock;
            currentCodeBook.setEncoded<BLOCK_DIM>(block);
            block.copyPixelsFrom(decodedBlock);
            statistics.dxtBlocks[BLOCK_LEVEL]++;
        }
        else if constexpr (BLOCK_DIM > Codebook::BlockMinDim)
        {
            // We can still split. Check if encoded block is below allowed error or we want to split the block
            auto encodedBlockError = Color::mse(rawBlock, decodedBlock);
            if (encodedBlockError < maxAllowedError)
            {
                // Error ok. Store full DXT block
                data = encodedBlock;
                currentCodeBook.setEncoded<BLOCK_DIM>(block);
                block.copyPixelsFrom(decodedBlock);
                statistics.dxtBlocks[BLOCK_LEVEL]++;
            }
            else
            {
                // Split block to improve error and recurse
                blockWasSplit = true;
                for (uint32_t i = 0; i < 4; ++i)
                {
                    auto [dummyFlag, blockData] = encodeBlock(statistics, currentCodeBook, previousCodeBook, block.block(i), maxAllowedError, swapToBGR);
                    std::copy(blockData.cbegin(), blockData.cend(), std::back_inserter(data));
                }
            }
        }
    }
    return {blockWasSplit, data};
}

auto DXTV::encode(const std::vector<XRGB8888> &image, const std::vector<XRGB8888> &previousImage, uint32_t width, uint32_t height, bool keyFrame, float maxBlockError, const bool swapToBGR) -> std::pair<std::vector<uint8_t>, std::vector<XRGB8888>>
{
    static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
    REQUIRE(width % Codebook::BlockMaxDim == 0, std::runtime_error, "Image width must be a multiple of " << Codebook::BlockMaxDim << " for DXTV compression");
    REQUIRE(height % Codebook::BlockMaxDim == 0, std::runtime_error, "Image height must be a multiple of " << Codebook::BlockMaxDim << " for DXTV compression");
    REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "Max. block error must be in [0.01,1]");
    // divide max block error to get into internal range
    maxBlockError /= 1000;
    // convert frames to codebooks
    auto currentCodeBook = Codebook(image, width, height, false);
    const CodeBook previousCodeBook = previousImage.empty() || keyFrame ? Codebook() : Codebook(previousImage, width, height, true);
    // calculate perceived frame distance
    const float frameError = previousCodeBook.empty<Codebook::BlockMaxDim>() ? INT_MAX : currentCodeBook.mse(previousCodeBook);
    // check if the new frame can be considered a verbatim copy
    if (!keyFrame && frameError < 0.001)
    {
        // frame is a duplicate. pass header only
        FrameHeader frameHeader;
        frameHeader.frameFlags = FRAME_KEEP;
        std::vector<uint8_t> compressedData;
        auto headerData = frameHeader.toVector();
        assert((headerData.size() % 4) == 0);
        return std::make_pair(headerData, previousImage);
    }
    // if we don't have a keyframe, check for scene change
    /*if (!keyFrame)
    {
        if (frameError > 0.03)
        {
            std::cout << "Inserting key frame " << keyFrames++ << std::endl;
            keyFrame = true;
        }
    }*/
    Statistics statistics;
    // add frame header to compressedData
    std::vector<uint8_t> compressedData;
    FrameHeader frameHeader;
    frameHeader.frameFlags = keyFrame ? 0 : FRAME_IS_PFRAME;
    frameHeader.blockFlagBytesPerLine = (currentCodeBook.blockWidth<Codebook::BlockMaxDim>() + 7) / 8;
    auto headerData = frameHeader.toVector();
    assert((headerData.size() % 4) == 0);
    compressedData = headerData;
    // loop through source images in lines
    for (uint32_t by = 0; by < currentCodeBook.blockHeight<Codebook::BlockMaxDim>(); ++by)
    {
        // compress one BlockMaxDim line of blocks
        auto bIt = std::next(currentCodeBook.begin<Codebook::BlockMaxDim>(), by * currentCodeBook.blockWidth<Codebook::BlockMaxDim>());
        std::vector<uint8_t> lineData;
        std::vector<bool> flagData;
        Statistics blockStatistics;
        for (uint32_t bx = 0; bx < currentCodeBook.blockWidth<Codebook::BlockMaxDim>(); ++bx, ++bIt)
        {
            auto [blockSplitFlag, blockData] = encodeBlock(blockStatistics, currentCodeBook, previousCodeBook, *bIt, maxBlockError, swapToBGR);
            std::copy(blockData.cbegin(), blockData.cend(), std::back_inserter(lineData));
            flagData.push_back(blockSplitFlag);
        }
        // expand block flags to multiple of 16 bits
        while ((flagData.size() % 16) != 0)
        {
            flagData.push_back(false);
        }
        // convert bits to a little-endian 16-Bit value and copy to compressedData
        uint32_t bitCount = 0;
        uint16_t flags16 = 0;
        for (auto fIt = flagData.cbegin(); fIt != flagData.cend(); ++fIt)
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
        std::copy(lineData.cbegin(), lineData.cend(), std::back_inserter(compressedData));
        // expand line data to multiple of 16 bit
        if ((lineData.size() % 2) != 0)
        {
            lineData.push_back(0);
        }
        // update statistics
        for (uint32_t level = 0; level < Codebook::BlockLevels; ++level)
        {
            statistics.motionBlocksCurr[level] += blockStatistics.motionBlocksCurr[level];
            statistics.motionBlocksPrev[level] += blockStatistics.motionBlocksPrev[level];
            statistics.dxtBlocks[level] += blockStatistics.dxtBlocks[level];
        }
    }
    // fill up frame data to multiple of 2
    compressedData = DataHelpers::fillUpToMultipleOf(compressedData, 2);
    assert((compressedData.size() % 2) == 0);
    // print statistics
    const auto nrOfMinBlocks = width / Codebook::BlockMinDim * height / Codebook::BlockMinDim;
    auto refPercentCurr = static_cast<float>((statistics.motionBlocksCurr[0] * 4 + statistics.motionBlocksCurr[1]) * 100) / nrOfMinBlocks;
    auto refPercentPrev = static_cast<float>((statistics.motionBlocksPrev[0] * 4 + statistics.motionBlocksPrev[1]) * 100) / nrOfMinBlocks;
    auto dxtPercent = static_cast<float>((statistics.dxtBlocks[0] * 4 + statistics.dxtBlocks[1]) * 100) / nrOfMinBlocks;
    std::cout << "Curr: " << statistics.motionBlocksCurr[0] << "/" << statistics.motionBlocksCurr[1] << " " << std::fixed << std::setprecision(1) << refPercentCurr << "%";
    std::cout << ", Prev: " << statistics.motionBlocksPrev[0] << "/" << statistics.motionBlocksPrev[1] << " " << std::fixed << std::setprecision(1) << refPercentPrev << "%";
    std::cout << ", DXT: " << statistics.dxtBlocks[0] << "/" << statistics.dxtBlocks[1] << " " << std::fixed << std::setprecision(1) << dxtPercent << "%" << std::endl;
    // convert current frame / codebook back to store as decompressed frame
    return std::make_pair(compressedData, image);
}

template <std::size_t BLOCK_DIM>
auto decodeBlock(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *
{
    auto dstPtr = currBlock;
    auto data0 = *data;
    if (data0 & BLOCK_IS_REF)
    {
        // decode block reference
        const bool fromPrev = data0 & BLOCK_FROM_PREV;
        const XRGB8888 *srcPtr = fromPrev ? prevBlock : currBlock;
        REQUIRE(srcPtr != nullptr, std::runtime_error, fromPrev ? "Previous image referenced, but empty" : "Current image referenced, but empty");
        // convert offsets to signed values
        auto offsetX = static_cast<int32_t>(data0 & BLOCK_MOTION_MASK) - ((1 << BLOCK_MOTION_BITS) / 2 - 1);
        auto offsetY = static_cast<int32_t>((data0 >> BLOCK_MOTION_Y_SHIFT) & BLOCK_MOTION_MASK) - ((1 << BLOCK_MOTION_BITS) / 2 - 1);
        // calculate start of block to copy
        srcPtr += offsetY * width + offsetX;
        // copy pixels to output block
        for (uint32_t y = 0; y < BLOCK_DIM; ++y)
        {
            for (uint32_t x = 0; x < BLOCK_DIM; ++x)
            {
                dstPtr[x] = srcPtr[x];
            }
            srcPtr += width;
            dstPtr += width;
        }
        ++data;
    }
    else
    {
        // decode DXT block
        std::vector<uint8_t> compressed(2 * 2 + BLOCK_DIM * BLOCK_DIM * 2 / 8);
        std::memcpy(compressed.data(), data, compressed.size());
        auto decompressed = DXT::decodeBlock<BLOCK_DIM>(compressed, false, swapToBGR);
        // copy pixels to output block
        auto srcIt = decompressed.cbegin();
        for (uint32_t y = 0; y < BLOCK_DIM; ++y)
        {
            for (uint32_t x = 0; x < BLOCK_DIM; ++x, ++srcIt)
            {
                dstPtr[x] = *srcIt;
            }
            dstPtr += width;
        }
    }
    return data;
}

auto DXTV::decode(const std::vector<uint8_t> &data, const std::vector<XRGB8888> &previousImage, uint32_t width, uint32_t height, const bool swapToBGR) -> std::vector<XRGB8888>
{
    const auto frameHeader = FrameHeader::fromVector(data);
    if (frameHeader.frameFlags == FRAME_KEEP)
    {
        REQUIRE(previousImage.size() == width * height, std::runtime_error, "Frame should be repeated, but previous image is empty or has wrong size");
        return previousImage;
    }
    if (frameHeader.frameFlags == FRAME_IS_PFRAME)
    {
        REQUIRE(previousImage.size() == width * height, std::runtime_error, "Frame is P-frame, but previous image is empty or has wrong size");
    }
    REQUIRE(frameHeader.blockFlagBytesPerLine % 2 == 0, std::runtime_error, "Block flag bytes must be a multiple of 2");
    auto frameData = data.data() + sizeof(FrameHeader);
    std::vector<XRGB8888> image(width * height);
    for (uint32_t by = 0; by < height / MAX_BLOCK_DIM; ++by)
    {
        auto flagsPtr = reinterpret_cast<const uint16_t *>(frameData);
        uint16_t flags = 0;
        uint32_t flagsAvailable = 0;
        auto currPtr = image.data() + by * (width / MAX_BLOCK_DIM);
        auto prevPtr = previousImage.empty() ? nullptr : previousImage.data() + by * (width / MAX_BLOCK_DIM);
        auto dataPtr = reinterpret_cast<const uint16_t *>(frameData + frameHeader.blockFlagBytesPerLine);
        for (uint32_t bx = 0; bx < width / MAX_BLOCK_DIM; ++bx)
        {
            // read flags if we need to
            if (flagsAvailable < 1)
            {
                flags = *flagsPtr++;
                flagsAvailable = 16;
            }
            // check if block is split
            if (flags & 1)
            {
                dataPtr = decodeBlock<4>(dataPtr, currPtr, prevPtr, width, swapToBGR);
                dataPtr = decodeBlock<4>(dataPtr, currPtr + 4, prevPtr + 4, width, swapToBGR);
                dataPtr = decodeBlock<4>(dataPtr, currPtr + 4 * (width / MAX_BLOCK_DIM), prevPtr + 4 * (width / MAX_BLOCK_DIM), width, swapToBGR);
                dataPtr = decodeBlock<4>(dataPtr, currPtr + 4 * (width / MAX_BLOCK_DIM) + 4, prevPtr + 4 * (width / MAX_BLOCK_DIM) + 4, width, swapToBGR);
            }
            else
            {
                dataPtr = decodeBlock<8>(dataPtr, currPtr, prevPtr, width, swapToBGR);
            }
            flags >>= 1;
            --flagsAvailable;
        }
    }
    return image;
}
