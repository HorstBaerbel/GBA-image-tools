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

#include <algorithm>
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
    uint16_t frameFlags = 0; // General frame flags, e.g. FRAME_IS_PFRAME or FRAME_KEEP
    uint16_t dummy = 0;      // currently unused

    std::vector<uint8_t> toVector() const
    {
        std::vector<uint8_t> result(4);
        *reinterpret_cast<uint16_t *>(result.data() + 0) = frameFlags;
        *reinterpret_cast<uint16_t *>(result.data() + 2) = 0;
        return result;
    }

    static auto fromVector(const std::vector<uint8_t> &data) -> FrameHeader
    {
        REQUIRE(data.size() >= 4, std::runtime_error, "Data size must be >= 4");
        auto frameFlags = *reinterpret_cast<const uint16_t *>(data.data() + 0);
        return {frameFlags, 0};
    }
};

// The image is split into 8x8 pixel blocks which can be futher split into 4x4 blocks.
//
// Every 8x8 block (block size 0) has one flag:
// Bit 0: Block handled entirely (0) or block split into 4x4 (1)
// These bits are sent in the bitstream for each horizontal 8x8 line in intervals of 16 blocks
// A 240 pixel image stream will send:
// - 16 bits at the start of the bitstream
// - Another 16 bits after 16 encoded blocks (with 2 unused bits)
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
//   DXT blocks store verbatim DXT data (2 * uint16_t RGB555 colors and index data depending on blocks size)
//   So either 2 * 2 + 16 * 2 / 8 = 8 Bytes (4x4 block) or 2 * 2 + 64 * 2 / 8 = 20 Bytes (8x8 block)
//
// - If 1 it is a motion-compensated block with a size of 2 Bytes:
//   Bit 15: Always 1 (see above)
//   Bit 14: Block is reference to current (0) or previous (1) frame
//   Bit 13+12: Currently unused
//   Bit 11-6: y pixel motion of referenced block [-31,32] from top-left corner
//   Bit  5-0: x pixel motion of referenced block [-31,32] from top-left corner
//

/// @brief Search for entry in codebook with minimum error
/// @return Returns (error, x offset, y offset) if usable entry found or empty optional, if not
template <std::size_t BLOCK_DIM>
auto findBestMatchingBlockMotion(const DXTV::CodeBook8x8 &codeBook, const BlockView<XRGB8888, BLOCK_DIM> &block, float maxAllowedError, bool fromPrevCodeBook) -> std::optional<std::tuple<float, int32_t, int32_t>>
{
    using return_type = std::tuple<float, int32_t, int32_t>;
    if (codeBook.empty<BLOCK_DIM>())
    {
        return std::optional<return_type>();
    }
    const auto offsetH = fromPrevCodeBook ? DXTV::PrevMotionHOffset : DXTV::CurrMotionHOffset;
    const auto offsetV = fromPrevCodeBook ? DXTV::PrevMotionVOffset : DXTV::CurrMotionVOffset;
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
auto encodeBlockInternal(DXTV::CodeBook8x8 &currentCodeBook, const DXTV::CodeBook8x8 &previousCodeBook, BlockView<XRGB8888, BLOCK_DIM> &block, float maxAllowedError, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<bool, std::vector<uint8_t>>
{
    static_assert(DXTV::MAX_BLOCK_DIM >= BLOCK_DIM);
    static constexpr std::size_t BLOCK_LEVEL = std::log2(DXTV::MAX_BLOCK_DIM) - std::log2(BLOCK_DIM);
    bool blockWasSplit = DXTV::BLOCK_NO_SPLIT;
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
        REQUIRE(DXTV::PrevMotionHOffset.first <= offsetX && offsetX <= DXTV::PrevMotionHOffset.second, std::runtime_error, "Reference block x offset out of range for previous frame");
        REQUIRE(DXTV::PrevMotionVOffset.first <= offsetY && offsetY <= DXTV::PrevMotionVOffset.second, std::runtime_error, "Reference block y offset out of range for previous frame");
        uint16_t refData = DXTV::BLOCK_IS_REF;
        refData |= DXTV::BLOCK_FROM_PREV;
        // convert offsets to unsigned value
        offsetX += ((1 << DXTV::BLOCK_MOTION_BITS) / 2 - 1);
        offsetY += ((1 << DXTV::BLOCK_MOTION_BITS) / 2 - 1);
        refData |= (offsetY & DXTV::BLOCK_MOTION_MASK) << DXTV::BLOCK_MOTION_Y_SHIFT;
        refData |= (offsetX & DXTV::BLOCK_MOTION_MASK);
        // store reference to previous frame
        data.push_back(refData & 0xFF);
        data.push_back((refData >> 8) & 0xFF);
        currentCodeBook.setEncoded<BLOCK_DIM>(block);
        Statistics::incValue(statistics, "motionBlocksPrev", 1, BLOCK_LEVEL);
    }
    else if (currRefIsBetter)
    {
        // check offset range
        auto offsetX = std::get<1>(currRef.value());
        auto offsetY = std::get<2>(currRef.value());
        REQUIRE(DXTV::CurrMotionHOffset.first <= offsetX && offsetX <= DXTV::CurrMotionHOffset.second, std::runtime_error, "Reference block x offset out of range for current frame");
        REQUIRE(DXTV::CurrMotionVOffset.first <= offsetY && offsetY <= DXTV::CurrMotionVOffset.second, std::runtime_error, "Reference block y offset out of range for current frame");
        uint16_t refData = DXTV::BLOCK_IS_REF;
        refData |= DXTV::BLOCK_FROM_CURR;
        // convert offsets to unsigned value
        offsetX += ((1 << DXTV::BLOCK_MOTION_BITS) / 2 - 1);
        offsetY += ((1 << DXTV::BLOCK_MOTION_BITS) / 2 - 1);
        refData |= (offsetY & DXTV::BLOCK_MOTION_MASK) << DXTV::BLOCK_MOTION_Y_SHIFT;
        refData |= (offsetX & DXTV::BLOCK_MOTION_MASK);
        // store reference to current frame
        data.push_back(refData & 0xFF);
        data.push_back((refData >> 8) & 0xFF);
        currentCodeBook.setEncoded<BLOCK_DIM>(block);
        Statistics::incValue(statistics, "motionBlocksCurr", 1, BLOCK_LEVEL);
    }
    else
    {
        // No good references found. DXT-encode full block
        auto rawBlock = block.pixels();
        auto encodedBlock = DXT::encodeBlock<BLOCK_DIM>(rawBlock, false, swapToBGR);
        auto decodedBlock = DXT::decodeBlock<BLOCK_DIM>(encodedBlock, false, swapToBGR);
        if constexpr (BLOCK_DIM <= DXTV::CodeBook8x8::BlockMinDim)
        {
            // We can't split anymore. Store 4x4 DXT block
            data = encodedBlock;
            currentCodeBook.setEncoded<BLOCK_DIM>(block);
            block.copyPixelsFrom(decodedBlock);
            Statistics::incValue(statistics, "dxtBlocks", 1, BLOCK_LEVEL);
        }
        else if constexpr (BLOCK_DIM > DXTV::CodeBook8x8::BlockMinDim)
        {
            // We can still split. Check if encoded block is below allowed error or we want to split the block
            auto encodedBlockError = Color::mse(rawBlock, decodedBlock);
            if (encodedBlockError < maxAllowedError)
            {
                // Error ok. Store full DXT block
                data = encodedBlock;
                currentCodeBook.setEncoded<BLOCK_DIM>(block);
                block.copyPixelsFrom(decodedBlock);
                Statistics::incValue(statistics, "dxtBlocks", 1, BLOCK_LEVEL);
            }
            else
            {
                // Split block to improve error and recurse
                blockWasSplit = true;
                for (uint32_t i = 0; i < 4; ++i)
                {
                    auto [dummyFlag, blockData] = encodeBlockInternal<BLOCK_DIM / 2>(currentCodeBook, previousCodeBook, block.block(i), maxAllowedError, swapToBGR, statistics);
                    std::copy(blockData.cbegin(), blockData.cend(), std::back_inserter(data));
                }
            }
        }
    }
    return {blockWasSplit, data};
}

template <>
auto DXTV::encodeBlock<4>(CodeBook8x8 &currentCodeBook, const CodeBook8x8 &previousCodeBook, BlockView<XRGB8888, 4> &block, float maxAllowedError, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<bool, std::vector<uint8_t>>
{
    REQUIRE(block.size() == 16, std::runtime_error, "Number of pixels in block must be 16");
    return encodeBlockInternal<4>(currentCodeBook, previousCodeBook, block, maxAllowedError, swapToBGR, statistics);
}

template <>
auto DXTV::encodeBlock<8>(CodeBook8x8 &currentCodeBook, const CodeBook8x8 &previousCodeBook, BlockView<XRGB8888, 8> &block, float maxAllowedError, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<bool, std::vector<uint8_t>>
{
    REQUIRE(block.size() == 64, std::runtime_error, "Number of pixels in block must be 64");
    return encodeBlockInternal<8>(currentCodeBook, previousCodeBook, block, maxAllowedError, swapToBGR, statistics);
}

auto DXTV::encode(const std::vector<XRGB8888> &image, const std::vector<XRGB8888> &previousImage, uint32_t width, uint32_t height, bool keyFrame, float maxBlockError, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<std::vector<uint8_t>, std::vector<XRGB8888>>
{
    static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
    REQUIRE(width % CodeBook8x8::BlockMaxDim == 0, std::runtime_error, "Image width must be a multiple of " << CodeBook8x8::BlockMaxDim << " for DXTV compression");
    REQUIRE(height % CodeBook8x8::BlockMaxDim == 0, std::runtime_error, "Image height must be a multiple of " << CodeBook8x8::BlockMaxDim << " for DXTV compression");
    REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "Max. block error must be in [0.01,1]");
    // divide max block error to get into internal range
    maxBlockError /= 1000;
    // convert frames to codebooks
    auto currentCodeBook = CodeBook8x8(image, width, height, false);
    const auto previousCodeBook = previousImage.empty() || keyFrame ? CodeBook8x8() : CodeBook8x8(previousImage, width, height, true);
    // calculate perceived frame distance
    const float frameError = previousCodeBook.empty() ? INT_MAX : currentCodeBook.mse(previousCodeBook);
    // check if the new frame can be considered a verbatim copy
    if (!keyFrame && frameError < 0.001)
    {
        // frame is a duplicate. pass header only
        FrameHeader frameHeader;
        frameHeader.frameFlags = FRAME_KEEP;
        std::vector<uint8_t> compressedFrameData;
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
    // round number of flag bits to full 16-bit units
    const auto blockFlagBitsPerLine = currentCodeBook.blockWidth() + (16 - currentCodeBook.blockWidth() % 16);
    const auto blockFlagBytesPerLine = blockFlagBitsPerLine / 8;
    // add frame header to compressed frame data
    FrameHeader frameHeader;
    frameHeader.frameFlags = keyFrame ? 0 : FRAME_IS_PFRAME;
    auto headerData = frameHeader.toVector();
    assert((headerData.size() % 4) == 0);
    std::vector<uint8_t> compressedFrameData;
    compressedFrameData = headerData;
    // build vector of one block result per line for parallel execution
    std::vector<std::vector<uint8_t>> compressedBlockData(currentCodeBook.blockHeight());
#pragma omp parallel for
    // loop through source images in lines
    for (int by = 0; by < currentCodeBook.blockHeight(); ++by)
    {
        // reserve maximum compressed data size
        std::vector<uint8_t> &compressedLineData = compressedBlockData.at(by);
        compressedLineData.reserve(blockFlagBytesPerLine + currentCodeBook.blockWidth() * 32);
        // compress one BlockMaxDim line of blocks
        auto bIt = std::next(currentCodeBook.begin(), by * currentCodeBook.blockWidth());
        // process in blocks of 16 to correctly store flags in intervals
        for (std::size_t bi = 0; bi < (currentCodeBook.blockWidth() + 15) / 16; ++bi)
        {
            // insert empty flag data into line data
            uint16_t flags16 = 0;
            const auto flagsIndex = compressedLineData.size();
            compressedLineData.push_back(0);
            compressedLineData.push_back(0);
            // compress 16 blocks
            const auto restBlockCount = std::min(currentCodeBook.blockWidth() - bi * 16, std::size_t(16));
            for (std::size_t bx = 0; bx < restBlockCount; ++bx, ++bIt)
            {
                auto [blockSplitFlag, blockData] = encodeBlockInternal(currentCodeBook, previousCodeBook, *bIt, maxBlockError, swapToBGR, statistics);
                std::copy(blockData.cbegin(), blockData.cend(), std::back_inserter(compressedLineData));
                flags16 = (flags16 >> 1) | (blockSplitFlag ? 0x8000 : 0);
            }
            // shift flags to correct position when we compressed less than 16 blocks
            assert(restBlockCount <= 16);
            flags16 >>= 16 - restBlockCount;
            // store generated flags in compressed line data
            *reinterpret_cast<uint16_t *>(compressedLineData.data() + flagsIndex) = flags16;
        }
        // expand line data to multiple of 16 bit
        if ((compressedLineData.size() % 2) != 0)
        {
            compressedLineData.push_back(0);
        }
    }
    // find out how much memory we need
    const auto compressedBlockDataSize = std::accumulate(compressedBlockData.cbegin(), compressedBlockData.cend(), std::size_t(0), [](const auto &sum, const auto &data)
                                                         { return data.size() + sum; });
    compressedFrameData.reserve(compressedFrameData.size() + compressedBlockDataSize);
    // combine block line data from parallel execution
    for (const auto &compressedLineData : compressedBlockData)
    {
        std::copy(compressedLineData.cbegin(), compressedLineData.cend(), std::back_inserter(compressedFrameData));
    }
    // compressed data size should already be multiple of 2
    assert((compressedFrameData.size() % 2) == 0);
    // print statistics
    if (statistics != nullptr)
    {
        const auto nrOfMinBlocks = width / CodeBook8x8::BlockMinDim * height / CodeBook8x8::BlockMinDim;
        auto refPercentCurr = static_cast<float>((statistics->getValue("motionBlocksCurr", 0) * 4 + statistics->getValue("motionBlocksCurr", 1)) * 100) / nrOfMinBlocks;
        auto refPercentPrev = static_cast<float>((statistics->getValue("motionBlocksPrev", 0) * 4 + statistics->getValue("motionBlocksPrev", 1)) * 100) / nrOfMinBlocks;
        auto dxtPercent = static_cast<float>((statistics->getValue("dxtBlocks", 0) * 4 + statistics->getValue("dxtBlocks", 1)) * 100) / nrOfMinBlocks;
        std::cout << "Curr: " << statistics->getValue("motionBlocksCurr", 0) << "/" << statistics->getValue("motionBlocksCurr", 1) << " " << std::fixed << std::setprecision(1) << refPercentCurr << "%";
        std::cout << ", Prev: " << statistics->getValue("motionBlocksPrev", 0) << "/" << statistics->getValue("motionBlocksPrev", 1) << " " << std::fixed << std::setprecision(1) << refPercentPrev << "%";
        std::cout << ", DXT: " << statistics->getValue("dxtBlocks", 0) << "/" << statistics->getValue("dxtBlocks", 1) << " " << std::fixed << std::setprecision(1) << dxtPercent << "%" << std::endl;
    }
    // convert current frame / codebook back to store as decompressed frame
    return std::make_pair(compressedFrameData, image);
}

template <std::size_t BLOCK_DIM>
auto decodeBlockInternal(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *
{
    static_assert(DXTV::MAX_BLOCK_DIM >= BLOCK_DIM);
    REQUIRE(data != nullptr, std::runtime_error, "Data can not be nullptr");
    REQUIRE(currBlock != nullptr, std::runtime_error, "currBlock can not be nullptr");
    REQUIRE(width > 0, std::runtime_error, "width must be > 0");
    auto dstPtr = currBlock;
    auto data0 = *data;
    if (data0 & DXTV::BLOCK_IS_REF)
    {
        // decode block reference
        const bool fromPrev = data0 & DXTV::BLOCK_FROM_PREV;
        const XRGB8888 *srcPtr = fromPrev ? prevBlock : currBlock;
        REQUIRE(srcPtr != nullptr, std::runtime_error, fromPrev ? "Previous image referenced, but empty" : "Current image referenced, but empty");
        // convert offsets to signed values
        auto offsetX = static_cast<int32_t>(data0 & DXTV::BLOCK_MOTION_MASK) - ((1 << DXTV::BLOCK_MOTION_BITS) / 2 - 1);
        auto offsetY = static_cast<int32_t>((data0 >> DXTV::BLOCK_MOTION_Y_SHIFT) & DXTV::BLOCK_MOTION_MASK) - ((1 << DXTV::BLOCK_MOTION_BITS) / 2 - 1);
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

template <>
auto DXTV::decodeBlock<4>(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *
{
    return decodeBlockInternal<4>(data, currBlock, prevBlock, width, swapToBGR);
}

template <>
auto DXTV::decodeBlock<8>(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *
{
    return decodeBlockInternal<8>(data, currBlock, prevBlock, width, swapToBGR);
}

auto DXTV::decode(const std::vector<uint8_t> &data, const std::vector<XRGB8888> &previousImage, uint32_t width, uint32_t height, const bool swapToBGR) -> std::vector<XRGB8888>
{
    REQUIRE(data.size() > sizeof(FrameHeader), std::runtime_error, "Not enough data to decode");
    REQUIRE(width > 0, std::runtime_error, "width must be > 0");
    REQUIRE(height > 0, std::runtime_error, "height must be > 0");
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
    auto frameData = data.data() + sizeof(FrameHeader);
    auto dataPtr = reinterpret_cast<const uint16_t *>(frameData);
    std::vector<XRGB8888> image(width * height);
    for (uint32_t by = 0; by < height / MAX_BLOCK_DIM; ++by)
    {
        uint16_t flags = 0;
        uint32_t flagsAvailable = 0;
        auto currPtr = image.data() + by * width * MAX_BLOCK_DIM;
        auto prevPtr = previousImage.empty() ? nullptr : previousImage.data() + by * width * MAX_BLOCK_DIM;
        for (uint32_t bx = 0; bx < width / MAX_BLOCK_DIM; ++bx)
        {
            // read flags if we need to
            if (flagsAvailable < 1)
            {
                flags = *dataPtr++;
                flagsAvailable = 16;
            }
            // check if block is split
            if (flags & 1)
            {
                dataPtr = decodeBlockInternal<4>(dataPtr, currPtr, prevPtr, width, swapToBGR);                                 // A - upper-left
                dataPtr = decodeBlockInternal<4>(dataPtr, currPtr + 4, prevPtr + 4, width, swapToBGR);                         // B - upper-right
                dataPtr = decodeBlockInternal<4>(dataPtr, currPtr + 4 * width, prevPtr + 4 * width, width, swapToBGR);         // C - lower-left
                dataPtr = decodeBlockInternal<4>(dataPtr, currPtr + 4 * width + 4, prevPtr + 4 * width + 4, width, swapToBGR); // D - lower-right
            }
            else
            {
                dataPtr = decodeBlockInternal<8>(dataPtr, currPtr, prevPtr, width, swapToBGR);
            }
            currPtr += 8;
            prevPtr += 8;
            flags >>= 1;
            --flagsAvailable;
        }
    }
    return image;
}
