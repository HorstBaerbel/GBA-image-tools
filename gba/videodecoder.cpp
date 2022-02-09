#include "videodecoder.h"

#include "decompression.h"
#include "dma.h"

#include "processingtypes.h"

namespace Video
{

    /// @brief Division table for x/3 where x is in [0,3*63]
    static const uint8_t DXT1ThirdTable[190] = {
        0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3,
        4, 4, 4, 5, 5, 5, 6, 6, 6, 7,
        7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
        10, 11, 11, 11, 12, 12, 12, 13, 13, 13,
        14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
        17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
        20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
        24, 24, 24, 25, 25, 25, 26, 26, 26, 27,
        27, 27, 28, 28, 28, 29, 29, 29, 30, 30,
        30, 31, 31, 31, 32, 32, 32, 33, 33, 33,
        34, 34, 34, 35, 35, 35, 36, 36, 36, 37,
        37, 37, 38, 38, 38, 39, 39, 39, 40, 40,
        40, 41, 41, 41, 42, 42, 42, 43, 43, 43,
        44, 44, 44, 45, 45, 45, 46, 46, 46, 47,
        47, 47, 48, 48, 48, 49, 49, 49, 50, 50,
        50, 51, 51, 51, 52, 52, 52, 53, 53, 53,
        54, 54, 54, 55, 55, 55, 56, 56, 56, 57,
        57, 57, 58, 58, 58, 59, 59, 59, 60, 60,
        60, 61, 61, 61, 62, 62, 62, 63, 63};

    inline uint16_t selectColor(uint32_t index, uint16_t c0, uint16_t c1, uint16_t c2, uint16_t c3)
    {
        return index == 0 ? c0 : (index == 1 ? c1 : (index == 2 ? c2 : c3));
    }

    // DXT1 compresses one 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    // See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    // Blocks are stored sequentially from left to right, top to bottom
    // See also: https://stackoverflow.com/questions/56474930/efficiently-implementing-dxt1-texture-decompression-in-hardware
    void UnCompDXT1Write16(uint16_t *dst, const uint16_t *src, uint32_t width, uint32_t height)
    {
        const uint32_t LineStride16 = width;                 // stride to next line in dst (width * 2 bytes)
        const uint32_t BlockLineStride16 = LineStride16 * 4; // vertical stride to next block in dst (4 lines)
        const uint32_t BlockStride16 = 4;                    // horizontal stride to next block in dst (4 * 2 bytes)
        for (uint32_t blockY = 0; blockY < (height / 4); blockY += 4)
        {
            auto blockLineDst = dst;
            for (uint32_t blockX = 0; blockX < (width / 4); blockX += 4)
            {
                // get anchor colors c0 and c1
                uint16_t c0 = *src++;
                uint16_t c1 = *src++;
                // calculate intermediate colors c2 and c3
                uint32_t r0 = c0 >> 11; // 5 bits of red
                uint32_t r1 = c1 >> 11; // 5 bits of red
                uint16_t c2 = static_cast<uint16_t>(DXT1ThirdTable[2 * r0 + r1]) << 10;
                uint16_t c3 = static_cast<uint16_t>(DXT1ThirdTable[r0 + 2 * r1]) << 10;
                uint32_t g0 = (c0 & 0x7E0) >> 5; // 6 bits of green
                uint32_t g1 = (c1 & 0x7E0) >> 5; // 6 bits of green
                c2 |= static_cast<uint16_t>(DXT1ThirdTable[2 * g0 + g1]) << 5;
                c3 |= static_cast<uint16_t>(DXT1ThirdTable[g0 + 2 * g1]) << 5;
                uint32_t b0 = c0 & 0x1F; // 5 bits of blue
                uint32_t b1 = c1 & 0x1F; // 5 bits of blue
                c2 |= static_cast<uint16_t>(DXT1ThirdTable[2 * b0 + b1]);
                c3 |= static_cast<uint16_t>(DXT1ThirdTable[b0 + 2 * b1]);
                // get pixel color indices
                uint32_t indices = *reinterpret_cast<const uint32_t *>(src);
                src += 2;
                // set pixels
                auto blockDst = blockLineDst;
                for (uint32_t y = 0; y < 4; y++)
                {
                    for (uint32_t x = 0; x < 4; x++)
                    {
                        // select color by 2 bit index from [c0, c1, c2, c3]
                        blockDst[x] = selectColor(indices & 0x3, c0, c1, c2, c3);
                        indices >>= 2;
                    }
                    // move to next line in destination vertically
                    blockDst += LineStride16;
                }
                // move to next block in destination horizontally
                blockLineDst += BlockStride16;
            }
            // move to next block line in destination vertically
            dst += BlockLineStride16;
        }
    }

    void decode(uint32_t *finalDst, uint32_t *scratchPad, uint32_t scratchPadSize, const Info &info, const Frame &frame)
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
            switch (static_cast<Image::ProcessingType>(chunk->processingType & ~Image::ProcessingTypeFinal))
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
            case Image::ProcessingType::CompressDXT1:
                UnCompDXT1Write16(reinterpret_cast<uint16_t *>(currentDst), reinterpret_cast<const uint16_t *>(currentSrc), info.width, info.height);
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