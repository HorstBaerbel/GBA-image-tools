#include "videodecoder.h"

#include "codec_dxtv.h"
#include "compression/lz77.h"
#include "memory/memory.h"
#include "sys/base.h"
#include "sys/decompress.h"

#include "processing/processingtypes.h"

namespace Video
{

    IWRAM_FUNC auto decode(uint32_t *scratchPad, uint32_t scratchPadSize, const Info &info, const Frame &frame) -> const uint32_t *
    {
        static_assert(sizeof(DataChunk) % 4 == 0);
        // get pointer to start of data chunk
        auto currentChunk = frame.data + sizeof(DataChunk) / 4;
        uint32_t *currentDst = nullptr;
        do
        {
            const auto chunk = reinterpret_cast<const DataChunk *>(currentChunk);
            const auto isFinal = (chunk->processingType & Image::ProcessingTypeFinal) != 0;
            // get pointer to start of frame data
            auto currentSrc = currentChunk + sizeof(DataChunk) / 4;
            // if we're reading from start of scratchpad, write to the end and vice versa
            currentDst = currentChunk == scratchPad ? scratchPad + ((scratchPadSize / 4) - ((chunk->uncompressedSize + 3) / 4)) : scratchPad;
            // check wether destination is in VRAM (no 8-bit writes possible)
            const bool dstInVRAM = (((uint32_t)currentDst) >= 0x05000000) && (((uint32_t)currentDst) < 0x08000000);
            // reverse processing operation used in this stage
            switch (static_cast<Image::ProcessingType>(chunk->processingType & (~Image::ProcessingTypeFinal)))
            {
            case Image::ProcessingType::Uncompressed:
                Memory::memcpy32(currentDst, currentSrc, chunk->uncompressedSize / 4);
                break;
            case Image::ProcessingType::CompressLZ10:
                dstInVRAM ? Decompress::LZ77UnCompWrite16bit(currentSrc, currentDst) : Decompress::LZ77UnCompWrite8bit(currentSrc, currentDst);
                break;
            case Image::ProcessingType::CompressRLE:
                dstInVRAM ? BIOS::RLUnCompReadNormalWrite16bit(currentSrc, currentDst) : BIOS::RLUnCompReadNormalWrite8bit(currentSrc, currentDst);
                break;
            case Image::ProcessingType::CompressDXTV:
                DXTV::UnCompWrite16bit<240>(currentDst, currentSrc, (const uint32_t *)VRAM, info.width, info.height);
                break;
            default:
                return currentDst;
            }
            // break if this was the last processing operation
            if (isFinal)
            {
                break;
            }
            // decide where to decode the next chunk from. our old destination is the new source
            currentChunk = currentDst;
        } while (true);
        return currentDst;
    }
}