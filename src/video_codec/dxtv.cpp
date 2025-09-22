#include "dxtv.h"

#include "blockview.h"
#include "codebook.h"
#include "color/conversions.h"
#include "color/psnr.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "image_codec/dxt.h"
#include "exception.h"
#include "math/linefit.h"
#include "if/dxtv_structs.h"
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

namespace Video
{

    /// @brief Search for entry in codebook with minimum error
    /// @return Returns (error, x offset, y offset) if usable entry found or empty optional, if not
    template <std::size_t BLOCK_DIM>
    auto findBestMatchingBlockMotion(const Dxtv::CodeBook8x8 &codeBook, const BlockView<XRGB8888, bool, BLOCK_DIM> &block, float allowedError, bool fromCurrCodeBook) -> std::optional<std::tuple<float, int32_t, int32_t>>
    {
        using return_type = std::tuple<float, int32_t, int32_t>;
        if (codeBook.empty())
        {
            return std::optional<return_type>();
        }
        const auto offsetH = fromCurrCodeBook ? Dxtv::CurrMotionHOffset : Dxtv::PrevMotionHOffset;
        const auto offsetV = fromCurrCodeBook ? Dxtv::CurrMotionVOffset : Dxtv::PrevMotionVOffset;
        // calculate start and end of motion search
        const int32_t blockX = block.x();
        const int32_t blockY = block.y();
        const int32_t xMax = codeBook.width() - static_cast<int32_t>(BLOCK_DIM);
        const int32_t yMax = codeBook.height() - static_cast<int32_t>(BLOCK_DIM);
        // clamp search range to frame
        const int32_t xStart = (blockX + offsetH.first) < 0 ? 0 : (blockX + offsetH.first);
        const int32_t xEnd = (blockX + offsetH.second) > xMax ? xMax : (blockX + offsetH.second);
        const int32_t yStart = (blockY + offsetV.first) < 0 ? 0 : (blockY + offsetV.first);
        const int32_t yEnd = (blockY + offsetV.second) > yMax ? yMax : (blockY + offsetV.second);
        // if we're searching in the current codebook, do not allow searching past the already decoded macro-block
        const int32_t yMacroBlock = blockY - (blockY % DxtvConstants::BLOCK_MAX_DIM);
        const int32_t vEnd = fromCurrCodeBook && yEnd > (yMacroBlock + DxtvConstants::BLOCK_MAX_DIM - BLOCK_DIM) ? (yMacroBlock + DxtvConstants::BLOCK_MAX_DIM - BLOCK_DIM) : yEnd;
        // search similar blocks
        const auto blockPixels = block.pixels();
        return_type bestMotion = {std::numeric_limits<float>::max(), 0, 0};
        for (int32_t y = yStart; y <= vEnd; ++y)
        {
            // if we're searching in the current codebook, do not search past the last decoded macro-block
            auto hEnd = xEnd;
            if (fromCurrCodeBook && (y + static_cast<int32_t>(BLOCK_DIM)) > yMacroBlock)
            {
                // make sure we're not searching in blocks not encoded yet
                hEnd = blockX - static_cast<int32_t>(BLOCK_DIM);
            }
            for (int32_t x = xStart; x <= hEnd; ++x)
            {
                auto error = codeBook.mse<BLOCK_DIM>(blockPixels, x, y);
                if (error < allowedError && error < std::get<0>(bestMotion))
                {
                    bestMotion = {error, x - blockX, y - blockY};
                }
            }
        }
        return std::get<0>(bestMotion) < allowedError ? bestMotion : std::optional<return_type>();
    }

    template <std::size_t BLOCK_DIM>
    auto encodeBlockInternal(Dxtv::CodeBook8x8 &currentCodeBook, const Dxtv::CodeBook8x8 &previousCodeBook, BlockView<XRGB8888, bool, BLOCK_DIM> &block, float quality, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<bool, std::vector<uint8_t>>
    {
        static_assert(DxtvConstants::BLOCK_MAX_DIM >= BLOCK_DIM);
        static constexpr std::size_t BLOCK_LEVEL = std::log2(DxtvConstants::BLOCK_MAX_DIM) - std::log2(BLOCK_DIM);
        bool blockWasSplit = DxtvConstants::BLOCK_NO_SPLIT;
        std::vector<uint8_t> data;
        // calculate allowed MSE for blocks. Map from [0, 100] to [1, 0]
        const float allowedError = std::pow((100.0F - quality) / 100.0F, 2.0F);
        // Try to find x/y motion block within error from previous frame
        auto prevRef = findBestMatchingBlockMotion(previousCodeBook, block, allowedError, false);
        // Try to find x/y motion block within error from current frame
        auto currRef = findBestMatchingBlockMotion(currentCodeBook, block, allowedError, true);
        // Choose the better one of both block references
        const bool prevRefIsBetter = prevRef.has_value() && (!currRef.has_value() || std::get<0>(prevRef.value()) <= std::get<0>(currRef.value()));
        const bool currRefIsBetter = currRef.has_value() && (!prevRef.has_value() || std::get<0>(currRef.value()) <= std::get<0>(prevRef.value()));
        if (prevRefIsBetter)
        {
            // check offset range
            auto offsetX = std::get<1>(prevRef.value());
            auto offsetY = std::get<2>(prevRef.value());
            REQUIRE(Dxtv::PrevMotionHOffset.first <= offsetX && offsetX <= Dxtv::PrevMotionHOffset.second, std::runtime_error, "Reference block x offset out of range for previous frame");
            REQUIRE(Dxtv::PrevMotionVOffset.first <= offsetY && offsetY <= Dxtv::PrevMotionVOffset.second, std::runtime_error, "Reference block y offset out of range for previous frame");
            REQUIRE(static_cast<int32_t>(block.x()) + offsetX >= 0 && static_cast<int32_t>(block.y()) + offsetY >= 0, std::runtime_error, "Reference block coordinates out of bounds");
            REQUIRE(static_cast<int32_t>(block.x()) + offsetX + BLOCK_DIM <= previousCodeBook.width() && static_cast<int32_t>(block.y()) + offsetY + BLOCK_DIM <= previousCodeBook.height(), std::runtime_error, "Reference block coordinates out of bounds");
            block.copyPixelsFrom(previousCodeBook.blockPixels<BLOCK_DIM>(static_cast<int32_t>(block.x()) + offsetX, static_cast<int32_t>(block.y()) + offsetY));
            // convert offsets to unsigned value
            offsetX += ((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1);
            offsetY += ((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1);
            // store reference to previous frame
            uint16_t refData = DxtvConstants::BLOCK_IS_REF;
            refData |= DxtvConstants::BLOCK_FROM_PREV;
            refData |= (offsetY & DxtvConstants::BLOCK_MOTION_MASK) << DxtvConstants::BLOCK_MOTION_Y_SHIFT;
            refData |= (offsetX & DxtvConstants::BLOCK_MOTION_MASK);
            data.push_back(refData & 0xFF);
            data.push_back((refData >> 8) & 0xFF);
            Statistics::incValue(statistics, "motionBlocksPrev", 1, BLOCK_LEVEL);
        }
        else if (currRefIsBetter)
        {
            // check offset range
            auto offsetX = std::get<1>(currRef.value());
            auto offsetY = std::get<2>(currRef.value());
            REQUIRE(Dxtv::CurrMotionHOffset.first <= offsetX && offsetX <= Dxtv::CurrMotionHOffset.second, std::runtime_error, "Reference block x offset out of range for current frame");
            REQUIRE(Dxtv::CurrMotionVOffset.first <= offsetY && offsetY <= Dxtv::CurrMotionVOffset.second, std::runtime_error, "Reference block y offset out of range for current frame");
            REQUIRE(static_cast<int32_t>(block.x()) + offsetX >= 0 && static_cast<int32_t>(block.y()) + offsetY >= 0, std::runtime_error, "Reference block coordinates out of bounds");
            REQUIRE(static_cast<int32_t>(block.x()) + offsetX + BLOCK_DIM <= currentCodeBook.width() && static_cast<int32_t>(block.y()) + offsetY + BLOCK_DIM <= currentCodeBook.height(), std::runtime_error, "Reference block coordinates out of bounds");
            block.copyPixelsFrom(currentCodeBook.blockPixels<BLOCK_DIM>(static_cast<int32_t>(block.x()) + offsetX, static_cast<int32_t>(block.y()) + offsetY));
            // convert offsets to unsigned value
            offsetX += ((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1);
            offsetY += ((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1);
            // store reference to current frame
            uint16_t refData = DxtvConstants::BLOCK_IS_REF;
            refData |= DxtvConstants::BLOCK_FROM_CURR;
            refData |= (offsetY & DxtvConstants::BLOCK_MOTION_MASK) << DxtvConstants::BLOCK_MOTION_Y_SHIFT;
            refData |= (offsetX & DxtvConstants::BLOCK_MOTION_MASK);
            data.push_back(refData & 0xFF);
            data.push_back((refData >> 8) & 0xFF);
            Statistics::incValue(statistics, "motionBlocksCurr", 1, BLOCK_LEVEL);
        }
        else
        {
            // No good references found. DXT-encode full block
            auto rawBlock = block.pixels();
            auto encodedBlock = DXT::encodeBlock<BLOCK_DIM>(rawBlock, false, swapToBGR);
            auto decodedBlock = DXT::decodeBlock<BLOCK_DIM>(encodedBlock, false, swapToBGR);
            if constexpr (BLOCK_DIM <= Dxtv::CodeBook8x8::BlockMinDim)
            {
                // We can't split anymore and can't get better error-wise. Store 4x4 DXT block
                data = encodedBlock;
                block.copyPixelsFrom(decodedBlock);
                Statistics::incValue(statistics, "dxtBlocks", 1, BLOCK_LEVEL);
            }
            else if constexpr (BLOCK_DIM > Dxtv::CodeBook8x8::BlockMinDim)
            {
                // We can still split. Check if encoded block is below allowed error or we want to split the block
                auto encodedBlockError = Color::mse(rawBlock, decodedBlock);
                if (encodedBlockError < allowedError)
                {
                    // Error ok. Store full DXT block
                    data = encodedBlock;
                    block.copyPixelsFrom(decodedBlock);
                    Statistics::incValue(statistics, "dxtBlocks", 1, BLOCK_LEVEL);
                }
                else
                {
                    // Split block to improve error and recurse
                    blockWasSplit = true;
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        auto [dummyFlag, blockData] = encodeBlockInternal<BLOCK_DIM / 2>(currentCodeBook, previousCodeBook, block.block(i), quality, swapToBGR, statistics);
                        std::copy(blockData.cbegin(), blockData.cend(), std::back_inserter(data));
                    }
                }
            }
        }
        block.data() = true; // mark block as encoded
        return {blockWasSplit, data};
    }

    template <>
    auto Dxtv::encodeBlock<4>(CodeBook8x8 &currentCodeBook, const CodeBook8x8 &previousCodeBook, BlockView<XRGB8888, bool, 4> &block, float quality, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<bool, std::vector<uint8_t>>
    {
        REQUIRE(block.size() == 16, std::runtime_error, "Number of pixels in block must be 16");
        return encodeBlockInternal<4>(currentCodeBook, previousCodeBook, block, quality, swapToBGR, statistics);
    }

    template <>
    auto Dxtv::encodeBlock<8>(CodeBook8x8 &currentCodeBook, const CodeBook8x8 &previousCodeBook, BlockView<XRGB8888, bool, 8> &block, float quality, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<bool, std::vector<uint8_t>>
    {
        REQUIRE(block.size() == 64, std::runtime_error, "Number of pixels in block must be 64");
        return encodeBlockInternal<8>(currentCodeBook, previousCodeBook, block, quality, swapToBGR, statistics);
    }

    auto Dxtv::encode(const std::vector<XRGB8888> &image, const std::vector<XRGB8888> &previousImage, uint32_t width, uint32_t height, float quality, const bool swapToBGR, Statistics::Frame::SPtr statistics) -> std::pair<std::vector<uint8_t>, std::vector<XRGB8888>>
    {
        REQUIRE(width % CodeBook8x8::BlockMaxDim == 0, std::runtime_error, "Image width must be a multiple of " << CodeBook8x8::BlockMaxDim << " for DXTV compression");
        REQUIRE(height % CodeBook8x8::BlockMaxDim == 0, std::runtime_error, "Image height must be a multiple of " << CodeBook8x8::BlockMaxDim << " for DXTV compression");
        REQUIRE(quality >= 0 && quality <= 100, std::runtime_error, "Max. block error must be in [0,100]");
        // convert frames to codebooks
        auto currentCodeBook = CodeBook8x8(image, width, height, false);
        const auto previousCodeBook = previousImage.empty() ? CodeBook8x8() : CodeBook8x8(previousImage, width, height, true);
        // calculate perceived frame distance
        const float frameError = previousCodeBook.empty() ? INT_MAX : currentCodeBook.mse(previousCodeBook);
        // check if the new frame can be considered a verbatim copy
        if (frameError < 0.0001)
        {
            // frame is a duplicate. pass header only
            DxtvFrameHeader frameHeader;
            frameHeader.frameFlags = DxtvConstants::FRAME_KEEP;
            frameHeader.uncompressedSize = width * height * 2;
            std::vector<uint8_t> compressedFrameData(sizeof(DxtvFrameHeader));
            DxtvFrameHeader::write(reinterpret_cast<uint32_t *>(compressedFrameData.data()), frameHeader);
            return std::make_pair(compressedFrameData, previousImage);
        }
        // round number of flag bits to full 16-bit units
        const auto blockFlagBitsPerLine = currentCodeBook.blockWidth() + (16 - currentCodeBook.blockWidth() % 16);
        const auto blockFlagBytesPerLine = blockFlagBitsPerLine / 8;
        // add frame header to compressed frame data
        DxtvFrameHeader frameHeader;
        frameHeader.uncompressedSize = width * height * 2;
        std::vector<uint8_t> compressedFrameData(sizeof(DxtvFrameHeader));
        DxtvFrameHeader::write(reinterpret_cast<uint32_t *>(compressedFrameData.data()), frameHeader);
        // build vector of one block result per line for parallel execution
        std::vector<std::vector<uint8_t>> compressedBlockData(currentCodeBook.blockHeight());
        // #pragma omp parallel for
        //  loop through source images in lines
        for (int by = 0; by < currentCodeBook.blockHeight(); ++by)
        {
            // reserve maximum compressed data size
            std::vector<uint8_t> &compressedLineData = compressedBlockData.at(by);
            compressedLineData.reserve(blockFlagBytesPerLine + currentCodeBook.blockWidth() * 32);
            // process in runs of 16 to correctly store flags in intervals
            for (std::size_t chunkIndex = 0; chunkIndex < (currentCodeBook.blockWidth() + 15) / 16; ++chunkIndex)
            {
                const auto blockStart = by * currentCodeBook.blockWidth() + chunkIndex * 16;
                // insert empty flag data into line data
                uint16_t flags16 = 0;
                const auto flagsIndex = compressedLineData.size();
                compressedLineData.push_back(0);
                compressedLineData.push_back(0);
                // compress 16 blocks
                const auto restBlockCount = std::min(currentCodeBook.blockWidth() - chunkIndex * 16, std::size_t(16));
                for (std::size_t bx = 0; bx < restBlockCount; ++bx)
                {
                    auto &block = currentCodeBook.block(blockStart + bx);
                    auto [blockSplitFlag, blockData] = encodeBlockInternal(currentCodeBook, previousCodeBook, block, quality, swapToBGR, statistics);
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
        return std::make_pair(compressedFrameData, currentCodeBook.pixels());
    }

    template <std::size_t BLOCK_DIM>
    auto decodeBlockInternal(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *
    {
        static_assert(DxtvConstants::BLOCK_MAX_DIM >= BLOCK_DIM);
        REQUIRE(data != nullptr, std::runtime_error, "Data can not be nullptr");
        REQUIRE(currBlock != nullptr, std::runtime_error, "currBlock can not be nullptr");
        REQUIRE(width > 0, std::runtime_error, "width must be > 0");
        auto dstPtr = currBlock;
        auto data0 = *data;
        if (data0 & DxtvConstants::BLOCK_IS_REF)
        {
            // decode block reference
            const bool fromPrev = data0 & DxtvConstants::BLOCK_FROM_PREV;
            const XRGB8888 *srcPtr = fromPrev ? prevBlock : currBlock;
            REQUIRE(srcPtr != nullptr, std::runtime_error, fromPrev ? "Previous image referenced, but empty" : "Current image referenced, but empty");
            // convert offsets to signed values
            auto offsetX = static_cast<int32_t>(data0 & DxtvConstants::BLOCK_MOTION_MASK) - ((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1);
            auto offsetY = static_cast<int32_t>((data0 >> DxtvConstants::BLOCK_MOTION_Y_SHIFT) & DxtvConstants::BLOCK_MOTION_MASK) - ((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1);
            // calculate start of block to copy
            srcPtr += offsetY * static_cast<int32_t>(width) + offsetX;
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
            return data + 1; // MC blocks use 2 bytes
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
                for (uint32_t x = 0; x < BLOCK_DIM; ++x)
                {
                    dstPtr[x] = *srcIt++;
                }
                dstPtr += width;
            }
            return data + 1 + 1 + (BLOCK_DIM * BLOCK_DIM / 8); // DXT blocks use 8 or 20 bytes
        }
    }

    template <>
    auto Dxtv::decodeBlock<4>(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *
    {
        return decodeBlockInternal<4>(data, currBlock, prevBlock, width, swapToBGR);
    }

    template <>
    auto Dxtv::decodeBlock<8>(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *
    {
        return decodeBlockInternal<8>(data, currBlock, prevBlock, width, swapToBGR);
    }

    auto Dxtv::decode(const std::vector<uint8_t> &data, const std::vector<XRGB8888> &previousImage, uint32_t width, uint32_t height, const bool swapToBGR) -> std::vector<XRGB8888>
    {
        REQUIRE(data.size() >= sizeof(DxtvFrameHeader), std::runtime_error, "Not enough data to decode");
        REQUIRE(width > 0, std::runtime_error, "width must be > 0");
        REQUIRE(height > 0, std::runtime_error, "height must be > 0");
        const DxtvFrameHeader frameHeader = DxtvFrameHeader::read(reinterpret_cast<const uint32_t *>(data.data()));
        if (frameHeader.frameFlags == DxtvConstants::FRAME_KEEP)
        {
            REQUIRE(previousImage.size() == width * height, std::runtime_error, "Frame should be repeated, but previous image is empty or has wrong size");
            return previousImage;
        }
        auto frameData = data.data() + sizeof(DxtvFrameHeader);
        auto dataPtr = reinterpret_cast<const uint16_t *>(frameData);
        std::vector<XRGB8888> image(width * height);
        for (uint32_t by = 0; by < height / DxtvConstants::BLOCK_MAX_DIM; ++by)
        {
            uint16_t flags = 0;
            uint32_t flagsAvailable = 0;
            auto currPtr = image.data() + by * width * DxtvConstants::BLOCK_MAX_DIM;
            auto prevPtr = previousImage.empty() ? nullptr : previousImage.data() + by * width * DxtvConstants::BLOCK_MAX_DIM;
            for (uint32_t bx = 0; bx < width / DxtvConstants::BLOCK_MAX_DIM; ++bx)
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

}
