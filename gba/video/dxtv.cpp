#include "dxtv.h"

#include "codec/dxt_tables.h"
#include "codec/dxtv_constants.h"
#include "dxtv_asm.h"
#include "image/dxt.h"
#include "memory/memory.h"
#include "print/output.h"

#define USE_ASM

namespace DXTV
{
    /// @brief Copy (un-)aligned block from src to curr
    template <uint32_t BLOCK_DIM>
    FORCEINLINE auto copyBlock(uint32_t *currPtr32, const uint16_t *srcPtr16, const uint32_t LineStride16);

    /// @brief Copy (un-)aligned block from src to curr
    template <>
    FORCEINLINE auto copyBlock<4>(uint32_t *currPtr32, const uint16_t *srcPtr16, const uint32_t LineStride16)
    {
        // check if block is word-aligned
        if (((uint32_t)srcPtr16 & 3) != 0)
        {
            // unaligned block
            auto currPtr16 = reinterpret_cast<uint16_t *>(currPtr32);
            currPtr16[0] = srcPtr16[0];
            currPtr16[1] = srcPtr16[1];
            currPtr16[2] = srcPtr16[2];
            currPtr16[3] = srcPtr16[3];
            srcPtr16 += LineStride16;
            currPtr16 += LineStride16;
            currPtr16[0] = srcPtr16[0];
            currPtr16[1] = srcPtr16[1];
            currPtr16[2] = srcPtr16[2];
            currPtr16[3] = srcPtr16[3];
            srcPtr16 += LineStride16;
            currPtr16 += LineStride16;
            currPtr16[0] = srcPtr16[0];
            currPtr16[1] = srcPtr16[1];
            currPtr16[2] = srcPtr16[2];
            currPtr16[3] = srcPtr16[3];
            srcPtr16 += LineStride16;
            currPtr16 += LineStride16;
            currPtr16[0] = srcPtr16[0];
            currPtr16[1] = srcPtr16[1];
            currPtr16[2] = srcPtr16[2];
            currPtr16[3] = srcPtr16[3];
        }
        else
        {
            // aligned block
            auto srcPtr32 = reinterpret_cast<const uint32_t *>(srcPtr16);
            currPtr32[0] = srcPtr32[0];
            currPtr32[1] = srcPtr32[1];
            srcPtr32 += LineStride16 / 2;
            currPtr32 += LineStride16 / 2;
            currPtr32[0] = srcPtr32[0];
            currPtr32[1] = srcPtr32[1];
            srcPtr32 += LineStride16 / 2;
            currPtr32 += LineStride16 / 2;
            currPtr32[0] = srcPtr32[0];
            currPtr32[1] = srcPtr32[1];
            srcPtr32 += LineStride16 / 2;
            currPtr32 += LineStride16 / 2;
            currPtr32[0] = srcPtr32[0];
            currPtr32[1] = srcPtr32[1];
        }
    }

    /// @brief Copy (un-)aligned block from src to curr
    template <>
    FORCEINLINE auto copyBlock<8>(uint32_t *currPtr32, const uint16_t *srcPtr16, const uint32_t LineStride16)
    {
        // check if block is word-aligned
        if (((uint32_t)srcPtr16 & 3) != 0)
        {
            // unaligned block
            auto currPtr16 = reinterpret_cast<uint16_t *>(currPtr32);
            for (uint32_t y = 0; y < 8; ++y)
            {
                currPtr16[0] = srcPtr16[0];
                currPtr16[1] = srcPtr16[1];
                currPtr16[2] = srcPtr16[2];
                currPtr16[3] = srcPtr16[3];
                currPtr16[4] = srcPtr16[4];
                currPtr16[5] = srcPtr16[5];
                currPtr16[6] = srcPtr16[6];
                currPtr16[7] = srcPtr16[7];
                srcPtr16 += LineStride16;
                currPtr16 += LineStride16;
            }
        }
        else
        {
            // aligned block
            auto srcPtr32 = reinterpret_cast<const uint32_t *>(srcPtr16);
            for (uint32_t i = 0; i < 8; ++i)
            {
                currPtr32[0] = srcPtr32[0];
                currPtr32[1] = srcPtr32[1];
                currPtr32[2] = srcPtr32[2];
                currPtr32[3] = srcPtr32[3];
                srcPtr32 += LineStride16 / 2;
                currPtr32 += LineStride16 / 2;
            }
        }
    }

    /// @brief Uncompress DXT or motion-compensated block
    /// @return Pointer past whole block data in src
    template <uint32_t BLOCK_DIM>
    FORCEINLINE auto decodeBlock(const uint16_t *dataPtr16, uint32_t *currPtr32, const uint32_t *prevPtr32, const uint32_t LineStride16) -> const uint16_t *;

    /// @brief Uncompress 4x4 or motion-compensated block
    /// @return Pointer past whole block data in src
    template <>
    FORCEINLINE auto decodeBlock<4>(const uint16_t *dataPtr16, uint32_t *currPtr32, const uint32_t *prevPtr32, const uint32_t LineStride16) -> const uint16_t *
    {
        auto currPtr16 = reinterpret_cast<uint16_t *>(currPtr32);
        if (*dataPtr16 & DXTV_CONSTANTS::BLOCK_IS_REF)
        {
            // get motion-compensated block info
            const uint16_t blockInfo = *dataPtr16++;
            const bool fromPrev = blockInfo & DXTV_CONSTANTS::BLOCK_FROM_PREV;
            auto srcPtr16 = reinterpret_cast<const uint16_t *>(fromPrev ? prevPtr32 : currPtr32);
            // convert offsets to signed values
            constexpr int32_t halfRange = (1 << DXTV_CONSTANTS::BLOCK_MOTION_BITS) / 2 - 1;
            const auto offsetX = static_cast<int32_t>(blockInfo & DXTV_CONSTANTS::BLOCK_MOTION_MASK) - halfRange;
            const auto offsetY = static_cast<int32_t>((blockInfo >> DXTV_CONSTANTS::BLOCK_MOTION_Y_SHIFT) & DXTV_CONSTANTS::BLOCK_MOTION_MASK) - halfRange;
            // calculate start of block to copy
            srcPtr16 += offsetY * LineStride16 + offsetX;
            // copy pixels to output block
            copyBlock<4>(currPtr32, srcPtr16, LineStride16);
        }
        else
        {
            // get DXT block colors
            dataPtr16 = DXT::getBlockColors(dataPtr16, DXT_BlockColors);
            // get pixel color indices and set pixels accordingly
            uint16_t indices = *dataPtr16++;
            // select color by 2 bit index from [c0, c1, c2, c3], then move to next line in destination vertically
            currPtr16[0] = DXT_BlockColors[(indices >> 0) & 0x3];
            currPtr16[1] = DXT_BlockColors[(indices >> 2) & 0x3];
            currPtr16[2] = DXT_BlockColors[(indices >> 4) & 0x3];
            currPtr16[3] = DXT_BlockColors[(indices >> 6) & 0x3];
            currPtr16 += LineStride16;
            currPtr16[0] = DXT_BlockColors[(indices >> 8) & 0x3];
            currPtr16[1] = DXT_BlockColors[(indices >> 10) & 0x3];
            currPtr16[2] = DXT_BlockColors[(indices >> 12) & 0x3];
            currPtr16[3] = DXT_BlockColors[(indices >> 14) & 0x3];
            currPtr16 += LineStride16;
            indices = *dataPtr16++;
            currPtr16[0] = DXT_BlockColors[(indices >> 0) & 0x3];
            currPtr16[1] = DXT_BlockColors[(indices >> 2) & 0x3];
            currPtr16[2] = DXT_BlockColors[(indices >> 4) & 0x3];
            currPtr16[3] = DXT_BlockColors[(indices >> 6) & 0x3];
            currPtr16 += LineStride16;
            currPtr16[0] = DXT_BlockColors[(indices >> 8) & 0x3];
            currPtr16[1] = DXT_BlockColors[(indices >> 10) & 0x3];
            currPtr16[2] = DXT_BlockColors[(indices >> 12) & 0x3];
            currPtr16[3] = DXT_BlockColors[(indices >> 14) & 0x3];
        }
        return dataPtr16;
    }

    /// @brief Uncompress 8x8 or motion-compensated block
    /// @return Pointer past whole block data in src
    template <>
    FORCEINLINE auto decodeBlock<8>(const uint16_t *dataPtr16, uint32_t *currPtr32, const uint32_t *prevPtr32, const uint32_t LineStride16) -> const uint16_t *
    {
        auto currPtr16 = reinterpret_cast<uint16_t *>(currPtr32);
        if (*dataPtr16 & DXTV_CONSTANTS::BLOCK_IS_REF)
        {
            // get motion-compensated block info
            const uint16_t blockInfo = *dataPtr16++;
            const bool fromPrev = blockInfo & DXTV_CONSTANTS::BLOCK_FROM_PREV;
            auto srcPtr16 = reinterpret_cast<const uint16_t *>(fromPrev ? prevPtr32 : currPtr32);
            // convert offsets to signed values
            constexpr int32_t halfRange = (1 << DXTV_CONSTANTS::BLOCK_MOTION_BITS) / 2 - 1;
            const auto offsetX = static_cast<int32_t>(blockInfo & DXTV_CONSTANTS::BLOCK_MOTION_MASK) - halfRange;
            const auto offsetY = static_cast<int32_t>((blockInfo >> DXTV_CONSTANTS::BLOCK_MOTION_Y_SHIFT) & DXTV_CONSTANTS::BLOCK_MOTION_MASK) - halfRange;
            // calculate start of block to copy
            srcPtr16 += offsetY * LineStride16 + offsetX;
            // copy pixels to output block
            copyBlock<8>(currPtr32, srcPtr16, LineStride16);
        }
        else
        {
            // get DXT block colors
            dataPtr16 = DXT::getBlockColors(dataPtr16, DXT_BlockColors);
            // get pixel color indices and set pixels accordingly
            for (uint32_t i = 0; i < 8; ++i)
            {
                uint16_t indices = *dataPtr16++;
                // select color by 2 bit index from [c0, c1, c2, c3], then move to next line in destination vertically
                currPtr16[0] = DXT_BlockColors[(indices >> 0) & 0x3];
                currPtr16[1] = DXT_BlockColors[(indices >> 2) & 0x3];
                currPtr16[2] = DXT_BlockColors[(indices >> 4) & 0x3];
                currPtr16[3] = DXT_BlockColors[(indices >> 6) & 0x3];
                currPtr16[4] = DXT_BlockColors[(indices >> 8) & 0x3];
                currPtr16[5] = DXT_BlockColors[(indices >> 10) & 0x3];
                currPtr16[6] = DXT_BlockColors[(indices >> 12) & 0x3];
                currPtr16[7] = DXT_BlockColors[(indices >> 14) & 0x3];
                currPtr16 += LineStride16;
            }
        }
        return dataPtr16;
    }

    template <>
    IWRAM_FUNC void UnCompWrite16bit<240>(const uint32_t *data, uint32_t *dst, const uint32_t *prevSrc, uint32_t width, uint32_t height)
    {
        constexpr uint32_t LineStride16 = 240;                    // stride to next line in dst in half words / pixels
        constexpr uint32_t LineStride32 = LineStride16 / 2;       // stride to next line in dst in words / 2 pixels
        constexpr uint32_t Block4HStride32 = 2;                   // horizontal stride to next 4x4 block in dst in words / 2 pixels
        constexpr uint32_t Block4VStride32 = 4 * LineStride32;    // vertical stride to next 4x4 block in dst in words / 2 pixels
        constexpr uint32_t Block8HStride32 = 2 * Block4HStride32; // horizontal stride to next 8x8 block in dst in words / 2 pixels
        auto dataPtr16 = reinterpret_cast<const uint16_t *>(data);
        // copy frame header and skip dummy flags and uncompressed size
        const uint16_t headerFlags = *dataPtr16++;
        dataPtr16 += 3;
        // check if we want to keep this duplicate frame
        if ((headerFlags & DXTV_CONSTANTS::FRAME_KEEP) != 0)
        {
            // Debug::printf("Duplicate frame");
            return;
        }
        // set up some variables
        for (uint32_t by = 0; by < height / DXTV_CONSTANTS::BLOCK_MAX_DIM; ++by)
        {
            uint32_t flags = 0;
            uint32_t flagsAvailable = 0;
            auto currPtr32 = dst + by * LineStride32 * DXTV_CONSTANTS::BLOCK_MAX_DIM;
            auto prevPtr32 = prevSrc == nullptr ? nullptr : prevSrc + by * LineStride32 * DXTV_CONSTANTS::BLOCK_MAX_DIM;
            for (uint32_t bx = 0; bx < width / DXTV_CONSTANTS::BLOCK_MAX_DIM; ++bx)
            {
                // read flags if we need to
                if (flagsAvailable < 1)
                {
                    flags = *dataPtr16++;
                    flagsAvailable = 16;
                }
                // check if block is split
                if (flags & 1)
                {
#ifdef USE_ASM
                    dataPtr16 = DecodeBlock4x4(dataPtr16, currPtr32, LineStride16 * 2, prevPtr32);                                                                         // A - upper-left
                    dataPtr16 = DecodeBlock4x4(dataPtr16, currPtr32 + Block4HStride32, LineStride16 * 2, prevPtr32 + Block4HStride32);                                     // B - upper-right
                    dataPtr16 = DecodeBlock4x4(dataPtr16, currPtr32 + Block4VStride32, LineStride16 * 2, prevPtr32 + Block4VStride32);                                     // C - lower-left
                    dataPtr16 = DecodeBlock4x4(dataPtr16, currPtr32 + Block4VStride32 + Block4HStride32, LineStride16 * 2, prevPtr32 + Block4VStride32 + Block4HStride32); // D - lower-right
#else
                    dataPtr16 = decodeBlock<4>(dataPtr16, currPtr32, prevPtr32, LineStride16);                                                                         // A - upper-left
                    dataPtr16 = decodeBlock<4>(dataPtr16, currPtr32 + Block4HStride32, prevPtr32 + Block4HStride32, LineStride16);                                     // B - upper-right
                    dataPtr16 = decodeBlock<4>(dataPtr16, currPtr32 + Block4VStride32, prevPtr32 + Block4VStride32, LineStride16);                                     // C - lower-left
                    dataPtr16 = decodeBlock<4>(dataPtr16, currPtr32 + Block4VStride32 + Block4HStride32, prevPtr32 + Block4VStride32 + Block4HStride32, LineStride16); // D - lower-right
#endif
                }
                else
                {
#ifdef USE_ASM
                    dataPtr16 = DecodeBlock8x8(dataPtr16, currPtr32, LineStride16 * 2, prevPtr32);
#else
                    dataPtr16 = decodeBlock<8>(dataPtr16, currPtr32, prevPtr32, LineStride16);
#endif
                }
                currPtr32 += Block8HStride32;
                prevPtr32 += Block8HStride32;
                flags >>= 1;
                --flagsAvailable;
            }
        }
    }

    uint32_t UnCompGetSize(const uint32_t *data)
    {
        return data[1];
    }
}
