#include "dxt.h"

#include "dxt_tables.h"

#include "sys/base.h"

namespace DXT
{

    IWRAM_DATA ALIGN(4) uint16_t blockColors[4]; // intermediate DXT block color storage

    template <>
    IWRAM_FUNC void UnCompWrite16bit<240>(uint16_t *dst, const uint16_t *src, uint32_t width, uint32_t height)
    {
        constexpr uint32_t LineStride16 = 240;                   // stride to next line in dst (screen width * 2 bytes)
        constexpr uint32_t BlockLineStride16 = LineStride16 * 4; // vertical stride to next block in dst (4 lines)
        constexpr uint32_t BlockStride16 = 4;                    // horizontal stride to next block in dst (4 * 2 bytes)
        constexpr uint32_t DstStride16 = LineStride16 - 4;
        const uint32_t nrOfBlocks = width / 4 * height / 4;
        auto colorPtr = src;
        auto indexPtr = reinterpret_cast<const uint32_t *>(src + nrOfBlocks * 8 / 4);
        for (uint32_t blockY = 0; blockY < height / 4; blockY++)
        {
            auto blockLineDst = dst;
            for (uint32_t blockX = 0; blockX < width / 4; blockX++)
            {
                // get DXT block colors
                colorPtr = getBlockColors(colorPtr, blockColors);
                // get pixel color indices and set pixels accordingly
                uint32_t indices = *indexPtr++;
                auto blockDst = blockLineDst;
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = blockColors[(indices >> 0) & 0x3];
                *blockDst++ = blockColors[(indices >> 2) & 0x3];
                *blockDst++ = blockColors[(indices >> 4) & 0x3];
                *blockDst++ = blockColors[(indices >> 6) & 0x3];
                // move to next line in destination vertically
                blockDst += DstStride16;
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = blockColors[(indices >> 8) & 0x3];
                *blockDst++ = blockColors[(indices >> 10) & 0x3];
                *blockDst++ = blockColors[(indices >> 12) & 0x3];
                *blockDst++ = blockColors[(indices >> 14) & 0x3];
                // move to next line in destination vertically
                blockDst += DstStride16;
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = blockColors[(indices >> 16) & 0x3];
                *blockDst++ = blockColors[(indices >> 18) & 0x3];
                *blockDst++ = blockColors[(indices >> 20) & 0x3];
                *blockDst++ = blockColors[(indices >> 22) & 0x3];
                // move to next line in destination vertically
                blockDst += DstStride16;
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = blockColors[(indices >> 24) & 0x3];
                *blockDst++ = blockColors[(indices >> 26) & 0x3];
                *blockDst++ = blockColors[(indices >> 28) & 0x3];
                *blockDst++ = blockColors[(indices >> 30) & 0x3];
                // move to next block in destination horizontally
                blockLineDst += BlockStride16;
            }
            // move to next block line in destination vertically
            dst += BlockLineStride16;
        }
    }

}
