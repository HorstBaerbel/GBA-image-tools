#include "videodecoder.h"

#include "base.h"
#include "decompression.h"
#include "dma.h"

#include "processingtypes.h"

namespace Video
{

    /// @brief Division table for x/3 where x is in [0,3*31]
    IWRAM_DATA ALIGN(4) const uint32_t OneThirdTable[94] = {
        0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3,
        4, 4, 4, 5, 5, 5, 6, 6, 6, 7,
        7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
        10, 11, 11, 11, 12, 12, 12, 13, 13, 13,
        14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
        17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
        20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
        24, 24, 24, 25, 25, 25, 26, 26, 26, 27,
        27, 27, 28, 28, 28, 29, 29, 29, 30, 30,
        30, 31, 31};

    IWRAM_DATA ALIGN(4) uint16_t colors[4];

    // Decompresses one DXT1 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    // See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    // Blocks are stored sequentially from left to right, top to bottom, but colors and
    // indices are stored separately for better compression (first all colors, then all indices)
    // See also: https://stackoverflow.com/questions/56474930/efficiently-implementing-dxt1-texture-decompression-in-hardware
    IWRAM_FUNC void UnCompDXTGWrite16(uint16_t *dst, const uint16_t *src, uint32_t width, uint32_t height)
    {
        const uint32_t nrOfBlocks = width / 4 * height / 4;
        const uint32_t LineStride16 = width;                 // stride to next line in dst (width * 2 bytes)
        const uint32_t BlockLineStride16 = LineStride16 * 4; // vertical stride to next block in dst (4 lines)
        const uint32_t BlockStride16 = 4;                    // horizontal stride to next block in dst (4 * 2 bytes)
        const uint32_t DstStride32 = (LineStride16 - 4) / 2;
        auto colorPtr = src;
        auto indexPtr = reinterpret_cast<const uint32_t *>(src + nrOfBlocks * 8 / 4);
        for (uint32_t blockY = 0; blockY < height; blockY += 4)
        {
            auto blockLineDst = dst;
            for (uint32_t blockX = 0; blockX < width; blockX += 4)
            {
                // get anchor colors c0 and c1
                uint32_t c0 = *colorPtr++;
                uint32_t c1 = *colorPtr++;
                colors[0] = c0;
                colors[1] = c1;
                // calculate intermediate colors c2 and c3
                uint32_t b0 = (c0 & 0x7C00) >> 9;  // 5 bits of blue
                uint32_t b1 = (c1 & 0x7C00) >> 10; // 5 bits of blue
                uint32_t bT = OneThirdTable[b0 + b1];
                uint32_t c2 = bT << 10;
                uint32_t c3 = (4 * bT - b0) << 9;
                uint32_t g0 = (c0 & 0x3E0) >> 4; // 5 bits of green
                uint32_t g1 = (c1 & 0x3E0) >> 5; // 5 bits of green
                uint32_t gT = OneThirdTable[g0 + g1];
                c2 |= gT << 5;
                c3 |= (4 * gT - g0) << 4;
                uint32_t r0 = c0 & 0x1F; // 5 bits of red
                uint32_t r1 = c1 & 0x1F; // 5 bits of red
                uint32_t rT = OneThirdTable[2 * r0 + r1];
                c2 |= rT;
                c3 |= (2 * rT - r0);
                colors[2] = c2;
                colors[3] = c3;
                // get pixel color indices
                uint32_t indices = *indexPtr++;
                // set pixels
                auto blockDst = reinterpret_cast<uint32_t *>(blockLineDst);
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = colors[(indices >> 0) & 0x3] | colors[(indices >> 2) & 0x3] << 16;
                *blockDst++ = colors[(indices >> 4) & 0x3] | colors[(indices >> 6) & 0x3] << 16;
                // move to next line in destination vertically
                blockDst += DstStride32;
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = colors[(indices >> 8) & 0x3] | colors[(indices >> 10) & 0x3] << 16;
                *blockDst++ = colors[(indices >> 12) & 0x3] | colors[(indices >> 14) & 0x3] << 16;
                // move to next line in destination vertically
                blockDst += DstStride32;
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = colors[(indices >> 16) & 0x3] | colors[(indices >> 18) & 0x3] << 16;
                *blockDst++ = colors[(indices >> 20) & 0x3] | colors[(indices >> 22) & 0x3] << 16;
                // move to next line in destination vertically
                blockDst += DstStride32;
                // select color by 2 bit index from [c0, c1, c2, c3]
                *blockDst++ = colors[(indices >> 24) & 0x3] | colors[(indices >> 26) & 0x3] << 16;
                *blockDst++ = colors[(indices >> 28) & 0x3] | colors[(indices >> 30) & 0x3] << 16;
                // move to next block in destination horizontally
                blockLineDst += BlockStride16;
            }
            // move to next block line in destination vertically
            dst += BlockLineStride16;
        }
    }

    IWRAM_FUNC void decode(uint32_t *finalDst, uint32_t *scratchPad, uint32_t scratchPadSize, const Info &info, const Frame &frame)
    {
        static_assert(sizeof(DataChunk) % 4 == 0);
        // split scratchpad into two parts
        auto scratch0 = scratchPad;
        auto scratch1 = scratchPad + scratchPadSize / (2 * 4);
        // get pointer to start of data chunk
        auto currentChunk = frame.data + sizeof(DataChunk) / 4;
        do
        {
            const auto chunk = reinterpret_cast<const DataChunk *>(currentChunk);
            const auto isFinal = (chunk->processingType & Image::ProcessingTypeFinal) != 0;
            // get pointer to start of frame data
            auto currentSrc = currentChunk + sizeof(DataChunk) / 4;
            // check where our output is going to
            auto currentDst = isFinal ? finalDst : scratchPad;
            // check wether destination is in VRAM (no 8-bit writes possible)
            const bool dstInVRAM = (((uint32_t)currentDst) >= 0x05000000) && (((uint32_t)currentDst) < 0x08000000);
            // reverse processing operation used in this stage
            switch (static_cast<Image::ProcessingType>(chunk->processingType & (~Image::ProcessingTypeFinal)))
            {
            case Image::ProcessingType::Uncompressed:
                DMA::dma_copy32(currentDst, currentSrc, chunk->uncompressedSize / 4);
                break;
            case Image::ProcessingType::CompressLz10:
                dstInVRAM ? Decompression::LZ77UnCompReadNormalWrite16bit(currentSrc, currentDst) : Decompression::LZ77UnCompReadNormalWrite8bit(currentSrc, currentDst);
                break;
            case Image::ProcessingType::CompressRLE:
                dstInVRAM ? Decompression::RLUnCompReadNormalWrite16bit(currentSrc, currentDst) : Decompression::RLUnCompReadNormalWrite8bit(currentSrc, currentDst);
                break;
            case Image::ProcessingType::CompressDXTG:
                UnCompDXTGWrite16(reinterpret_cast<uint16_t *>(currentDst), reinterpret_cast<const uint16_t *>(currentSrc), info.width, info.height);
                break;
            default:
                return;
            }
            // break if this was the last processing operation
            if (isFinal)
            {
                break;
            }
            // decide where to decode the next chunk from. our old destination is the new source
            currentChunk = currentDst;
            // swap scratchpads
            scratchPad = currentDst == scratch1 ? scratch0 : scratch1;
        } while (true);
    }
}